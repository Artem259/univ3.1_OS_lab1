#ifndef COMPUTATION_HPP
#define COMPUTATION_HPP

#include "Service.hpp"
#include "src/compfuncs.hpp"
#include "src/trialfuncs.hpp"

class Computation {
private:
    char func;
    int x;
    HANDLE pipe;
    std::variant<HARD_FAIL, SOFT_FAIL, int> result;

    void computeResult() {
        if (func == 'f')
            result = os::lab1::compfuncs::trial_f<os::lab1::compfuncs::INT_SUM>(x);
        else // func == 'g'
            result = os::lab1::compfuncs::trial_g<os::lab1::compfuncs::INT_SUM>(x);
    }

    void sendResultToNamedPipe() {
        void* resultVoid = static_cast<void*>(&result);
        bool flag = WriteFile(
                pipe, // handle to our outbound pipe
                resultVoid, // data to send
                sizeof(result), // size of data to send (bytes)
                nullptr, // actual amount of data sent
                nullptr // not using overlapped IO
        );
        if (!flag) {
            std::cerr << "[C] Failed to send data.\n";
            throw std::runtime_error("");
        }
    }

    void closeNamedPipe() {
        CloseHandle(pipe);
    }
public:
    explicit Computation(char func, int x) : func(func), x(x) {
        if (func != 'f' && func != 'g') {
            std::cerr << "[C] Unexpected computation function.\n";
            throw std::runtime_error("");
        }

        pipe = nullptr;
    }

    void connectToNamedPipe() {
        std::string pipeName = std::string(R"(\\.\pipe\pipe_)") + func;
        char* c_pipeName = const_cast<char*>(pipeName.c_str());

        pipe = CreateFile(
                c_pipeName,
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
        );
        if (pipe == INVALID_HANDLE_VALUE) {
            std::cerr << "[C] Failed to connect to pipe.\n";
            throw std::runtime_error("");
        }
    }

    void run() {
        computeResult();
        connectToNamedPipe();
        sendResultToNamedPipe();
        closeNamedPipe();
    }
};


#endif //COMPUTATION_HPP
