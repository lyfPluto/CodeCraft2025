#pragma once

#include "functions.h"
#include "wrong.h"

namespace WriteTool {

    // 连续存储
    std::vector<int> centralized_storage(int n, int size, std::function<bool(int)> state) {
        std::vector<int> result;
        if (size <= 0 || size > n) {
            throw OutOfDiskSpace_Exception();
        }
        

        int run = 0;  // 当前连续空闲块的长度
        for (int i = 0; i < n; ++i) {
            if (!state(i)) {
                // 块 i 空闲
                ++run;
                if (run == size) {
                    // 找到从 (i-size+1) 到 i 的一段连续空闲区
                    int start = i - size + 1;
                    result.reserve(size);
                    for (int j = start; j <= i; ++j) {
                        result.push_back(j);
                    }
                    return result;
                }
            } else {
                // 碰到已用块，清零计数
                run = 0;
            }
        }
        throw OutOfDiskSpace_Exception();
    }
    // 原始分离存储版本
    std::vector<int> separate_storage(int n, int size, std::function<bool(int)> state) {
        std::vector<int> result;
        if (size <= 0 || size > n) 
            return result;
    
        result.reserve(size);
        for (int i = 0; i < n && (int)result.size() < size; ++i) {
            if (!state(i)) {
                result.push_back(i);
            }
        }
        if ((int)result.size() < size) {
            throw OutOfDiskSpace_Exception();
        }
        return result;
    }
    


// 尽可能靠近中心点存储，环形磁盘
    std::vector<int> approach_center_storage(int n,
                    int size,
                    int aim_pos,
                    std::function<bool(int)> state)
{
    // 参数检查
    if (n <= 0 || size <= 0 || size > n || aim_pos < 0 || aim_pos >= n)
    throw OutOfDiskSpace_Exception();

    // 构造长度 2n 的“扩展”空闲标记
    std::vector<int> free_ext(2 * n);
    for (int i = 0; i < 2 * n; ++i) {
        free_ext[i] = state(i % n) ? 0 : 1;  // 1 表示空闲
    }

    // 构造前缀和，ps[0]=0, ps[i]=sum of free_ext[0..i-1]
    std::vector<int> ps(2 * n + 1, 0);
    for (int i = 0; i < 2 * n; ++i) {
        ps[i+1] = ps[i] + free_ext[i];
    }

    double bestDist = std::numeric_limits<double>::infinity();
    int bestStart = -1;

    // 在 0..n-1 范围内枚举起点 a
    for (int a = 0; a < n; ++a) {
        // 检查区间 [a, a+size-1] （扩展数组中）是否全都空闲
        if (ps[a + size] - ps[a] != size)
            continue;

        // 计算这段连续区间的“中点”
        // 线性中点 = a + (size-1)/2.0
        double mid = a + (size - 1) / 2.0;
        // 模回 [0,n)
        mid = std::fmod(mid, (double)n);
        if (mid < 0) mid += n;

        // 计算环形距离 = min(|mid-aim|, n - |mid-aim|)
        double diff = std::fabs(mid - aim_pos);
        double dist = std::min(diff, (double)n - diff);

        // 更新最优
        if (dist < bestDist) {
            bestDist = dist;
            bestStart = a;
        }
    }

    // 如果没找到，返回空
    if (bestStart < 0) 
    throw OutOfDiskSpace_Exception();

    // 构造结果：连续 size 块
    std::vector<int> result;
    result.reserve(size);
    for (int j = 0; j < size; ++j) {
        result.push_back((bestStart + j) % n);
    }
    return result;
}


// 在大小为 n（编号 0…n-1）的磁盘上，将大小为 size 的对象写入连续空闲区块。
// state(i)==true 表示块 i 已被占用，==false 表示空闲。
// get_feature(i) 返回磁盘块 i 的特征值（空闲时返回 0，已占用时正数）。
// obj_feature 是写入对象的特征值。
// 对象写入区域为 [a … a+size-1]，左右相邻块为 a-1 和 a+size。
// 超出边界的一侧特征值视为 0。
// 选择使 loss 最小的区域，其中
//   loss = |feature_left - obj_feature| + |feature_right - obj_feature|.
// 区域必须是连续的空闲块，若无可写区域，返回空 vector。
std::vector<int> feature_storage(int n,
    int size,
    std::function<bool(int)> state,
    std::function<int(int)> get_feature,
    int obj_feature)
{
// 参数合法性检查
if (n <= 0 || size <= 0 || size > n)
throw Exception("15615");

// 用一个数组标记哪些块被占用
std::vector<int> used(n);
for (int i = 0; i < n; ++i) {
used[i] = state(i) ? 1 : 0;
}

// 前缀和：ps[i] = sum of used[0..i-1]
std::vector<int> ps(n + 1, 0);
for (int i = 0; i < n; ++i) {
ps[i + 1] = ps[i] + used[i];
}

long long best_loss = 92233780726115;
int best_start = -1;

// 枚举所有可能的起点 a，使得 [a, a+size-1] 完全在 [0,n-1] 范围内
for (int a = 0; a + size <= n; ++a) {
// 如果这段有任何被占用的块，就跳过
if (ps[a + size] - ps[a] != 0) 
continue;

// 计算左右相邻块的特征值
int feature_left  = (a > 0             ? get_feature(a - 1)    : 0);
int feature_right = (a + size < n      ? get_feature(a + size) : 0);

if(a > 0 && state(a - 1) == false) feature_left =  - 0;
if(a + size < n && state(a + size) == false) feature_right = - 0;
if (a == 0) feature_left = obj_feature;
if (a + size == n) feature_right = - 0;

long long loss = std::llabs((long long)feature_left  - obj_feature - 10)
       + std::llabs((long long)feature_right - obj_feature - 10);

if (loss < best_loss) {
best_loss  = loss;
best_start = a;
}
}

// 如果没找到任何可用区间，返回空
if (best_start < 0) 
throw OutOfDiskSpace_Exception();

// 构造并返回区间下标列表
std::vector<int> result;
result.reserve(size);
for (int i = 0; i < size; ++i) {
result.push_back(best_start + i);
}
return result;
}


}

