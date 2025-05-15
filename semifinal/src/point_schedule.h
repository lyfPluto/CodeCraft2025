#pragma once

#include "functions.h"
#include "debug.h"

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

//std::vector<bool> PointSchedule(int N, int G, const std::vector<bool>& request, int step) {
std::vector<bool> pointSchedule(int G, std::function<bool(int)> request, int step, int preCost) {
    std::priority_queue<State> q;
    if (preCost == 1) preCost = 80;
    // 初始状态：磁头位置head_pos，令牌数G
    State initialState = {G, preCost, 0};
    q.push(initialState);
    // 存储最终的动作序列
    //std::vector<bool> result;
    actions* result;
    int pos = 0;
    bool pre_is_empty = false;
    while (true) {
        std::priority_queue<State> next_q;
        //std::set<State> visited;
        int count = 0;
        bool isRequest = request(pos);
        int minTimeStep = q.top().timeStep;
        int minToken = q.top().tokens;
        int minPreCost = std::numeric_limits<int>::max();
        while (!q.empty()) {

            State currentState = q.top();
            q.pop();
            if (currentState.timeStep > minTimeStep) break;
            if (currentState.preCost >= minPreCost) continue;
            ++ count;
            
            minToken = std::min(minToken, currentState.tokens);
            minPreCost = std::min(minPreCost, currentState.preCost);
            result = currentState.opes;
            
            if (pre_is_empty && currentState.preCost != 80) {
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
                }
                continue;
            }

            if ((!isRequest) && currentState.tokens >= 1) {
                // Pass：忽略当前存储单元，磁头挪动到下一个存储单元
                
                State passState = currentState;
                bool pass_only = passState.preCost == 80;
                passState.tokens -= 1;
                passState.preCost = 80;
                if (passState.timeStep == 0) {
                    //passState.actions.push_back(false);
                    passState.opes = new actions({false, passState.opes});
                }
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
        
        if (q.empty()) {
            std::vector<bool> ans;
            while (result != nullptr) {
                ans.push_back(result -> ope);
                result = result -> next;
            }
            std::reverse(ans.begin(), ans.end());
            return ans;
        }
        if (count == 1 && q.top().timeStep > 0) {
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



/*
struct State {
    int pos;      // 当前磁头位置
    int tokens;   // 剩余令牌数
    int preCost; // 上次读取消耗的令牌数
    int timeStep; // 当前时间片编号
    std::vector<bool> actions; // 动作序列（true表示Read，false表示Pass）
};

//std::vector<bool> PointSchedule(int N, int G, const std::vector<bool>& request, int step) {
std::vector<bool> pointSchedule(int G, std::function<bool(int)> request, int step, int preCost) {
    std::queue<State> q;
    //std::vector<std::vector<bool>> visited(N, std::vector<bool>(G, false)); // 记录已访问的状态
    auto comp = [](const State& a, const State& b) {
        if (a.pos != b.pos) return a.pos < b.pos;
        if (a.tokens != b.tokens) return a.tokens < b.tokens;
        if (a.preCost != b.preCost) return a.preCost < b.preCost;
        if (a.timeStep != b.timeStep) return a.timeStep < b.timeStep;
        return false;
    };
    std::set<State, decltype(comp)> visited(comp);
    // 初始状态：磁头位置head_pos，令牌数G
    State initialState = {0, G, preCost, 0, {}};
    q.push(initialState);
    
    int farthest = -1;
    // 存储最终的动作序列
    std::vector<bool> result;
    
    // BFS
    while (!q.empty()) {
        State currentState = q.front();
        q.pop();
        
        
        // 两种可能的动作：Pass 或 Read
        if ((!request(currentState.pos)) && currentState.tokens >= 1) {
            // Pass：忽略当前存储单元，磁头挪动到下一个存储单元
            State passState = currentState;
            passState.pos ++;
            passState.tokens -= 1;
            
            passState.preCost = 1;
            if (passState.timeStep == 0) {
                passState.actions.push_back(false);
            }
            if (visited.find(passState) == visited.end()) {
                visited.insert(passState);
                q.push(passState);
            }
        }


        int token_need;
        if (currentState.preCost == 1) {
            token_need = 64;
        } else {
            token_need = std::max(16, static_cast<int>(std::ceil(currentState.preCost * 0.8)));
        }
        if (currentState.tokens >= token_need) {
                State readState = currentState;
                readState.pos ++;
                readState.tokens -= token_need;
                readState.preCost = token_need;
                if (readState.timeStep == 0) {
                    readState.actions.push_back(true);
                }
                if (visited.find(readState) == visited.end()) {
                    visited.insert(readState);
                    q.push(readState);
                }      
        }


        State skipState = currentState;
        skipState.timeStep++;
        skipState.tokens = G;
        if (visited.find(skipState) == visited.end()) {
            visited.insert(skipState);
            if (skipState.timeStep < step) {
                q.push(skipState);
            } else {
                if (skipState.pos > farthest) {
                    farthest = skipState.pos;
                    result = skipState.actions;
                }
                
            }
        }
    }

    return result;
}
*/