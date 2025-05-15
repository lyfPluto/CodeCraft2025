#pragma once

#include <chrono>
#include <thread>
#include <string>
#include <map>

#include "debug.h"
#include "wrong.h"


/**
每千万次调用的时间开销约为2.68秒
*/

class TimeCheck {
public:
    static void start(std::string feature) {
        if constexpr (!submit) {
            //auto now = std::chrono::high_resolution_clock::now();
            //long long time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            if(callTime.find(feature) != callTime.end()) {
                throw Exception("对同一个时间feature调用两次start函数，feature: " + feature);
            }
            callTime.insert({feature, std::chrono::duration_cast<std::chrono::nanoseconds>
                (std::chrono::high_resolution_clock::now().time_since_epoch()).count()});            
        }
    }
    static void end(std::string feature) {
        if constexpr (!submit) {
            auto now = std::chrono::high_resolution_clock::now();
            long long time = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            if(callTime.find(feature) == callTime.end()) {
                throw Exception("调用end函数前未对该时间feature调用start函数，feature: " + feature);
            }
            if(comTime.find(feature) == comTime.end()) {
                comTime.insert({feature, 0});
                callCount.insert({feature, 0});
            }
            long long runTime = time - callTime[feature];
            comTime[feature] = comTime[feature] + runTime;
            callCount[feature] = callCount[feature] + 1;
            callTime.erase(feature);
        }
    }
    static void clean() {
        if constexpr (!submit) {
            timeLog.clear();
            if(!callTime.empty()) {
                throw Exception("调用清理函数时还有时间feature未结束。");
            }
            std::map<long long, std::string, std::greater<long long>> time_sort;
            for(auto& entry: comTime) {
                time_sort[entry.second] = entry.first;
            }
            for(auto& entry: time_sort) {
                timeLog << entry.second << ": " << 
                    (entry.first) / 1000000.0 - (callCount[entry.second] * 14369.0 / 52240275)
                    << "  " << callCount[entry.second] << newLine;
            }
            comTime.clear();
        }
    }
    static void flush() {
        if constexpr (!submit) {
            timeLog.clear();
            std::map<long long, std::string, std::greater<long long>> time_sort;
            for(auto& entry: comTime) {
                time_sort[entry.second] = entry.first;
            }
            for(auto& entry: time_sort) {
                timeLog << entry.second << ": " << 
                    (entry.first) / 1000000.0 - (callCount[entry.second] * 14369.0 / 52240275)
                    << "  " << callCount[entry.second] << newLine;
            }
        }
    }
private:
    static std::map<std::string, long long> callTime;
    static std::map<std::string, long long> comTime;
    static std::map<std::string, long long> callCount;
};
std::map<std::string, long long> TimeCheck::callTime;
std::map<std::string, long long> TimeCheck::comTime;
std::map<std::string, long long> TimeCheck::callCount;



