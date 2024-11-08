#ifndef PROCESS_H
#define PROCESS_H

class Process {
public:
    static int run(const char* command);
    static int get_my_pid();

};

#endif // PROCESS_H