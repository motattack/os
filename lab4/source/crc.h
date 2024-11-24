#ifndef CRC_H
#define CRC_H

#include <string>
#include <cstdint>
#include <utility>

# define SAMPLE_TEXT {10, 32, 95, 95, 32, 32, 32, 32, 95, 95, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 95, 10, 47, 32, 47, 32, 47, 92, 32, 92, 32, 92, 95, 32, 95, 95, 32, 95, 95, 95, 32, 32, 95, 32, 95, 95, 32, 32, 32, 95, 95, 32, 95, 32, 32, 32, 95, 32, 95, 95, 32, 124, 32, 124, 32, 95, 95, 32, 95, 32, 32, 95, 95, 95, 32, 95, 95, 95, 10, 92, 32, 92, 47, 32, 32, 92, 47, 32, 47, 32, 39, 95, 95, 47, 32, 95, 32, 92, 124, 32, 39, 95, 32, 92, 32, 47, 32, 95, 96, 32, 124, 32, 124, 32, 39, 95, 32, 92, 124, 32, 124, 47, 32, 95, 96, 32, 124, 47, 32, 95, 95, 47, 32, 95, 32, 92, 10, 32, 92, 32, 32, 47, 92, 32, 32, 47, 124, 32, 124, 32, 124, 32, 40, 95, 41, 32, 124, 32, 124, 32, 124, 32, 124, 32, 40, 95, 124, 32, 124, 32, 124, 32, 124, 95, 41, 32, 124, 32, 124, 32, 40, 95, 124, 32, 124, 32, 40, 95, 124, 32, 32, 95, 95, 47, 95, 10, 32, 32, 92, 47, 32, 32, 92, 47, 32, 124, 95, 124, 32, 32, 92, 95, 95, 95, 47, 124, 95, 124, 32, 124, 95, 124, 92, 95, 95, 44, 32, 124, 32, 124, 32, 46, 95, 95, 47, 124, 95, 124, 92, 95, 95, 44, 95, 124, 92, 95, 95, 95, 92, 95, 95, 95, 40, 32, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 124, 95, 95, 95, 47, 32, 32, 124, 95, 124, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 124, 47, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 95, 32, 32, 32, 95, 10, 95, 95, 32, 32, 32, 32, 32, 32, 95, 95, 95, 32, 95, 95, 32, 95, 95, 95, 32, 32, 95, 32, 95, 95, 32, 32, 32, 95, 95, 32, 95, 32, 32, 124, 32, 124, 95, 40, 95, 41, 95, 32, 95, 95, 32, 95, 95, 95, 32, 32, 32, 95, 95, 95, 10, 92, 32, 92, 32, 47, 92, 32, 47, 32, 47, 32, 39, 95, 95, 47, 32, 95, 32, 92, 124, 32, 39, 95, 32, 92, 32, 47, 32, 95, 96, 32, 124, 32, 124, 32, 95, 95, 124, 32, 124, 32, 39, 95, 32, 96, 32, 95, 32, 92, 32, 47, 32, 95, 32, 92, 10, 32, 92, 32, 86, 32, 32, 86, 32, 47, 124, 32, 124, 32, 124, 32, 40, 95, 41, 32, 124, 32, 124, 32, 124, 32, 124, 32, 40, 95, 124, 32, 124, 32, 124, 32, 124, 95, 124, 32, 124, 32, 124, 32, 124, 32, 124, 32, 124, 32, 124, 32, 32, 95, 95, 47, 95, 10, 32, 32, 92, 95, 47, 92, 95, 47, 32, 124, 95, 124, 32, 32, 92, 95, 95, 95, 47, 124, 95, 124, 32, 124, 95, 124, 92, 95, 95, 44, 32, 124, 32, 32, 92, 95, 95, 124, 95, 124, 95, 124, 32, 124, 95, 124, 32, 124, 95, 124, 92, 95, 95, 95, 40, 95, 41, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 124, 95, 95, 95, 47, 10}

const std::string STANDARD_DELIMITER = "~##~";
constexpr char STANDARD_STR_END = '\n';

class CRC {
public:
    explicit CRC(std::string delimiter = STANDARD_DELIMITER, float showWarnings = false)
        : delimiter_(std::move(delimiter)), _showWarnings(false) {
    }

    static uint32_t CalculateCRC(const std::string &data) {
        uint32_t crc = 0xFFFFFFFF;

        for (const char i: data) {
            crc ^= static_cast<uint8_t>(i);
            for (size_t j = 0; j < 8; ++j)
                crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
        }

        return ~crc;
    }

    bool VerifyCRC(const std::string &dataWithCRC, float &temperature) const {
        const size_t delimiterPos = dataWithCRC.find(delimiter_);
        if (delimiterPos == std::string::npos) {
            if (_showWarnings)
                std::clog << "[Warning] CRC: Delimiter '" << delimiter_ << "' not found!" << std::endl;
            return false;
        }

        std::string crcString = dataWithCRC.substr(0, delimiterPos);
        std::string temperatureData = dataWithCRC.substr(delimiterPos + delimiter_.length());

        if (!temperatureData.empty() && temperatureData.back() == STANDARD_STR_END)
            temperatureData.pop_back();

        uint32_t receivedCRC = 0;
        try {
            receivedCRC = std::stoul(crcString);
        } catch (const std::invalid_argument &err) {
            if (_showWarnings)
                std::clog << "[Warning] CRC: CRC part is not a valid number!" << " (" << err.what() << ")" << std::endl;
            return false;
        } catch (const std::out_of_range &err) {
            if (_showWarnings)
                std::clog << "[Warning] CRC: CRC value is out of range!" << " (" << err.what() << ")" << std::endl;
            return false;
        }

        const uint32_t calculatedCRC = CalculateCRC(temperatureData);

        if (receivedCRC == calculatedCRC) {
            temperature = std::stof(temperatureData);
            return true;
        }
        if (_showWarnings)
            std::clog << "[Warning] CRC mismatch detected!\n"
                    << "  Received message: '" << temperatureData << "'\n"
                    << "  Received CRC: '" << receivedCRC << "'\n"
                    << "  Calculated CRC: " << calculatedCRC << "\n"
                    << "  The message will be discarded due to invalid integrity check."
                    << std::endl;
        return false;
    }

    void ShowWarnings() {
        _showWarnings = true;
    }

private:
    std::string delimiter_;
    bool _showWarnings;
};

#endif //CRC_H
