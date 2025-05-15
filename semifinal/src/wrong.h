#pragma once

#include <exception>
#include <string>
#include "debug.h"
class Exception : public std::exception { // 通用异常
public:
    Exception(const std::string &msg) : message(msg) {}
    Exception(const char* msg) : message(std::string(msg)) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};

class OutOfDiskSpace_Exception : public std::exception {
public:
    const char* what() const noexcept override {
        return "磁盘容量不足";
    }
};
class InsufficientBackpackCapacity : public std::exception {
public:
    const char* what() const noexcept override {
        return "背包容量不足";
    }
};



#define EXCEPTION_CHECK(cond, err_msg) { \
    if constexpr (!submit) {                                \
        if (cond) {                                        \
            debugFlush();                                   \
            throw Exception(err_msg);                      \
        }                                                  \
    }                                                      \
}


