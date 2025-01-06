#if defined(WIN32)
#   include <winsock2.h>    /* socket */
#   include <ws2tcpip.h>    /* ipv6 */
#else
#   include <sys/socket.h>  /* socket */
#   include <netinet/in.h>  /* socket */
#   include <arpa/inet.h>   /* socket */
#   include <unistd.h>      /* close */
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define SOCKET_ERROR -1
#endif

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <iomanip>

#include "sqlite3.h"

// Сервер запустится на localhost
// Порт сервера — 80. Для Linux требуется запуск с правами root (sudo),
// так как порты ниже 1024 могут быть открыты только суперпользователем.
#define DEFAULT_SERVER_PORT 80
#define DEFAULT_SERVER_ADDRESS "127.0.0.1"

struct LogEntry {
    std::string timestamp;
    std::string log_type;
    std::string temperature;
};

std::string readFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "<html><body><h1>Error: Cannot open file!</h1></body></html>";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string convertToUTC(const std::string &localTimeStr) {
    std::tm tm_utc = {};
    std::istringstream ss(localTimeStr);
    ss >> std::get_time(&tm_utc, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Error parsing time" << std::endl;
        return "";
    }

    std::time_t local_time = std::mktime(&tm_utc);
    if (local_time == -1) {
        std::cerr << "Error converting time" << std::endl;
        return "";
    }

    std::tm* utc_tm = std::gmtime(&local_time);
    std::ostringstream utc_time_stream;
    utc_time_stream << std::put_time(utc_tm, "%Y-%m-%d %H:%M:%S");

    return utc_time_stream.str();
}

std::vector<LogEntry> queryDatabase(const std::string &logType, const std::string &startDate,
                                    const std::string &endDate, int offset, int limit) {
    limit = std::min(limit, 100);

    sqlite3 *db;
    if (sqlite3_open("logs.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open database!" << std::endl;
        return {};
    }

    std::string startDateUTC = convertToUTC(startDate);
    std::string endDateUTC = convertToUTC(endDate);

    std::vector<LogEntry> logs;
    std::string query =
        "SELECT datetime(timestamp, 'localtime') AS local_timestamp, log_type, temperature FROM logs WHERE log_type = ? AND timestamp BETWEEN ? AND ? LIMIT ? OFFSET ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, logType.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, startDateUTC.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, endDateUTC.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, limit);
        sqlite3_bind_int(stmt, 5, offset);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LogEntry entry;
            entry.timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            entry.log_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            entry.temperature = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            logs.push_back(entry);
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to prepare SQL statement!" << std::endl;
    }

    sqlite3_close(db);
    return logs;
}

LogEntry queryLastTemperature() {
    sqlite3 *db;
    LogEntry lastEntry;

    if (sqlite3_open("logs.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open database!" << std::endl;
        return lastEntry;
    }

    std::string query = "SELECT datetime(timestamp, 'localtime') AS local_timestamp, log_type, temperature FROM logs ORDER BY timestamp DESC LIMIT 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            lastEntry.timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            lastEntry.log_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            lastEntry.temperature = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to prepare SQL statement!" << std::endl;
    }

    sqlite3_close(db);
    return lastEntry;
}

std::unordered_map<std::string, std::string> parseQueryString(const std::string &query) {
    std::unordered_map<std::string, std::string> params;
    std::stringstream ss(query);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            std::replace(value.begin(), value.end(), '+', ' ');
            params[key] = value;
        }
    }
    return params;
}

std::string generateJSON(const std::vector<LogEntry> &logs) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < logs.size(); ++i) {
        json << "{"
                << "\"timestamp\":\"" << logs[i].timestamp << "\","
                << "\"log_type\":\"" << logs[i].log_type << "\","
                << "\"temperature\":\"" << logs[i].temperature << "\""
                << "}";
        if (i < logs.size() - 1) {
            json << ",";
        }
    }
    json << "]";
    return json.str();
}

void handleClient(SOCKET clientSocket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    std::string request(buffer);
    std::string response;

    if (request.find("GET / ") != std::string::npos) {
        std::string html = readFile("index.html");
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
    } else if (request.find("GET /stats") != std::string::npos) {
		size_t queryStart = request.find("?");
		size_t queryEnd = request.find(" ", queryStart);
		std::string queryString = request.substr(queryStart + 1, queryEnd - queryStart - 1);

		auto params = parseQueryString(queryString);
		std::string logType = params.count("logType") ? params["logType"] : "all";
		std::string startDate = params.count("startDate") ? params["startDate"] : "1960-01-01 00:00:00";
		std::string endDate = params.count("endDate") ? params["endDate"] : "2038-01-19 23:59:59";
		int offset = params.count("offset") ? std::stoi(params["offset"]) : 0;
		int limit = params.count("limit") ? std::stoi(params["limit"]) : 50;

		auto logs = queryDatabase(logType, startDate, endDate, offset, limit);
		std::string json = generateJSON(logs);

		response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json;
    } else if (request.find("GET /current-temperature") != std::string::npos) {
        LogEntry lastTemperature = queryLastTemperature();

        if (!lastTemperature.timestamp.empty()) {
            std::vector<LogEntry> singleLog = {lastTemperature};
            std::string json = generateJSON(singleLog);

            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json;
        } else {
            response = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n"
                    "{\"error\":\"No temperature data available\"}";
        }
    } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\nPage not found!";
    }

    send(clientSocket, response.c_str(), response.length(), 0);
#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [ip] [port]" << std::endl;
        return -1;
    }

    std::string serverAddress = argv[1];
    int serverPort = (argc == 3) ? std::stoi(argv[2]) : DEFAULT_SERVER_PORT;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return -1;
    }
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket!" << std::endl;
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);

    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket!" << std::endl;
        return -1;
    }

    listen(serverSocket, 10);
    std::cout << "Server is running on " << serverAddress << ":" << serverPort << "..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
