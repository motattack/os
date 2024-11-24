#include <iostream>
#include <numeric>
#include <chrono>
#include <sstream>
#include <vector>

#include "sqlite3.h"
#include "my_serial.hpp" // changed: Read(str, timeout)
#include "crc.h"

sqlite3* db;

void initDatabase() {
    if (sqlite3_open("logs.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }

    // Переключение в режим WAL, чтобы другие могли читать базу.
    const char* pragmaWAL = "PRAGMA journal_mode=WAL;";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, pragmaWAL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to set WAL mode: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(1);
    }

    const char* createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            log_type TEXT NOT NULL,
            temperature TEXT NOT NULL
        );
    )";

    if (sqlite3_exec(db, createTableQuery, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(1);
    }
}

void logToDatabase(const std::string& logType, float temperature) {
    const char* insertQuery = R"(
        INSERT INTO logs (log_type, temperature) VALUES (?, ?);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, logType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, std::to_string(temperature).c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to insert log: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

void cleanupLogs(const std::string& logType, const std::chrono::hours& maxAge) {
    auto cutoffTime = std::chrono::system_clock::now() - maxAge;
    std::time_t cutoffTimeT = std::chrono::system_clock::to_time_t(cutoffTime);

    char formattedTime[20]; // "YYYY-MM-DD HH:MM:SS" + null-терминированный символ
    std::strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", std::gmtime(&cutoffTimeT));

    const char* deleteQuery = R"(
        DELETE FROM logs WHERE log_type = ? AND (timestamp < ? OR timestamp > CURRENT_TIMESTAMP);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, deleteQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare delete statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, logType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, formattedTime, -1, SQLITE_STATIC);

    int rowsAffected = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rowsAffected++;
    }

    if (rowsAffected > 0)
        std::cout << "Deleted " << rowsAffected << " logs of type '" << logType << "'." << std::endl;

    sqlite3_finalize(stmt);
}

int main(int argc, char** argv) {
    initDatabase();

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
            logToDatabase("all", temperature);

            hourlyTemperatures.push_back(temperature);
            dailyTemperatures.push_back(temperature);

            auto now = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed = now - startTime;
            if (elapsed.count() >= 3600) {
                const float hourlyAvg = std::accumulate(hourlyTemperatures.begin(), hourlyTemperatures.end(), 0.0f) / static_cast<float>(hourlyTemperatures.size());
                logToDatabase("hourly", hourlyAvg);
                hourlyTemperatures.clear();
                startTime = now;
            }

            if (elapsed.count() >= 86400) {
                const float dailyAvg = std::accumulate(dailyTemperatures.begin(), dailyTemperatures.end(), 0.0f) / static_cast<float>(dailyTemperatures.size());
                logToDatabase("daily", dailyAvg);
                dailyTemperatures.clear();
                startTime = now;
            }
        }

        cleanupLogs("all", std::chrono::hours(24));
        cleanupLogs("hourly", std::chrono::hours(24 * 30));
        cleanupLogs("daily", std::chrono::hours(24 * 365));
    }

    serialPort.Close();
    sqlite3_close(db);
    return 0;
}
