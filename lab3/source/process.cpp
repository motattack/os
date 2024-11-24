#include "process.h"

#ifdef WIN32
#   include <windows.h>
#else
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

int Process::run(const char *command) {
#ifdef WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(
        nullptr,
        const_cast<LPSTR>(command),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        printf("Error: CreateProcess failed (%d).\n", GetLastError());
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        printf("Error: GetExitCodeProcess failed (%d).\n", GetLastError());
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -1;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode;

#else
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error forking process");
        return -1;
    } else if (pid == 0) {
        execlp("/bin/sh", "sh", "-c", command, (char*)NULL);
        perror("Error executing command");
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error waiting for child process");
            return -1;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
#endif
}

int Process::get_my_pid() {
#ifdef WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool Process::is_process_alive(int pid) {
#ifdef WIN32
    const HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == nullptr) {
        return false;
    }
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
#else
    return kill(pid, 0) == 0;
#endif
}