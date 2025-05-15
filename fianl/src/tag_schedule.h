#pragma once

#include "functions.h"
#include "debug.h"
#include "wrong.h"
#include "tool.h"

namespace __PredictTool {
/// 计算给定 sample 属于每个标签 tag 的对数似然，并返回对数似然最高的标签
int log_likelihood_estimation(const std::unordered_map<int, std::vector<double>>& tag_dis, const std::vector<double>& sample) {
    int best_tag = -1;
    double best_score = -std::numeric_limits<double>::infinity();
    constexpr double EPSILON = 1e-10;
    // 对每个标签
    for (const auto& kv : tag_dis) {
        int tag = kv.first;
        const std::vector<double>& lambdas = kv.second;

        // 要求每个标签的 lambda 数量与 sample 大小一致
        if (lambdas.size() != sample.size()) {
            continue; // 或者抛出异常，取决于需求
        }

        double score = 0.0;
        bool valid = true;

        // 计算对数似然： sum_i [ x_i*log(lambda_i) - lambda_i - log(x_i!) ]
        for (size_t i = 0; i < sample.size(); ++i) {
            int x = static_cast<int>(sample[i]);
            double lambda = lambdas[i];

            // 替换 λ 接近零的值
            if (lambda < EPSILON) {
                lambda = EPSILON; // 使用一个非常小的值替代 λ=0
            }

            // x*log(lambda)
            score += x * std::log(lambda);
            // -lambda
            score -= lambda;
            // -log(x!)，使用 lgamma(x+1) 计算 log(x!)
            score -= std::lgamma(x + 1.0);
        }

        // 更新最优标签
        if (score > best_score) {
            best_score = score;
            best_tag = tag;
        }
    }

    return best_tag;
}

    
}



class TagSchedule {
public:
    virtual std::vector<std::pair<int, int>> getSchedule(int obj_num, std::function<int(int)> obj_tags, 
        std::function<std::vector<int>(int)> obj_req_time, 
        std::function<int(int)> obj_create_teme, std::function<int(int)> obj_delete_time) = 0;
};


/**
 * 每次选择请求数量的余弦相似度最大的标签
 */
class GreedyTagSchedule: public TagSchedule{
public:
    std::vector<std::pair<int, int>> 
    getSchedule(int obj_num, std::function<int(int)> obj_tags, std::function<std::vector<int>(int)> obj_req_time, 
    std::function<int(int)> obj_create_teme, std::function<int(int)> obj_delete_time) override {
        std::vector<std::pair<int, int>> ans;
        std::unordered_map<int, std::vector<int>> req_count;

        int size = 1 + TagPredict::subscript_convert(GlobalInfo::T + EXTRA_TIME);
        for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
            req_count[tag] = std::vector<int> (size, 0);
        }
        for (int obj_id = 1; obj_id <= obj_num; ++ obj_id) {
            int obj_tag = obj_tags(obj_id);
            if (obj_tag == 0) continue;
            for (const int& req_time: obj_req_time(obj_id)) {
                req_count[obj_tag][TagPredict::subscript_convert(req_time)] ++;
            }
        }

        for (int obj_id = 1; obj_id <= obj_num; ++ obj_id) {
            int obj_tag = obj_tags(obj_id);
            if (obj_tag != 0) continue;
            std::vector<int> req_time = obj_req_time(obj_id);
            std::vector<int> obj_req_count(size, 0);
            for (int rt: req_time) {
                obj_req_count[TagPredict::subscript_convert(rt)] ++;
            }
            std::vector<double> scores;
            for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                scores.push_back(Tool::cosine_similarity(obj_req_count, req_count[tag]));
                //scores.push_back(Tool::dotProduct(obj_req_count, req_count[tag]));
            }
            int tag_choice = Tool::highestScoreChoice(scores) + 1;
            ans.push_back({obj_id, tag_choice});
        }

        return ans;

    }

};


/**
 * 使用对数似然估计所属标签
 */
