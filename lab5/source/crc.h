#ifndef CRC_H
#define CRC_H

#include <string>
#include <cstdint>
#include <utility>

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
