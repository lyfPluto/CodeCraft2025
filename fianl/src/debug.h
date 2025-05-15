#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <array>

#include "parameters.h"
//#define SUBMIT
//#define FLUSH_IMMEDIATELY
#ifdef SUBMIT
    constexpr bool submit = true;
#else
    constexpr bool submit = false;
#endif

#ifdef FLUSH_IMMEDIATELY
    constexpr bool flush_immediately = true;
#else
    constexpr bool flush_immediately = false;
#endif

class Debug;
std::vector<Debug*> __total_debug;

#define newLine "\n"
#define Tab "    "
#define TTab "        "
#define TTTab "            "
class Debug {
public:
    #ifndef SUBMIT
        std::string fileName;
        std::ofstream file;
        Debug(std::string _fileName): fileName(_fileName), permit(false) {
            
            file.open("Debug/" + fileName + ".txt", std::ios::app);
            clear();
            __total_debug.push_back(this);
        }
        Debug(std::string _fileName, bool _permit): fileName(_fileName), permit(_permit) {
            
            file.open("Debug/" + fileName + ".txt", std::ios::app);
            clear();
            __total_debug.push_back(this);
        }
        void clear() {
            file.close();
            file.open("Debug/" + fileName + ".txt", std::ios::trunc);
            file.close();
            file.open("Debug/" + fileName + ".txt", std::ios::app);
        }
        void open() {
            permit = true;
        }
        void close() {
            permit = false;
        }
        void flush() {
            file.flush();
        }
        ~Debug() {
            file.close();
        }
    #else
        Debug(std::string fileName) {};
        Debug(std::string _fileName, bool _permit) {};
        void clear() {};
        void open() {};
        void close() {};
        void flush() {};
        ~Debug() {};
    #endif

    template<typename T>
    friend Debug& operator<<(Debug& debug, T inform);
    template<typename T>
    friend Debug& operator<<(Debug& debug, const std::vector<T>& vec);
    template<typename T, std::size_t N>
    friend Debug& operator<<(Debug& debug, const std::array<T, N>& arr);
    template<typename T1, typename T2>
    friend Debug& operator<<(Debug& debug, const std::pair<T1, T2>& p);
private:
    bool permit;

};

template<typename T>
Debug& operator<<(Debug& debug, T inform) {
    #ifndef SUBMIT
        if(debug.permit) {
            
            debug.file << inform;  
            if constexpr (flush_immediately) {
                debug.flush();
            }
        }

    #endif
    return debug;
}

void debugFlush() {
    if constexpr (!submit) {
        for (Debug* debug: __total_debug) {
            debug -> flush();
        }        
    }

}

// 模板特化：当 T 为 double 时，只保留两位小数
template<>
Debug& operator<<(Debug& debug, double inform) {
#ifndef SUBMIT
    if (debug.permit) {
        debug.file << std::fixed << std::setprecision(2) << inform;
        if constexpr (flush_immediately) {
            debug.flush();
        }
    }
#endif
    return debug;
}

// 重载版本：针对 std::vector<T>
template<typename T>
Debug& operator<<(Debug& debug, const std::vector<T>& vec) {
#ifndef SUBMIT
    if (debug.permit) {
        debug << "[ ";
        for (const auto& element : vec) {
            debug << element << " ";
        }
        debug << "]";
    }
#endif
    return debug;
}

// 针对 std::array 的重载版本
template<typename T, std::size_t N>
Debug& operator<<(Debug& debug, const std::array<T, N>& arr) {
#ifndef SUBMIT
    if (debug.permit) {
        debug << "[ ";
        for (const auto& element : arr) {
            debug << element << " ";
        }
        debug << "]";
    }
#endif
    return debug;
}

// 针对 std::pair 的重载版本
template<typename T1, typename T2>
Debug& operator<<(Debug& debug, const std::pair<T1, T2>& p) {
#ifndef SUBMIT
    if (debug.permit) {
        // 使用递归调用其它 operator<< 重载，将 pair 按 (first, second) 格式输出
        debug << "(";
        debug << p.first;
        debug << ", ";
        debug << p.second;
        debug << ")";
        if constexpr (flush_immediately) {
            debug.flush();
        }
    }
#endif
    return debug;
}



Debug Warning("Warning", false);
Debug debug("debug", true);
Debug IO("io");
Debug diskLog("disk_log", true);
Debug result("result", true);
Debug timeLog("time_check", true);
Debug perform("perform", true);
Debug data_collector("data_collector", true);
Debug objLog("object_log");



