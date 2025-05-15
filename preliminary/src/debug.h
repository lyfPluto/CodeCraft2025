#pragma once
#include<string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "hyper_parameters.h"
//#define SUBMIT

#ifdef SUBMIT
    constexpr bool submit = true;
#else
    constexpr bool submit = false;
#endif











#define newLine "\n"
#define Tab "    "
#define TTab "        "
#define TTTab "            "
class Debug {
public:
    #ifndef SUBMIT
        std::string fileName;
        Debug(std::string _fileName): fileName(_fileName), permit(false) {
            std::ofstream file("Debug/" + fileName + ".txt");
            file.close();
        }
        Debug(std::string _fileName, bool _permit): fileName(_fileName), permit(_permit) {
            std::ofstream file("Debug/" + fileName + ".txt");
            file.close();
        }
        void clear() {
            std::ofstream file("Debug/" + fileName + ".txt");
            file.close();
        }
    #else
        Debug(std::string fileName) {};
        Debug(std::string _fileName, bool _permit) {};
        void clear() {};
    #endif
    void open() {
        permit = true;
    }
    void close() {
        permit = false;
    }
    template<typename T>
    friend Debug& operator<<(Debug& debug, T inform);
private:
    bool permit;

};

template<typename T>
Debug& operator<<(Debug& debug, T inform) {
    #ifndef SUBMIT
        if(debug.permit) {
            std::ofstream file("Debug/" + debug.fileName + ".txt", std::ios::app);
            file << inform;
            file.close();            
        }

    #endif
    return debug;
}



// 模板特化：当 T 为 double 时，只保留两位小数
template<>
Debug& operator<<(Debug& debug, double inform) {
#ifndef SUBMIT
    if (debug.permit) {
        std::ofstream file("Debug/" + debug.fileName + ".txt", std::ios::app);
        file << std::fixed << std::setprecision(2) << inform;
        file.close();
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





