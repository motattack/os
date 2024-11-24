#include <iostream>
#include <fstream>
#include <numeric>
#include <chrono>

#include "my_serial.hpp" // changed: Read(str, timeout)
#include "crc.h"

#if !defined(WIN32)
#    include <unistd.h>
#    include <time.h>
#endif


const std::string logFileHourly = "temperature_hourly.log";
const std::string logFileDaily = "temperature_daily.log";
const std::string logFileAll = "temperature_all.log";

std::string trim(const std::string &str) {
    const size_t start = str.find_first_not_of(" \t");
    const size_t end = str.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string data;

    explicit LogEntry(const std::string &line) {
        std::istringstream iss(line);
        std::tm tm = {};
        char delim;
        iss >> std::get_time(&tm, "[%d.%m.%Y %H:%M:%S]") >> delim;
        timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        std::getline(iss, data);
        data = trim(data);
    }

    [[nodiscard]] bool isOlderThan(const std::chrono::hours &duration) const {
        return std::chrono::system_clock::now() - timestamp > duration;
    }

    [[nodiscard]] bool isTimeTravelerDetected() const {
        return std::chrono::system_clock::now() - timestamp < std::chrono::seconds(0);
    }
};


void logData(const std::string &fileName, const std::string &data) {
    if (std::ofstream outFile(fileName, std::ios_base::app); outFile.is_open())
        outFile << data << std::endl;
    else
        std::cerr << "Error writing to file " << fileName << std::endl;
}


void cleanupLogFile(const std::string &fileName, const std::chrono::hours &maxAge) {
    std::ifstream inFile(fileName);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open file for cleaning: " << fileName << std::endl;
        return;
    }

    std::vector<LogEntry> entries;
    std::string line;
    while (std::getline(inFile, line)) {
        if (LogEntry entry(line); !entry.isOlderThan(maxAge) && !entry.isTimeTravelerDetected())
            entries.push_back(entry);
        else if (entry.isTimeTravelerDetected())
            std::cout << std::string(SAMPLE_TEXT) << std::endl;
    }

    inFile.close();

    if (std::ofstream outFile(fileName, std::ios_base::trunc); outFile.is_open()) {
        for (const auto &entry: entries) {
            std::time_t t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::tm *tmNow = std::localtime(&t);
            outFile << std::put_time(tmNow, "[%d.%m.%Y %H:%M:%S]") << " - " << entry.data << std::endl;
        }
    } else
        std::cerr << "Failed to open file for writing: " << fileName << std::endl;
}

void logAverage(const std::string &logFileName, const std::vector<float> &temperatures, const std::string &label) {
    if (!temperatures.empty()) {
        const float sum = std::accumulate(temperatures.begin(), temperatures.end(), 0.0f);
        const float average = sum / static_cast<float>(temperatures.size());
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const std::tm *tmNow = std::localtime(&now);
        std::stringstream ss;
        ss << std::put_time(tmNow, "[%d.%m.%Y %H:%M:%S]") << " - " << label << ": " << average;
        logData(logFileName, ss.str());
    }
}

void logAllData(const std::string &message) {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const std::tm *tmNow = std::localtime(&now);
    std::stringstream ss;
    ss << std::put_time(tmNow, "[%d.%m.%Y %H:%M:%S]") << " - " << message;
    logData(logFileAll, ss.str());
}

void ensureLogFilesExist(const std::vector<std::string> &logFiles) {
    for (const auto &logFile: logFiles)
        if (std::ifstream infile(logFile); !infile.good())
            if (std::ofstream outfile(logFile); !outfile.is_open())
                std::cerr << "Error creating log file: " << logFile << std::endl;
}

int main(int argc, char **argv) {
    ensureLogFilesExist({logFileHourly, logFileDaily, logFileAll});

    if (argc < 2) {
        std::cout << "Usage: main [port]" << std::endl;
        return -1;
    }

    const cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_115200);
    cplib::SerialPort serialPort;

    const std::string portName = argv[1];
    if (serialPort.Open(portName, params) != cplib::SerialPort::RE_OK) {
        std::cerr << "Error opening the port: " << portName << ". Terminating..." << std::endl;
        return -2;
    }

    CRC crcCalculator;
    crcCalculator.ShowWarnings();
    std::string message{};
    float temperature;

    std::vector<float> hourlyTemperatures;
    std::vector<float> dailyTemperatures;

    serialPort.SetTimeout(1.0);
    auto startTime = std::chrono::system_clock::now();

    while (true) {
        serialPort >> message;
        if (message.empty())
            continue;

        if (crcCalculator.VerifyCRC(message, temperature)) {
            std::cout << "Temperature: " + std::to_string(temperature) << std::endl;
            logAllData("Temperature: " + std::to_string(temperature));

            hourlyTemperatures.push_back(temperature);
            dailyTemperatures.push_back(temperature);

            auto now = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed = now - startTime;
            if (elapsed.count() >= 3600) {
                logAverage(logFileHourly, hourlyTemperatures, "Hourly Avg");
                startTime = now;
            }

            if (elapsed.count() >= 86400) {
                logAverage(logFileDaily, dailyTemperatures, "Daily Avg");
                startTime = now;
            }
        }

        cleanupLogFile(logFileAll, std::chrono::hours(24));
        cleanupLogFile(logFileHourly, std::chrono::hours(24 * 30));
        cleanupLogFile(logFileDaily, std::chrono::hours(24 * 365));
    }

    serialPort.Close();
    return 0;
}
