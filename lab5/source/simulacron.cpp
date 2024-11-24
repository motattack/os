#include <iostream>
#include "my_serial.hpp"
#include <random>
#include <string>
#include <cstdint>
#include "crc.h"

void csleep(double timeout) {
#if defined (WIN32)
    if (timeout <= 0.0)
        ::Sleep(INFINITE);
    else
        ::Sleep(static_cast<DWORD>(timeout * 1e3));
#else
    if (timeout <= 0.0)
        pause();
    else {
        struct timespec t;
        t.tv_sec = (int)timeout;
        t.tv_nsec = (int)((timeout - t.tv_sec)*1e9);
        nanosleep(&t, NULL);
    }
#endif
}

class TemperatureSensorSimulator {
public:
    static float GenerateTargetTemperature() {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<float> dist(-31.4f, 33.6f);
        return dist(rng);
    }

    static float GenerateRandomAdjustmentStep() {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<float> dist(0.05f, 0.7f);
        return dist(rng);
    }

    static float SmoothlyChangeTemperature(float currentTemperature, const float targetTemperature) {
        const float adjustmentStep = GenerateRandomAdjustmentStep();

        if (currentTemperature < targetTemperature) {
            currentTemperature += adjustmentStep;
            if (currentTemperature > targetTemperature) {
                currentTemperature = targetTemperature;
            }
        } else if (currentTemperature > targetTemperature) {
            currentTemperature -= adjustmentStep;
            if (currentTemperature < targetTemperature) {
                currentTemperature = targetTemperature;
            }
        }
        return currentTemperature;
    }

    static void PublishTemperature(cplib::SerialPort &port) {
        float currentTemperature = GenerateTargetTemperature();
        float targetTemperature = GenerateTargetTemperature();

        while (true) {
            currentTemperature = SmoothlyChangeTemperature(currentTemperature, targetTemperature);

            if (currentTemperature == targetTemperature)
                targetTemperature = GenerateTargetTemperature();

            const std::string temperature_text = std::to_string(currentTemperature);
            const uint32_t crc = CRC::CalculateCRC(temperature_text);
            std::string data = std::to_string(crc);
            data += STANDARD_DELIMITER;
            data += temperature_text;

            std::cout << data << std::endl;

            port << data << "\n";
            csleep(SERIAL_PORT_DEFAULT_TIMEOUT);
        }
    }
};

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: simulacron [port]" << std::endl;
        return -1;
    }

    const std::string portName = argv[1];

    const cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_115200);
    cplib::SerialPort serialPort;

    if (serialPort.Open(portName, params) != cplib::SerialPort::RE_OK) {
        std::cerr << "Error opening the port: " << portName << ". Terminating..." << std::endl;
        return -2;
    }

    TemperatureSensorSimulator::PublishTemperature(serialPort);

    serialPort.Close();
    return 0;
}
