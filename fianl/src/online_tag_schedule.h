#pragma once

#include "functions.h"
#include "debug.h"
#include "wrong.h"
#include "tool.h"
#include "parameters.h"




class OnlineTagSchedule {
public:
    virtual std::vector<std::pair<int, int>>  getSchedule() = 0;
};



class EasyOnlineTagSchedule: public OnlineTagSchedule {
public:
    EasyOnlineTagSchedule() {
        subscript_convert = [](int time) -> int {
            return (time - 1) / 100;
        };
        size = 1 + TagPredict::subscript_convert(GlobalInfo::T + EXTRA_TIME);
        for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
            tag_count[tag] = new FenwickTree(size);
            _tag_count[tag] = 0;
            req_count[tag] = std::vector<int> (size, 0);
            req_frequency[tag] = std::vector<double> (size, 0);
        }
        older_time = -1;
    }
    void obj_create(int obj_id, int obj_tag) {
        EXCEPTION_CHECK(obj_tag < 0 || obj_tag > GlobalInfo::M, "EasyOnlineTagSchedule::obj_create: 标签越界");
        if (obj_tag == 0) {
            EXCEPTION_CHECK(zero_tag_obj.find(obj_id) != zero_tag_obj.end(), 
                "EasyOnlineTagSchedule::obj_create: 对象已存在");
            zero_tag_obj[obj_id] = std::vector<int> (size, 0);
            zero_tag_obj_score[obj_id] = std::unordered_map<int, double> ();
            std::unordered_map<int, double>& temp = zero_tag_obj_score[obj_id];
            for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                temp[tag] = 0;
            }
        } else {
            tag_count[obj_tag] -> update(subscript_convert(GlobalVariable::epoch), 1);
            _tag_count[obj_tag] ++;
        }
    
    }
    void obj_delete(int obj_id, int obj_tag) {
        EXCEPTION_CHECK(obj_tag < 0 || obj_tag > GlobalInfo::M, "EasyOnlineTagSchedule::obj_create: 标签越界");
        if (obj_tag == 0) {
            zero_tag_obj.erase(obj_id);
            zero_tag_obj_score.erase(obj_id);
        } else {
            tag_count[obj_tag] -> update(subscript_convert(GlobalVariable::epoch), -1);
            _tag_count[obj_tag] --;
        }
    }
    void add_request(int obj_id, int obj_tag) {
        if (obj_tag == 0) {
            EXCEPTION_CHECK(zero_tag_obj.find(obj_id) == zero_tag_obj.end(), "EasyOnlineTagSchedule::add_request: 对象id不存在");
            ++ zero_tag_obj[obj_id][subscript_convert(GlobalVariable::epoch)];
        } else {
            req_frequency[obj_tag][subscript_convert(GlobalVariable::epoch)] += 1.0 / _tag_count[obj_tag];
            ++ req_count[obj_tag][subscript_convert(GlobalVariable::epoch)];
        }
    }


    std::vector<std::pair<int, int>> getSchedule() override {
        int now_time = subscript_convert(GlobalVariable::epoch);
        while (older_time < now_time - 1) {
            ++ older_time;
            for (auto& entry: zero_tag_obj) {
                int obj_id = entry.first;
                std::vector<int>& obj_req_count = entry.second;
                for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                    /*
                    zero_tag_obj_score[obj_id][tag] += (obj_req_count[older_time] - 
                        (double) req_count[tag][older_time] / tag_count[tag] -> query(older_time)) * 
                        (obj_req_count[older_time] - 
                        (double) req_count[tag][older_time] / tag_count[tag] -> query(older_time));
                    */
                    zero_tag_obj_score[obj_id][tag] += (obj_req_count[older_time] - 
                        req_frequency[tag][older_time]) * 
                        (obj_req_count[older_time] - 
                            req_frequency[tag][older_time]);
                }
            }
        }
        std::vector<std::pair<int, int>> ans;
        for (auto& entry: zero_tag_obj) {
            int obj_id = entry.first;
            
            std::vector<int>& obj_req_count = entry.second;
            std::vector<double> scores;
            for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                double score = 0;
                score = zero_tag_obj_score[obj_id][tag];

                //score += (obj_req_count[now_time] - (double) req_count[tag][now_time] / tag_count[tag] -> query(now_time)) * 
                //    (obj_req_count[now_time] - (double) req_count[tag][now_time] / tag_count[tag] -> query(now_time));
                score += (obj_req_count[now_time] - 
                    req_frequency[tag][now_time]) * 
                    (obj_req_count[now_time] - 
                        req_frequency[tag][now_time]);
                /*
                for (int t = 0; t <= now_time; ++ t) {
                    score += (obj_req_count[t] - req_count[tag][t]) * (obj_req_count[t] - req_count[tag][t]);
                }*/
                scores.push_back(-score);
            }
            int tag_choice = Tool::highestScoreChoice(scores) + 1;
            ans.push_back({obj_id, tag_choice});
        }
        return ans;
    }
private:

    std::function<int(int)> subscript_convert;
    std::unordered_map<int, std::vector<int>> req_count;
    std::unordered_map<int, FenwickTree*> tag_count;
    std::unordered_map<int, int> _tag_count;
    std::unordered_map<int, std::vector<double>> req_frequency;
    std::unordered_map<int, std::vector<int>> zero_tag_obj;
    // 保存每个对象的每个标签的现有分数
    std::unordered_map<int, std::unordered_map<int, double>> zero_tag_obj_score;
    int older_time;

    int size;
};






