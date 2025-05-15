#pragma once

#include <vector>
#include <algorithm>
#include <random>
#include <sstream>

#include "functions.h"

namespace Tool {
    template <typename T, typename Container>
    T* highestScoreChoice(Container& container, const std::function<double(const T*)>& getScore, double minScore);
    template <typename T, typename Container>
    T* highestScoreChoice(Container& container, const std::function<double(const T*)>& getScore);
    std::string opeSimplify(const std::string& input);
    template <typename T>
    std::string toString(const std::vector<T>& vec);
    template <typename T, std::size_t N>
    std::string toString(const std::array<T, N>& arr);
    template<typename T>
    T& getRandomElement(std::vector<T>& vec);
    template <typename T>
    void shuffleVector(std::vector<T>& vec);
    // 选择vector中评分最高的元素
    template <typename T>
    T& highestScoreChoice(std::vector<T>& vec, std::vector<double>& scores);
    // 获取评分最高的下标
    int highestScoreChoice(const std::vector<double>& scores);
    template <typename T>
    void sortByScore(std::vector<T>& vec, const std::vector<double>& scores);
    // 生成随机矩阵
    std::vector<std::vector<double>> generateRandomMatrix(int rows, int cols);

    std::vector<double> fun(double num, const std::vector<double>& dis, const std::vector<double>& max);
};


namespace Tool {
    template <typename T, typename Container>
    T* highestScoreChoice(Container& container, const std::function<double(const T*)>& getScore, double minScore) {
        if (container.empty()) {
            throw std::runtime_error("highestScoreChoice: container is empty.");
        }
    
        T* best = nullptr; // 用于存储分数最高的指针
        double bestScore = minScore - 0.000000001; // 初始化为最低允许分数
    
        // 遍历容器的所有元素并找出分数最高且满足条件的元素
        for (T* obj : container) {
            double score = getScore(obj);
            if (score >= minScore && score > bestScore) {
                bestScore = score;
                best = obj;
            }
        }
    
        // 如果没有找到符合条件的元素，返回 nullptr
        return best;
    }
    

    template <typename T, typename Container>
    T* highestScoreChoice(Container& container, const std::function<double(const T*)>& getScore) {
        T* best = nullptr;
        double bestScore = -std::numeric_limits<double>::infinity();  // 初始为最小分数
    
        for (T* obj : container) {
            double score = getScore(obj);
            if (std::isnan(score)) continue;  // 跳过 NaN 分数
            if (best == nullptr || score > bestScore) {
                bestScore = score;
                best = obj;
            }
        }
    
        return best; // 如果没有找到有效分数，将返回 nullptr
    }
    std::string opeSimplify(const std::string& input) {
        std::string result;
        size_t i = 0;
        while (i < input.size()) {
            char c = input[i];
            // 如果字符为 'p' 或 'r'，进行合并处理
            if (c == 'p' || c == 'r') {
                char letter = c;
                size_t count = 0;
                // 统计连续相同字符的个数
                while (i < input.size() && input[i] == letter) {
                    ++count;
                    ++i;
                }
                // 如果连续个数大于 1，则输出数字+字符，否则直接输出字符
                if (count > 1)
                    result += std::to_string(count) + letter;
                else
                    result.push_back(letter);
            } else {
                // 对于非 'p' 或 'r' 字符，直接输出
                result.push_back(c);
                ++i;
            }
        }
        return result;
    }

    template <typename T>
    std::string toString(const std::vector<T>& vec) {
        std::stringstream ss;
        ss << "[ ";
        for (const auto& element : vec) {
            ss << element << " ";
        }
        ss << "]";
        return ss.str();
    }

    template <typename T, std::size_t N>
    std::string toString(const std::array<T, N>& arr) {
        std::stringstream ss;
        ss << "[ ";
        for (const auto& element : arr) {
            ss << element << " ";
        }
        ss << "]";
        return ss.str();
    }

    template<typename T>
    T& getRandomElement(std::vector<T>& vec) {
        EXCEPTION_CHECK(vec.empty(), "Tool::getRandomElement: 数组不能为空");
        if (vec.empty()) {
            throw std::runtime_error("Vector is empty.");
        }
        // 建议使用静态变量来保持随机数引擎的状态（防止每次调用都重新种子）
        static std::random_device rd;
        static std::mt19937 gen(rand());
        std::uniform_int_distribution<> dis(0, vec.size() - 1);
        int randomIndex = dis(gen);
        return vec[randomIndex];
    }

