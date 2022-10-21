#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <stdexcept>
#include <future>
#include "Windows.h"
#include "Service.hpp"
#include "src/compfuncs.hpp"
#include "src/trialfuncs.hpp"

class Manager {
private:
    int x;
    std::string arg;
    PROCESS_INFORMATION piF{}, piG{};
    HANDLE pipeF, pipeG;
    std::variant<HARD_FAIL, SOFT_FAIL, int> resF, resG;
    int res;
    std::atomic_bool isTerminated;
    int attemptsLeftF, attemptsLeftG;


    void xInputRequest() {
        std::cout << " >> Enter x: ";
        std::cin >> x;
    }

    void createNamedPipe(char func) {
        if (func != 'f' && func != 'g') {
            std::cerr << "[M] Unexpected computation function: " << func << ".\n";
            throw std::runtime_error("");
        }

        if (func == 'f') {
            pipeF = CreateNamedPipe(
                    R"(\\.\pipe\pipe_f)", // name of the pipe
                    PIPE_ACCESS_DUPLEX, // 2-way pipe
                    PIPE_TYPE_BYTE, // send data as a byte stream
                    1, // only allow 1 instance of this pipe
                    0, // no outbound buffer
                    0, // no inbound buffer
                    0, // use default wait time
                    nullptr // use default security attributes
            );
            if (pipeF == nullptr || pipeF == INVALID_HANDLE_VALUE) {
                std::cerr << "[M] Failed to create pipeF instance.\n";
                throw std::runtime_error("");
            }
        }
        else {
            pipeG = CreateNamedPipe(
                    R"(\\.\pipe\pipe_g)", // name of the pipe
                    PIPE_ACCESS_DUPLEX, // 2-way pipe
                    PIPE_TYPE_BYTE, // send data as a byte stream
                    1, // only allow 1 instance of this pipe
                    0, // no outbound buffer
                    0, // no inbound buffer
                    0, // use default wait time
                    nullptr // use default security attributes
            );
            if (pipeG == nullptr || pipeG == INVALID_HANDLE_VALUE) {
                std::cerr << "[M] Failed to create pipeG instance.\n";
                throw std::runtime_error("");
            }
        }
    }

    void createProcess(char func) {
        if (func != 'f' && func != 'g') {
            std::cerr << "[M] Unexpected computation function: " << func << ".\n";
            throw std::runtime_error("");
        }

        if (func == 'f') {
            STARTUPINFO siF;

            ZeroMemory(&siF, sizeof(siF));
            siF.cb = sizeof(siF);
            ZeroMemory(&piF, sizeof(piF));

            std::string cmdF = arg + " f " + std::to_string(x);
            char* c_cmdF = const_cast<char*>(cmdF.c_str());

            if (!CreateProcess( nullptr, // No module name (use command line)
                                c_cmdF, // Command line
                                nullptr, // Process handle not inheritable
                                nullptr, // Thread handle not inheritable
                                false, // Set handle inheritance to FALSE
                                0, // Flags //CREATE_NEW_CONSOLE
                                nullptr, // Use parent's environment block
                                nullptr, // Use parent's starting directory
                                &siF,  // Pointer to STARTUPINFO structure
                                &piF ) // Pointer to PROCESS_INFORMATION structure
                    )
            {
                std::cerr << "[M] Failed to create process for f-function computation.\n";
                throw std::runtime_error("");
            }
        }
        else {
            STARTUPINFO siG;

            ZeroMemory(&siG, sizeof(siG));
            siG.cb = sizeof(siG);
            ZeroMemory(&piG, sizeof(piG));

            std::string cmdG = arg + " g " + std::to_string(x);
            char* c_cmdG = const_cast<char*>(cmdG.c_str());

            if (!CreateProcess( nullptr, // No module name (use command line)
                                c_cmdG, // Command line
                                nullptr, // Process handle not inheritable
                                nullptr, // Thread handle not inheritable
                                false, // Set handle inheritance to FALSE
                                0, // Flags //CREATE_NEW_CONSOLE
                                nullptr, // Use parent's environment block
                                nullptr, // Use parent's starting directory
                                &siG, // Pointer to STARTUPINFO structure
                                &piG ) // Pointer to PROCESS_INFORMATION structure
                    )
            {
                std::cerr << "[M] Failed to create process for g-function computation.\n";
                throw std::runtime_error("");
            }
        }
    }

