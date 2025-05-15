#pragma once

#include "functions.h"
#include "debug.h"
#include "wrong.h"
#include "tool.h"

std::unordered_map<int,int> loadRealTag() {
    if constexpr (submit) {
        return std::unordered_map<int,int> ();
    } else {
        std::unordered_map<int,int> result;
        std::ifstream in("data/sample_official_map_1.txt");
        if (!in) {
            throw std::runtime_error("无法打开文件");
        }
        int key, value;
        // 按“键 值”格式逐对读取
        while (in >> key >> value) {
            result[key] = value;
        }
        // 可选：检查是否因为格式错误而提前退出
        if (!in.eof() && in.fail()) {
            throw std::runtime_error("读取文件时遇到格式错误");
        }
        return result;

    }

}
