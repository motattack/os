#include "my_thread.hpp" // Changed WaitStartup
#include "my_shmem.hpp"
#include "process.h"
#include <iostream>
#include <chrono>
#include <utility>

#define TIMEOUT_300_MS 0.3
#define TIMEOUT_1_SEC 1
#define TIMEOUT_2_SEC 2
#define TIMEOUT_3_SEC 3
#define PID_DOES_NOT_EXIST (-44)

char *getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_ms.time_since_epoch()) % 1000;

    std::tm *local_time = std::localtime(&now_c);

    static char time_str[24]; // Массив для хранения времени в формате "ДД.ММ.ГГГГ ЧЧ:ММ:СС.мс"

    std::snprintf(time_str, sizeof(time_str), "%02d.%02d.%04d %02d:%02d:%02d.%03d",
                  local_time->tm_mday, // День
                  local_time->tm_mon + 1, // Месяц (от 0 до 11)
                  local_time->tm_year + 1900, // Год с учетом 1900
                  local_time->tm_hour, // Часы
                  local_time->tm_min, // Минуты
                  local_time->tm_sec, // Секунды
                  ms.count()); // Миллисекунды

    return time_str;
}

template<class T>
class Counter final : public cplib::Thread {
public:
    explicit Counter(cplib::SharedMem<T> *shared_data) : _display_console(false) {
        _ptr_shared_data = shared_data;
    }

    int MainStart() override {
        if (_display_console)
            std::cout << "Counter: starting..." << std::endl;
        return 0;
    }

    void MainQuit() override {
        if (_display_console)
            std::cout << "Counter: stopping..." << std::endl;
    }

    void Main() override {
        while (true) {
            CancelPoint();
            Sleep(TIMEOUT_300_MS);
            _ptr_shared_data->Lock();
            ++_ptr_shared_data->Data()->counter;
            _ptr_shared_data->Unlock();
        }
    }

    void enableConsoleOutput() {
        _display_console = true;
    }

private:
    cplib::SharedMem<T> *_ptr_shared_data;
    bool _display_console;
};

class Clone final : public cplib::Thread {
public:
    explicit Clone(std::string command) {
        _command = std::move(command);
    }

    int MainStart() override {
        return 0;
    }

    void MainQuit() override {
    }

    void Main() override {
        Process::run(_command.c_str());
    }

private:
    std::string _command;
};

class Launcher final : public cplib::Thread {
public:
    explicit Launcher(Clone *clones, const int clone_count) : _clones(clones), _clone_count(clone_count),
                                                              _display_console(false) {
    };

    int MainStart() override {
        if (_display_console)
            std::cout << "Launcher: starting..." << std::endl;

        _log = fopen("log.txt", "a");
        if (!_log)
            return 1;

        for (int i = 0; i < _clone_count; i++) {
            _clones[i].Start();
            _clones[i].WaitStartup();
        }

        return 0;
    }

    void MainQuit() override {
        if (_display_console)
            std::cout << "Launcher: stopping..." << std::endl;

        for (int i = 0; i < _clone_count; i++) {
            _clones[i].Stop();
            _clones[i].Join();
        }

        fclose(_log);
    }

    void Main() override {
        while (true) {
            CancelPoint();
            Sleep(TIMEOUT_3_SEC);

            for (int i = 0; i < _clone_count; i++) {
                if (_clones[i].ThreadState() == STATE_STOPPED) {
                    _clones[i].Start();
                    _clones[i].WaitStartup();
                } else {
                    fprintf(_log, "[%s] Child: %d. still alive.\n", getCurrentTime(), i + 1);
                    fflush(_log);
                }
            }
        }
    }

    void enableConsoleOutput() {
        _display_console = true;
    }

private:
    Clone *_clones;
    int _clone_count;
    bool _display_console;
    FILE *_log;
};

template<class T>
class Logger final : public cplib::Thread {
public:
    explicit Logger(cplib::SharedMem<T> *shared_data, const int pid): _log(nullptr), _print_counter(false),
                                                                      _display_console(false) {
        _ptr_shared_data = shared_data;
        _pid = pid;
    }

    int MainStart() override {
        if (_display_console)
            std::cout << "Logger: starting..." << std::endl;

        _log = fopen("log.txt", "a");

        if (!_log)
            return 1;

        fprintf(_log, "[%s] PID: %d. Logger: started.\n", getCurrentTime(), _pid);
        fflush(_log);

        return 0;
    }