    static std::variant<HARD_FAIL, SOFT_FAIL, int> readResultFromNamedPipe(char func, Manager* manager, HANDLE* pipe, std::mutex* mx) {
        ConnectNamedPipe(*pipe, nullptr);
        if (manager->isTerminated) {
            return {};
        }

        std::variant<HARD_FAIL, SOFT_FAIL, int> data;
        void* buffer = static_cast<void*>(&data);
        bool flag = ReadFile(
                *pipe,
                buffer, // the data from the pipe will be put here
                sizeof(data), // number of bytes allocated
                nullptr, // number of bytes actually read
                nullptr // not using overlapped IO
        );
        if (!flag) {
            std::cerr << "[M] Failed to read data from named pipe.\n";
            throw std::runtime_error("");
        }
        auto* funcRes = static_cast<std::variant<HARD_FAIL, SOFT_FAIL, int>*>(buffer);
        bool goAttempt = false;
        mx->lock();
        if (!manager->isTerminated) {
            if (std::holds_alternative<HARD_FAIL>(*funcRes)) {
                std::cout << " -- " << func << "(x): " << *funcRes << "\n";
                manager->terminateProcessAndPipe(func == 'f' ? 'g' : 'f');
            }
            else if (std::holds_alternative<SOFT_FAIL>(*funcRes)) {
                manager->decrementAttemptsLeft(func);
                int attemptsLeft = manager->attemptsLeft(func);
                if (attemptsLeft > 0) {
                    std::cout << " -- " << func << "(x): " << *funcRes << " [" << attemptsLeft << "]\n";
                    manager->closeHandles(func);
                    manager->createNamedPipe(func);
                    manager->createProcess(func);
                    goAttempt = true;
                }
                else {
                    std::cout << " -- " << func << "(x): " << *funcRes << "\n";
                    manager->terminateProcessAndPipe(func == 'f' ? 'g' : 'f');
                }
            }
            else {
                std::cout << " -- " << func << "(x): " << *funcRes << "\n";
            }
        }
        mx->unlock();
        if (goAttempt)
            return readResultFromNamedPipe(func, manager, pipe, mx);
        return *funcRes;
    }

    void receiveResults() {
        std::mutex mx;
        isTerminated = false;

        auto futureF = std::async(std::launch::async, readResultFromNamedPipe, 'f', this, &pipeF, &mx);
        auto futureG = std::async(std::launch::async, readResultFromNamedPipe, 'g', this, &pipeG, &mx);

        resF = futureF.get();
        resG = futureG.get();
    }

    void calculateFinalResult() {
        if (holds_alternative<int>(resF) && holds_alternative<int>(resG)) {
            res = std::get<int>(resF) + std::get<int>(resG);
        }
        else {
            res = -2;
        }

        if (res != -2) {
            std::cout << "  Result: " << res << "\n";
        }
        else {
            std::cout << "  Result: <fail>\n";
        }
    }

    void terminateProcessAndPipe(char func) {
        if (func != 'f' && func != 'g') {
            std::cerr << "[M] Unexpected terminateProcessAndPipe() function parameter.\n";
            throw std::runtime_error("");
        }

        isTerminated = true;
        if (func == 'f') {
            TerminateProcess(piF.hProcess, 1);
        }
        else { // forFunc == 'g'
            TerminateProcess(piG.hProcess, 1);
        }

        std::string pipeName = std::string(R"(\\.\pipe\pipe_)") + func;
        char* c_pipeName = const_cast<char*>(pipeName.c_str());

        CreateFile(
                c_pipeName,
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
        );
    }

    void closeHandles(char func) {
        if (func != 'f' && func != 'g') {
            std::cerr << "[M] Unexpected computation function: " << func << ".\n";
            throw std::runtime_error("");
        }

        if (func == 'f') {
            DisconnectNamedPipe(pipeF);
            CloseHandle(pipeF);
            CloseHandle(piF.hProcess);
            CloseHandle(piF.hThread);
        }
        else {
            DisconnectNamedPipe(pipeG);
            CloseHandle(pipeG);
            CloseHandle(piG.hProcess);
            CloseHandle(piG.hThread);
        }
    }

    int attemptsLeft(char func) const {
        if (func != 'f' && func != 'g') {
            std::cerr << "[M] Unexpected computation function: " << func << ".\n";
            throw std::runtime_error("");
        }

        if (func == 'f')
            return attemptsLeftF;
        return attemptsLeftG;
    }

    void decrementAttemptsLeft(char func) {
        if (func != 'f' && func != 'g') {
            std::cerr << "[M] Unexpected computation function: " << func << ".\n";
            throw std::runtime_error("");
        }

        if (func == 'f')
            attemptsLeftF--;
        else
            attemptsLeftG--;
    }

public:
    explicit Manager(std::string arg, int maxAmountOfAttempts) : arg(std::move(arg)) {
        x = 0;
        piF = {};
        piG = {};
        pipeF = nullptr;
        pipeG = nullptr;
        res = -1;
        isTerminated = false;
        attemptsLeftF = maxAmountOfAttempts;
        attemptsLeftG = maxAmountOfAttempts;
    }

    void run() {
        xInputRequest();
        createNamedPipe('f');
        createNamedPipe('g');
        createProcess('f');
        createProcess('g');
        receiveResults(); // resF, resG
        calculateFinalResult();
        closeHandles('f');
        closeHandles('g');
    }
};


#endif //MANAGER_HPP
