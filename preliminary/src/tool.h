#pragma once

#include <vector>
#include <algorithm>
#include <random>
namespace Tool {
    // 打乱vector中的元素
    template <typename T>
    void shuffleVector(std::vector<T>& vec) {
        // 使用随机数引擎
        std::random_device rd;                  // 获取随机数种子
        //std::mt19937 gen(rd());                 // Mersenne Twister 随机数引擎
        std::mt19937 gen(42);
        std::shuffle(vec.begin(), vec.end(), gen);  // 打乱 vector 中的元素
    }

    // 选择vector中评分最高的元素
    template <typename T>
    T& highestScoreChoice(std::vector<T>& vec, std::vector<double>& scores) {
        EXCEPTION_CHECK(vec.size() != scores.size(), "highestChoice两个参数的长度需相同。");
        EXCEPTION_CHECK(vec.empty(), "highestChoice: vector为空。");
        size_t bestIndex = 0;
        double bestScore = scores[0];
        for (size_t i = 1; i < scores.size(); ++i) {
            if (scores[i] > bestScore) {
                bestScore = scores[i];
                bestIndex = i;
            }
        }
        return vec[bestIndex];
    }


    // 生成随机矩阵
    std::vector<std::vector<double>> generateRandomMatrix(int rows, int cols) {
        // 初始化一个 rows x cols 的矩阵，所有元素初始为0.0
        std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols, 0.0));
    
        // 随机数引擎和均匀分布 [0, 1]
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
    
        // 填充矩阵
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                matrix[i][j] = dist(gen);
    
        return matrix;
    }


    std::vector<std::vector<double>> allocateProportions_normalization(const vector<double>& num, 
        const vector<vector<double>>& dis, const vector<double>& max) {
        // 输入检查
        if (max.empty()) throw std::runtime_error("allocateProportions: max不能为空");
        if (*std::min_element(max.begin(), max.end()) < 0) 
            throw std::runtime_error("allocateProportions: max元素不能小于0");
        if (num.size() != dis.size()) 
            throw std::runtime_error("allocateProportions: num和dis的长度不相等");

        size_t N = num.size();        // 待分配项个数
        size_t L = max.size();        // 分段数
        // 检查每个 dis[i] 的长度是否为 L
        for (size_t i = 0; i < N; ++i) {
            if (dis[i].size() != L) 
                throw std::runtime_error("allocateProportions: 每个dis[i]的长度必须与max相同");
        }

        // global_alloc[j] 表示各分段累计分配的总量（全局共享，不能超过max）
        vector<double> global_alloc(L, 0.0);
        // result[i][j] 表示第 i 个 num 分配到第 j 个分段的量
        vector<vector<double>> result(N, vector<double>(L, 0.0));

        // 依次处理每个 num[i]
        for (size_t i = 0; i < N; ++ i) {
            // 对 dis[i] 进行归一化处理：
            // 1. 平移：如果有负数，则将整个向量平移，使最小值为0
            // 2. 归一化：使所有元素之和为1（如果归一化后和为0，则设为均等权重）
            vector<double> norm(L, 0.0);
            double min_val = *std::min_element(dis[i].begin(), dis[i].end());
            if (min_val < 0) {
                for (size_t j = 0; j < L; ++j) {
                    norm[j] = dis[i][j] - min_val;
                }
            } else {
                norm = dis[i]; // 直接拷贝
            }
            double sum_norm = std::accumulate(norm.begin(), norm.end(), 0.0);
            if (sum_norm > 1e-12) {
                for (size_t j = 0; j < L; ++j) {
                    norm[j] /= sum_norm;
                }
            } else {
                // 如果所有归一化后都为0，则均分权重
                for (size_t j = 0; j < L; ++j) {
                    norm[j] = 1.0 / L;
                }
            }
            double remaining = num[i];  // 当前待分配的量
            // active[j] 表示第 j 个分段对于当前 num[i] 是否仍可分配（尚未达到上限）
            vector<bool> active(L, true);
            // curAlloc[j] 表示 num[i] 分配到第 j 个分段的量
            vector<double> curAlloc(L, 0.0);

            while (remaining > 1e-12) {
                double sum_active = 0.0;
                // 计算活跃分段的归一化权重之和
                for (size_t j = 0; j < L; ++j) {
                    if (active[j]) sum_active += norm[j];
                }
                if (sum_active <= 1e-12) break;

                bool anyCapped = false;
                // 本轮各分段分配的量（未累加到 curAlloc 和 global_alloc 前）
                vector<double> thisAlloc(L, 0.0);
                for (size_t j = 0; j < L; ++j) {
                    if (!active[j]) continue;
                    double alloc = remaining * norm[j] / sum_active;
                    double available = max[j] - global_alloc[j]; // 当前分段还可分配的容量
                    if (alloc > available + 1e-12) {
                        alloc = available;
                        active[j] = false;
                        anyCapped = true;
                    }
                    thisAlloc[j] = alloc;
                }
                // 累计本轮分配的量
                double sum_alloc = 0.0;
                for (size_t j = 0; j < L; ++j) {
                    global_alloc[j] += thisAlloc[j];
                    curAlloc[j] += thisAlloc[j];
                    sum_alloc += thisAlloc[j];
                }
                remaining -= sum_alloc;
                // 如果本轮所有活跃项均未被截断，则剩余量可一次性按比例分配，退出循环
                if (!anyCapped) break;
            }
            result[i] = curAlloc;
        }

        double allocated_total = std::accumulate(global_alloc.begin(), global_alloc.end(), 0.0);
        //std::cout << "总分配量 = " << allocated_total << std::endl;
        return result;
    }

    std::vector<std::vector<double>> allocateProportions(const std::vector<double>& num, const vector<vector<double>>& dis, 
        const vector<double>& max) {
        EXCEPTION_CHECK(max.empty(), "allocateProportions: max不能为空");
        EXCEPTION_CHECK(*min_element(max.begin(), max.end()) < 0, "allocateProportions: max元素不能小于0");
        for (auto& d: dis) EXCEPTION_CHECK(*min_element(d.begin(), d.end()) < 0, "allocateProportions: dis元素不能小于0");
        // 检查尺寸一致性
        if (num.size() != dis.size()) throw std::runtime_error("allocateProportions: num和dis的长度不相等");
        size_t N = num.size();        // 待分配项个数
        size_t L = max.size();        // 分段数
        // 检查每个 dis[i] 的长度是否为 L
        for (size_t i = 0; i < N; ++i) {
            if (dis[i].size() != L) throw std::runtime_error("allocateProportions: 每个dis[i]的长度必须与max相同");
        }
        // global_alloc[i]表示各分段累计分配的总量（全局共享，不能超过max）
        std::vector<double> global_alloc(L, 0.0);
        // result[i][j]表示第i个num分配到第j个分段的量
        std::vector<vector<double>> result(N, vector<double>(L, 0.0));

        // 依次处理每个 num[i]
        for (size_t i = 0; i < N; ++i) {
            double remaining = num[i];  // 当前待分配的量
            // active[j]表示第 j 个分段对当前 num[i]是否仍可分配（尚未达到上限）
            std::vector<bool> active(L, true);
            // 当前 num[i]的分配结果（每个分段的分配量）
            std::vector<double> curAlloc(L, 0.0);

            while (remaining > 1e-12) {
                double sum_active = 0.0;
                // 计算活跃分段的比例权重总和
                for (size_t j = 0; j < L; ++j) {
                    if (active[j])
                    sum_active += dis[i][j];
                }
                if (sum_active <= 1e-12) break;

                bool anyCapped = false;
                // 本轮各分段分配的量（未累加到curAlloc和global_alloc前）
                vector<double> thisAlloc(L, 0.0);
                for (size_t j = 0; j < L; ++j) {
                    if (!active[j]) continue;
                    double alloc = remaining * dis[i][j] / sum_active;
                    double available = max[j] - global_alloc[j]; // 当前分段还可分配的容量
                    if (alloc > available + 1e-12) {
                        alloc = available;
                        active[j] = false;
                        anyCapped = true;
                    }
                    thisAlloc[j] = alloc;
                }
                // 累计本轮分配的量
                double sum_alloc = 0.0;
                for (size_t j = 0; j < L; ++j) {
                    global_alloc[j] += thisAlloc[j];
                    curAlloc[j] += thisAlloc[j];
                    sum_alloc += thisAlloc[j];
                }
                remaining -= sum_alloc;
                // 如果本轮所有活跃项均未被截断，则剩余量可一次性按比例分配，退出循环
                if (!anyCapped) break;
            }
        result[i] = curAlloc;
        }

        double allocated_total = std::accumulate(global_alloc.begin(), global_alloc.end(), 0.0);
        //std::cout << "总分配量 = " << allocated_total << std::endl;
        return result;
    }


    

    std::vector<double> fun(double num, const std::vector<double>& dis, const std::vector<double>& max) {
        if (dis.size() != max.size()) {
            throw std::runtime_error("fun: dis 和 max 数组长度不相等");
        }
        size_t n = dis.size();
        
        double sum_max = std::accumulate(max.begin(), max.end(), 0.0);
        if (sum_max < num) {
            throw std::runtime_error("fun: 最大值数组总和小于待分配的总量");
        }
        
        // 初始化结果数组，所有项尚未固定
        std::vector<double> ans(n, 0.0);
        // 用于记录还未被固定分配的项的索引
        std::vector<int> active;
        for (size_t i = 0; i < n; ++i) {
            active.push_back(i);
        }
        
        double remaining = num;
        
        // 迭代分配，直到所有未固定项的分配均不超过其 max
        while (!active.empty()) {
            // 计算活跃项的比例总和
            double sum_active = 0.0;
            for (int idx : active) {
                sum_active += dis[idx];
            }
            bool anyCapped = false;
            std::vector<int> new_active;
            // 尝试按照比例分配剩余量
            for (int idx : active) {
                double alloc = remaining * dis[idx] / sum_active;
                if (alloc > max[idx] - ans[idx]) {
                    // 超过上限，则将该项固定为上限
                    double diff = max[idx] - ans[idx];
                    ans[idx] = max[idx];
                    remaining -= diff;
                    anyCapped = true;
                } else {
                    // 还未达到上限，保留该项待后续分配
                    new_active.push_back(idx);
                }
            }
            // 如果没有项被固定，说明剩余量按比例分配后均在上限内，直接分配完毕
            if (!anyCapped) {
                for (int idx : new_active) {
                    ans[idx] += remaining * dis[idx] / sum_active;
                }
                remaining = 0;
                break;
            }
            active = new_active;
            // 如果剩余量很小，退出循环
            if (remaining < 1e-12) break;
        }
        
        // 为了数值稳定性，调整最后的总和（理论上应该等于 num）
        double current_sum = std::accumulate(ans.begin(), ans.end(), 0.0);
        if (std::abs(current_sum - num) > 1e-9) {
            // 可以根据需要进行调整，或者抛出异常
            // 这里简单打印一个警告
            std::cerr << "Warning: 分配总和不精确: " << current_sum << " != " << num << std::endl;
        }
        
        return ans;
    }
    

    
    

    


}

