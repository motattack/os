#ifndef PROCESS_H
#define PROCESS_H

class Process {
public:
    static int run(const char* command);
    static int get_my_pid();
    static bool is_process_alive(int pid);
};

#endif // PROCESS_H