    void MainQuit() override {
        if (_display_console)
            std::cout << "Logger: stopping..." << std::endl;

        fprintf(_log, "[%s] PID %d. Logger: stopped.\n", getCurrentTime(), _pid);
        fclose(_log);
    }

    void Main() override {
        while (_print_counter) {
            CancelPoint();
            Sleep(TIMEOUT_3_SEC);

            if (!_message.empty()) {
                fprintf(_log, "%s", _message.c_str());
            }
            fprintf(_log, "[%s] PID %d: counter=%d.\n", getCurrentTime(), _pid, _ptr_shared_data->Data()->counter);
            fflush(_log);
        }
    }

    void setMessage(std::string msg) {
        _message = std::move(msg);
    }

    void printCounter() {
        _print_counter = true;
    }

    void enableConsoleOutput() {
        _display_console = true;
    }

private:
    cplib::SharedMem<T> *_ptr_shared_data;
    int _pid;
    FILE *_log;
    std::string _message;
    bool _print_counter;
    bool _display_console;
};

struct Data {
    int counter = 0;
    int writer_pid = PID_DOES_NOT_EXIST;
};

int main(int argc, char **argv) {
    cplib::SharedMem<Data> shared_memory("44mem");
    if (!shared_memory.IsValid()) {
        std::cout << "Failed to create shared memory block!" << std::endl;
        return -1;
    }

    int pid = Process::get_my_pid();
    bool is_writer = false;
    bool is_copy_command = false;

    if (!Process::is_process_alive(shared_memory.Data()->writer_pid)) {
        shared_memory.Lock();
        shared_memory.Data()->writer_pid = pid;
        shared_memory.Unlock();
        is_writer = true;
    }

    if (argc == 2) {
        if (strcmp(argv[1], "copy_1") == 0) {
            shared_memory.Lock();
            shared_memory.Data()->counter += 10;
            shared_memory.Unlock();

            is_copy_command = true;
        } else if (strcmp(argv[1], "copy_2") == 0) {
            shared_memory.Lock();
            shared_memory.Data()->counter *= 2;
            shared_memory.Unlock();

            cplib::Thread::Sleep(TIMEOUT_2_SEC);

            shared_memory.Lock();
            shared_memory.Data()->counter /= 2;
            shared_memory.Unlock();

            is_copy_command = true;
        }
    }

    std::string progName = argv[0];
    Clone clones[2] = {
        Clone(progName + " copy_1"), // Using argv[0] and appending " copy_1"
        Clone(progName + " copy_2") // Using argv[0] and appending " copy_2"
    };

    auto counter = Counter(&shared_memory);
    auto logger = Logger(&shared_memory, pid);
    auto launcher = Launcher(clones, 2);

    if (is_writer) {
        std::cout << "Writer initialized, PID: " << pid << std::endl;
        logger.printCounter();
    }

    if (!is_copy_command) {
        std::cout << "My PID: " << pid << std::endl;
        logger.enableConsoleOutput();
        counter.enableConsoleOutput();

        counter.Start();
        counter.WaitStartup();
    }

    logger.Start();
    logger.WaitStartup();

    if (is_writer) {
        launcher.enableConsoleOutput();
        launcher.Start();
        launcher.WaitStartup();
    }

    std::string command;
    while (!is_copy_command) {
        std::cout << "Current writer PID: " << shared_memory.Data()->writer_pid << std::endl;
        std::cout << "Enter new counter value (or 'exit' to stop): " << std::endl;
        std::getline(std::cin, command);
        if (command == "exit" or command == "e" or command == "q" or command == "quit") {
            break;
        }
        try {
            const int new_value = std::stoi(command);
            shared_memory.Lock();
            shared_memory.Data()->counter = new_value;
            shared_memory.Unlock();
        } catch (std::invalid_argument &e) {
            std::cerr << e.what() << ": Invalid input. Please enter a number." << std::endl;
        }
    }

    counter.Stop();
    logger.Stop();
    launcher.Stop();

    counter.Join();
    logger.Join();
    launcher.Join();

    if (is_writer)
        std::cout << "Buy, buy!" << std::endl;

    return 0;
}
