#pragma once

#include "functions.h"
#include "debug.h"
#include "wrong.h"


/**
 * 磁头调度策略
 */
class HeadSchedule {
public:
    virtual std::vector<bool> getSchedule(int G, std::function<bool(int)> request, int preCost) = 0;
};

class OriginHeadSchedule: public HeadSchedule {
public:
    std::vector<bool> getSchedule(int G, std::function<bool(int)> request, int preCost) override {

        std::function<int(int)> get_query_dis =  [](int pre_cost) -> int {
            int query_dis;
            switch(pre_cost) {
                case 1:
                    query_dis = 1;
                    break;
                case 64:
                    query_dis = 3;
                    break;
                case 52:
                    query_dis = 5;
                    break;
                case 42:
                    query_dis = 6;
                    break;
                case 34:
                    query_dis = 8;
                    break;
                case 28:
                    query_dis = 9;
                    break;
                case 23:
                    query_dis = 10;
                    break;
                case 19:
                    query_dis = 10;
                    break;
                case 16:
                    query_dis = 10;
                    break;
                default:
                    throw Exception("调用get_query_dis函数异常，priCost不在查询范围。");
            }
            return query_dis;
        };
        std::vector<bool> ans;
        if (G <= 0) {
            return ans;
        }
        
        int pos = 0;
        int remain = G;
        while (true) {
            bool pass_flag = true;
            int query_dis = get_query_dis(preCost);
            for (int i = 0; i < query_dis; i ++) {
                if (request(pos + i)) {
                    pass_flag = false;
                }
            }
            if ((preCost == 1) && (!request(pos))) pass_flag = true;
            if (pass_flag) {
                if (remain <= 0) {
                    return ans;
                }
                remain --;
                ans.push_back(false);
                pos ++;
                preCost = 1;
            } else {
                int need;
                if (preCost == 1) need = 64;
                else need = std::max(16, static_cast<int>(std::ceil(preCost * 0.8)));
                if (remain < need) {
                    return ans;
                }
                ans.push_back(true);
                remain -= need;       
                pos ++;       
                preCost = need;
            }
        }
        return ans;
    }

};



class BFSHeadSchedule: public HeadSchedule {
public:
    BFSHeadSchedule(int _step): step(_step) {}
    std::vector<bool> getSchedule(int G, std::function<bool(int)> request, int preCost) override {
        std::priority_queue<State> q;
        if (preCost == 1) preCost = 80;
        // 初始状态：磁头位置head_pos，令牌数G
        State initialState = {G, preCost, 0, nullptr};
        q.push(initialState);
        // 存储最终的动作序列
        //std::vector<bool> result;
        actions* result;
        int pos = 0;
        bool pre_is_empty = false;
        while (true) {
            std::priority_queue<State> next_q;
            const bool& isRequest = request(pos);
            int minTimeStep = q.top().timeStep;
            int stanrd_time = q.top().timeStep;
            int minPreCost = std::numeric_limits<int>::max();
            //debug << newLine << "pos:: " << pos << newLine;;
            while (!q.empty()) {
                const State currentState = q.top();
                if (currentState.timeStep != stanrd_time) {
                    stanrd_time = currentState.timeStep;
                    minPreCost = std::numeric_limits<int>::max();
                }
                q.pop();
                if (currentState.preCost >= minPreCost) continue;
                minPreCost = std::min(minPreCost, currentState.preCost);
                result = currentState.opes;
                //debug << currentState << newLine;
                if (pre_is_empty && (currentState.preCost != 80)) {
                    int token_need;
                    if (currentState.preCost == 1) {
                        token_need = 64;
                    } else {
                        token_need = std::max(16, static_cast<int>(std::ceil(currentState.preCost * 0.8)));
                    }
                    if (currentState.tokens >= token_need) {
                            State readState = currentState;
                            readState.tokens -= token_need;
                            readState.preCost = token_need;
                            if (readState.timeStep == 0) {
                                readState.opes = new actions({true, readState.opes});
                            }
                            next_q.push(readState);
                    }  else {
                        State skipState = currentState;
                        skipState.timeStep ++;
                        skipState.tokens = G;
                        if (skipState.timeStep < step) {
                            q.push(skipState);
                        }                    
                    }
                    continue;
                }
    
                if ((!isRequest) && (currentState.tokens >= 1)) {
                    // Pass：忽略当前存储单元，磁头挪动到下一个存储单元
                    //debug << "flag" << newLine;
                    State passState = currentState;
                    bool pass_only = (passState.preCost == 80);
                    passState.tokens -= 1;
                    passState.preCost = 80;
                    if (passState.timeStep == 0) {
                        passState.opes = new actions({false, passState.opes});
                    }
                    //debug << "passState: " << passState << newLine; 
                    next_q.push(passState);
                    if (pass_only) continue;
                }
    
        
    
                // 两种可能的动作：Pass 或 Read
                int token_need;
                if (currentState.preCost == 1) {
                    token_need = 64;
                } else {
                    token_need = std::max(16, static_cast<int>(std::ceil(currentState.preCost * 0.8)));
                }
                if (currentState.tokens >= token_need) {
                        State readState = currentState;
                        readState.tokens -= token_need;
                        readState.preCost = token_need;
                        if (readState.timeStep == 0) {
                            //readState.actions.push_back(true);
                            readState.opes = new actions({true, readState.opes});
                        }
                        next_q.push(readState);
        
                } else {
                    State skipState = currentState;
                    skipState.timeStep ++;
                    skipState.tokens = G;
                    if (skipState.timeStep < step) {
                        q.push(skipState);
                    }                    
                }
                
            }
            q = std::move(next_q);
            //q = next_q;
    
            if (q.empty()) {
                std::vector<bool> ans;
                while (result != nullptr) {
                    ans.push_back(result -> ope);
                    result = result -> next;
                }
                std::reverse(ans.begin(), ans.end());
                return ans;
            }
            if (q.size() == 1 && q.top().timeStep > 0) {
                std::vector<bool> ans;
                while (result != nullptr) {
                    ans.push_back(result -> ope);
                    result = result -> next;
                }
                std::reverse(ans.begin(), ans.end());
                return ans;
            }
            pos ++;
            pre_is_empty = !isRequest;
        }
    
        std::vector<bool> ans;
        while (result != nullptr) {
            ans.push_back(result -> ope);
            result = result -> next;
        }
        std::reverse(ans.begin(), ans.end());
        return ans;
    }
private:
    const int step;
    struct actions {
        bool ope;
        actions* next;
    };
    
    struct State {
        int tokens;   // 剩余令牌数
        int preCost; // 上次读取消耗的令牌数
        int timeStep; // 当前时间片编号
        actions* opes; // 动作序列（true表示Read，false表示Pass）    
        bool operator<(const State& other) const {
            if (timeStep != other.timeStep) return timeStep > other.timeStep;
            if (tokens != other.tokens) return tokens < other.tokens;
            if (preCost != other.preCost) return preCost > other.preCost;
            return false;
        }
    };
};