class LogLikelihoodTagSchedule: public TagSchedule{
    public:
        std::vector<std::pair<int, int>> 
        getSchedule(int obj_num, std::function<int(int)> obj_tags, std::function<std::vector<int>(int)> obj_req_time, 
        std::function<int(int)> obj_create_teme, std::function<int(int)> obj_delete_time) override {
            std::vector<std::pair<int, int>> ans;
            std::unordered_map<int, std::vector<double>> req_count;
            int size = 1 + TagPredict::subscript_convert(GlobalInfo::T + EXTRA_TIME);
            for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                req_count[tag] = std::vector<double> (size, 0);
            }
            for (int obj_id = 1; obj_id <= obj_num; ++ obj_id) {
                int obj_tag = obj_tags(obj_id);
                if (obj_tag == 0) continue;
                for (const int& req_time: obj_req_time(obj_id)) {
                    req_count[obj_tag][TagPredict::subscript_convert(req_time)] += 1;
                }
            }
    
            for (int obj_id = 1; obj_id <= obj_num; ++ obj_id) {
                int obj_tag = obj_tags(obj_id);
                if (obj_tag != 0) continue;
                std::vector<int> req_time = obj_req_time(obj_id);
                std::vector<double> obj_req_count(size, 0);
                for (int rt: req_time) {
                    obj_req_count[TagPredict::subscript_convert(rt)] += 1;
                }

                int tag_choice = __PredictTool::log_likelihood_estimation(req_count, obj_req_count);

                EXCEPTION_CHECK(tag_choice == -1, "__PredictTool::log_likelihood_estimation: tag_choice值异常");
                ans.push_back({obj_id, tag_choice});
            }
    
            return ans;
    
        }
    
    };



class ImproveGreedyTagSchedule: public TagSchedule{
    public:
        std::vector<std::pair<int, int>> 
        getSchedule(int obj_num, std::function<int(int)> obj_tags, std::function<std::vector<int>(int)> obj_req_time, 
        std::function<int(int)> obj_create_time, std::function<int(int)> obj_delete_time) override {
            std::vector<std::pair<int, int>> ans;
            std::unordered_map<int, std::vector<double>> req_count;
            int size = 1 + TagPredict::subscript_convert(GlobalInfo::T + EXTRA_TIME);
            for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                req_count[tag] = std::vector<double> (size, 0);
            }
            for (int obj_id = 1; obj_id <= obj_num; ++ obj_id) {
                int obj_tag = obj_tags(obj_id);
                if (obj_tag == 0) continue;
                for (const int& req_time: obj_req_time(obj_id)) {
                    req_count[obj_tag][TagPredict::subscript_convert(req_time)] 
                        += 1.0 / TagPredict::tag_obj_holding[obj_tag] -> query(req_time);
                }
            }
            for (int obj_id = 1; obj_id <= obj_num; ++ obj_id) {
                int obj_tag = obj_tags(obj_id);
                if (obj_tag != 0) continue;
                int create_time = TagPredict::subscript_convert(obj_create_time(obj_id));
                int delete_time = TagPredict::subscript_convert(obj_delete_time(obj_id));
                std::vector<int> req_time = obj_req_time(obj_id);
                std::vector<double> obj_req_count(size, 0);
                for (int rt: req_time) {
                    obj_req_count[TagPredict::subscript_convert(rt)] += 1;
                }
                std::vector<double> scores;
                for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                    double score = 0;
                    for (int t = create_time; t < delete_time; ++ t) {
                        score += (obj_req_count[t] - req_count[tag][t]) * (obj_req_count[t] - req_count[tag][t]);
                    }
                    scores.push_back(-score);
                    //scores.push_back(Tool::cosine_similarity(obj_req_count, req_count[tag]));
                }
                int tag_choice = Tool::highestScoreChoice(scores) + 1;
                ans.push_back({obj_id, tag_choice});
            }
    
            return ans;
    
        }
    
    };