    // 打乱vector中的元素
    template <typename T>
    void shuffleVector(std::vector<T>& vec) {
        // 使用随机数引擎
        std::random_device rd;                  // 获取随机数种子
        //std::mt19937 gen(rd());                 // Mersenne Twister 随机数引擎
        std::mt19937 gen(rand());
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
    // 获取评分最高的下标
    int highestScoreChoice(const std::vector<double>& scores) {
        EXCEPTION_CHECK(scores.empty(), "highestScoreChoice: vector为空。");
        int bestIndex = 0;
        double bestScore = scores[0];
        for (size_t i = 1; i < scores.size(); ++i) {
            if (scores[i] > bestScore) {
                bestScore = scores[i];
                bestIndex = i;
            }
        }
        return bestIndex;
    }
    // 获取评分最高的下标
    int highestScoreChoice(int size, std::function<int(int)> scores) {
        EXCEPTION_CHECK(size <= 0, "highestScoreChoice: size 不能为空。");
        int bestIndex = 0;
        int bestScore = scores(0);
        for (int i = 1; i < size; ++i) {
            int score = scores(i);
            if (score > bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    
    template <typename T>
    void sortByScore(std::vector<T>& vec, const std::vector<double>& scores) {
        EXCEPTION_CHECK(vec.size() != scores.size(), "sortByScore两个参数的长度需相同。");
        EXCEPTION_CHECK(vec.empty(), "sortByScore: vector为空。");
        // 创建索引数组，并按分数从大到小对索引进行排序
        std::vector<size_t> indices(vec.size());
        for (size_t i = 0; i < vec.size(); ++i) {
            indices[i] = i;
        }
        // 按照scores对应的分数排序索引数组
        std::sort(indices.begin(), indices.end(), [&scores](size_t i, size_t j) {
            return scores[i] > scores[j];  // 分数较大排前面
        });
        // 根据排序后的索引重新排列 vec
        std::vector<T> sortedVec(vec.size());
        for (size_t i = 0; i < vec.size(); ++i) {
            sortedVec[i] = vec[indices[i]];
        }
        vec = std::move(sortedVec);  // 更新原vec
    }

    

    // 生成随机矩阵
    std::vector<std::vector<double>> generateRandomMatrix(int rows, int cols) {
        // 初始化一个 rows x cols 的矩阵，所有元素初始为0.0
        std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols, 0.0));
    
        // 随机数引擎和均匀分布 [0, 1]
        std::random_device rd;
        std::mt19937 gen(rand());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
    
        // 填充矩阵
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                matrix[i][j] = dist(gen);
    
        return matrix;
    }

    void softmax(std::vector<double>& nums) {
        if (nums.empty()) return;
    
        // 找出 nums 中的最大值，用于数值稳定性处理
        double max_val = *std::max_element(nums.begin(), nums.end());
        
        // 计算每个元素的 exp(x - max_val)
        double sum = 0.0;
        for (auto& x : nums) {
            x = std::exp(x - max_val);
            sum += x;
        }
        // 归一化
        for (auto& x : nums) {
            x /= sum;
        }
    }
    
    
    std::vector<std::vector<double>> allocateProportions(const std::vector<double>& nums, 
        std::vector<std::vector<double>> dis, const std::vector<double>& max) {
        
        EXCEPTION_CHECK(std::accumulate(nums.begin(), nums.end(), 0) > 
            std::accumulate(max.begin(), max.end(), 0), 
            "Tool::allocateProportions: nums元素和不能超过max元素和");
        EXCEPTION_CHECK(*std::min_element(nums.begin(), nums.end()) < 0, "Tool::allocateProportions: nums中的元素不能小于0");
        EXCEPTION_CHECK(nums.size() != dis.size(), "Tool::allocateProportions: nums和dis数组长度不相等");
        EXCEPTION_CHECK(nums.empty(), "Tool::allocateProportions: nums不能为空");
        EXCEPTION_CHECK(dis[0].size() != max.size(), "Tool::allocateProportions: max与dis[_]数组长度不相等");
        
        for (std::vector<double>& d: dis) softmax(d);
        int n = max.size();
        int m = nums.size();
        std::vector<double> gradient(n, 0);
        for (int num_index = 0; num_index < m; ++ num_index) {
            for (int gra_index = 0; gra_index < n; ++ gra_index) {
                gradient[gra_index] += nums[num_index] * dis[num_index][gra_index];
            }
        }
        double num_sum = 0;
        for (const double& num: nums) {
            num_sum +=num;
        }
        double scale = 1;
        while (true) {
            std::vector<std::vector<double>> ans(n, std::vector<double> (m, 0));
            std::vector<double> sum(n, 0);
            for (int num_index = 0; num_index < m; ++ num_index) {
                const std::vector<double>& num_dis = dis[num_index];
                for (int dis_index = 0; dis_index < n; ++ dis_index) {
                    sum[dis_index] += scale * nums[num_index] * num_dis[dis_index];
                    ans[dis_index][num_index] += scale * nums[num_index] * num_dis[dis_index];
                }
            }
            double all_sum = 0;
            for (int index = 0; index < n; ++ index) {
                if (sum[index] >= max[index]) {
                    all_sum += max[index];
                    gradient[index] = 0;
                } else {
                    all_sum += sum[index];
                }
            };
            if (all_sum >= num_sum - 1e-10) {
                for (int index = 0; index < n; ++ index) {
                    if (sum[index] > max[index]) {
                        for (int num_index = 0; num_index < m; num_index ++) {
                            ans[index][num_index] *= max[index] / sum[index];
                        }
                    }
                };
                return ans;
            }
            scale += (num_sum - all_sum) / std::accumulate(gradient.begin(), gradient.end(), 0.0);
        }
    };


    


    

    
    
    double cosine_similarity(std::vector<double> v1, std::vector<double> v2) {
        // 检查两个向量是否具有相同长度
        if (v1.size() != v2.size()) {
            throw std::invalid_argument("Vectors must have the same length.");
        }
        
        double dot = 0.0;      // 内积
        double norm1 = 0.0;    // 向量v1的平方和
        double norm2 = 0.0;    // 向量v2的平方和
        
        // 计算内积和两个向量的平方和
        for (size_t i = 0; i < v1.size(); i++) {
            dot += v1[i] * v2[i];
            norm1 += v1[i] * v1[i];
            norm2 += v2[i] * v2[i];
        }
        
        double mag1 = std::sqrt(norm1);
        double mag2 = std::sqrt(norm2);
        
        // 如果任意一个向量为零向量，则无法计算余弦相似度
        if (mag1 == 0.0 || mag2 == 0.0) {
            return 0;
            //throw std::invalid_argument("Cannot compute cosine similarity for zero vector.");
        }
        
        return dot / (mag1 * mag2);
    }

    double cosine_similarity(std::vector<int> v1, std::vector<int> v2) {
        // 检查两个向量是否具有相同长度
        if (v1.size() != v2.size()) {
            throw std::invalid_argument("Vectors must have the same length.");
        }
        
        double dot = 0.0;      // 内积
        double norm1 = 0.0;    // v1 的平方和
        double norm2 = 0.0;    // v2 的平方和
    
        // 计算内积和平方和时，将 int 显式转换为 double
        for (size_t i = 0; i < v1.size(); ++i) {
            dot += static_cast<double>(v1[i]) * static_cast<double>(v2[i]);
            norm1 += static_cast<double>(v1[i]) * static_cast<double>(v1[i]);
            norm2 += static_cast<double>(v2[i]) * static_cast<double>(v2[i]);
        }
    
        double mag1 = std::sqrt(norm1);
        double mag2 = std::sqrt(norm2);
    
        // 如果任一向量为零向量，则无法计算余弦相似度
        if (mag1 == 0.0 || mag2 == 0.0) {
            return 0;
            //throw std::invalid_argument("Cannot compute cosine similarity for zero vector.");
        }
    
        return dot / (mag1 * mag2);
    }

    // 返回两向量内积
    long long dotProduct(const std::vector<int>& a, const std::vector<int>& b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("dotProduct: 两个向量长度不一致");
        }
        long long sum = 0;
        for (size_t i = 0; i < a.size(); ++i) {
            sum += a[i] * b[i];
        }
        return sum;
    }
    // 返回两向量内积
    double dotProduct(const std::vector<double>& a, const std::vector<double>& b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("dotProduct: 两个向量长度不一致");
        }
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            sum += a[i] * b[i];
        }
        return sum;
    }
    


}

