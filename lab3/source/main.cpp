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

template<class T>
class Counter final : public cplib::Thread {
public:
    explicit Counter(cplib::SharedMem<T> *shared_data) : _display_console(false) {
        _ptr_shared_data = shared_data;
    }

    int MainStart() override {
        if (_display_console)
            std::cout << "Counter thread starting..." << std::endl;
        return 0;
    }

    void MainQuit() override {
        if (_display_console)
            std::cout << "Counter thread stopping..." << std::endl;
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
            std::cout << "Launcher thread starting..." << std::endl;

        for (int i = 0; i < _clone_count; i++) {
            _clones[i].Start();
            _clones[i].WaitStartup();
        }

        return 0;
    }

    void MainQuit() override {
        if (_display_console)
            std::cout << "Launcher thread stopping..." << std::endl;

        for (int i = 0; i < _clone_count; i++) {
            _clones[i].Stop();
            _clones[i].Join();
        }
    }

    void Main() override {
        while (true) {
            CancelPoint();
            Sleep(TIMEOUT_3_SEC);

            for (int i = 0; i < _clone_count; i++) {
                if (_clones[i].ThreadState() == STATE_STOPPED) {
                    _clones[i].Start();
                    _clones[i].WaitStartup();
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
            std::cout << "Logger thread starting..." << std::endl;

        _log = fopen("log.txt", "a");
        if (!_log)
            return 1;

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        fprintf(_log, "PID: %d. Logger thread started at %s", _pid, std::ctime(&now_c));

        return 0;
    }

    void MainQuit() override {
        if (_display_console)
            std::cout << "Logger thread stopping..." << std::endl;

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        fprintf(_log, "PID: %d. Logger thread stopped at %s", _pid, std::ctime(&now_c));
        fclose(_log);
    }

    void Main() override {
        while (_print_counter) {
            CancelPoint();
            Sleep(TIMEOUT_3_SEC);

            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            if (!_message.empty()) {
                fprintf(_log, "%s", _message.c_str());
            }
            fprintf(_log, "PID #%d Counter: %d at %s", _pid, _ptr_shared_data->Data()->counter, std::ctime(&now_c));
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
    bool writer_is_running = false;
    bool log_is_free = true;
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

    if (argc == 2) {
        if (stricmp(argv[1], "copy_1") == 0) {
            shared_memory.Lock();
            shared_memory.Data()->counter += 10;
            shared_memory.Unlock();

            is_copy_command = true;
        } else if (stricmp(argv[1], "copy_2") == 0) {
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

    Clone clones[2] = {
        Clone("main.exe copy_1"),
        Clone("main.exe copy_2")
    };

    auto counter = Counter(&shared_memory);
    auto logger = Logger(&shared_memory, pid);
    auto launcher = Launcher(clones, 2);

    if (!shared_memory.Data()->writer_is_running and !is_copy_command) {
        shared_memory.Lock();
        is_writer = true;
        shared_memory.Data()->writer_is_running = is_writer;
        shared_memory.Unlock();
    }

    logger.Start();
    logger.WaitStartup();

    if (is_writer) {
        std::cout << "Writer initialized, PID: " << pid << std::endl;
        logger.printCounter();

        logger.enableConsoleOutput();
        counter.enableConsoleOutput();
        launcher.enableConsoleOutput();

        counter.Start();
        counter.WaitStartup();

        launcher.Start();
        launcher.WaitStartup();
    }

    std::string command;
    while (is_writer) {
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
