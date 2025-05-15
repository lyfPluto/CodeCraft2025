#pragma once

#include "functions.h"
#include "wrong.h"


namespace RecycleTool {

    std::vector<std::pair<int, int>> easy_recycle(int n, std::vector<int> tags, std::unordered_map<int, int> tag_aim, int swap_limit) {

        std::vector<std::pair<int, int>> result;
        std::vector<int> current_tags = tags;
        std::vector<bool> processed(n, false);
    
        // Step 1: Process mutual target swaps
        for (int i = 0; i < n; ++i) {
            if (processed[i] || current_tags[i] == 0) continue;
            int t_i = current_tags[i];
            auto it_i = tag_aim.find(t_i);
            if (it_i == tag_aim.end()) continue;
            int aim_i = it_i->second;
            if (aim_i < 0 || aim_i >= n || aim_i == i) continue;
            if (current_tags[aim_i] == 0) continue;
            int t_j = current_tags[aim_i];
            auto it_j = tag_aim.find(t_j);
            if (it_j == tag_aim.end()) continue;
            int aim_j = it_j->second;
            if (aim_j == i) {
                result.emplace_back(i, aim_i);
                std::swap(current_tags[i], current_tags[aim_i]);
                processed[i] = processed[aim_i] = true;
                if (--swap_limit == 0) return result;
            }
        }
    
        // Step 2: Move objects to their target if free
        for (int i = 0; i < n; ++i) {
            if (processed[i] || current_tags[i] == 0) continue;
            int t_i = current_tags[i];
            auto it_i = tag_aim.find(t_i);
            if (it_i == tag_aim.end()) continue;
            int aim_i = it_i->second;
            if (aim_i >= 0 && aim_i < n && current_tags[aim_i] == 0) {
                result.emplace_back(i, aim_i);
                std::swap(current_tags[i], current_tags[aim_i]);
                processed[aim_i] = true;
                if (--swap_limit == 0) return result;
            }
        }
    
        // Precompute initial losses
        std::vector<int> losses(n, 0);
        for (int i = 0; i < n; ++i) {
            if (current_tags[i] == 0 || processed[i]) continue;
            auto it = tag_aim.find(current_tags[i]);
            if (it != tag_aim.end()) {
                int aim = it->second;
                losses[i] = abs(i - aim);
            }
        }
    
        // Step 3: Greedy swap with max loss reduction (O(k·n))
        while (swap_limit > 0) {
            // Find current max loss object
            int max_i = -1, max_loss = -1;
            for (int i = 0; i < n; ++i) {
                if (processed[i] || current_tags[i] == 0) continue;
                if (losses[i] > max_loss) {
                    max_loss = losses[i];
                    max_i = i;
                }
            }
            if (max_i == -1 || max_loss <= 0) break;
    
            // Find best swap partner for max_i
            int best_j = -1, best_gain = 0;
            int t_i = current_tags[max_i];
            int aim_i = tag_aim[t_i];
            for (int j = 0; j < n; ++j) {
                if (j == max_i || processed[j] || current_tags[j] == 0) continue;
                int t_j = current_tags[j];
                auto it_j = tag_aim.find(t_j);
                if (it_j == tag_aim.end()) continue;
                int aim_j = it_j->second;
    
                // Calculate loss change
                int current_loss_j = losses[j];
                int new_loss_i = abs(j - aim_i);
                int new_loss_j = abs(max_i - aim_j);
                int gain = (losses[max_i] + current_loss_j) - (new_loss_i + new_loss_j);
    
                if (gain > best_gain) {
                    best_gain = gain;
                    best_j = j;
                }
            }
    
            if (best_j != -1 && best_gain > 0) {
                result.emplace_back(max_i, best_j);
                std::swap(current_tags[max_i], current_tags[best_j]);
    
                // Update losses and processed status
                auto update_loss = [&](int pos) {
                    if (current_tags[pos] == 0) {
                        losses[pos] = 0;
                    } else {
                        auto it = tag_aim.find(current_tags[pos]);
                        losses[pos] = (it != tag_aim.end()) ? abs(pos - it->second) : 0;
                    }
                    if (tag_aim.count(current_tags[pos]) && tag_aim[current_tags[pos]] == pos) {
                        processed[pos] = true;
                    }
                };
    
                update_loss(max_i);
                update_loss(best_j);
                swap_limit--;
            } else {
                break; // No beneficial swap found
            }
        }
    
        return result;
    }
    using namespace std;

    vector<pair<int, int>> improve_recycle(int n, int swap_limit, vector<int> tags, unordered_map<int, pair<int, int>> tag_ranges) {
        vector<pair<int, int>> result;
    
        // Phase 1: 预处理错误位置，处理空闲块
        unordered_map<int, queue<int>> wrong_positions;
        for (int i = 0; i < n; ++i) {
            if (tags[i] == 0) continue;
            int k = tags[i];
            auto& range = tag_ranges[k];
            if (i < range.first || i >= range.second) {
                wrong_positions[k].push(i);
            }
        }
    
        for (const auto& entry : tag_ranges) {
            int k = entry.first;
            int left = entry.second.first;
            int right = entry.second.second;
            for (int j = left; j < right; ++j) {
                if (tags[j] != 0) continue;
                auto& q = wrong_positions[k];
                if (!q.empty()) {
                    int i = q.front();
                    q.pop();
                    result.emplace_back(i, j);
                    swap(tags[i], tags[j]);
                    if (--swap_limit == 0) return result;
                }
            }
            if (swap_limit == 0) return result;
        }
    
        // Phase 2: 收集错误位置并排序，使用二分查找优化
        vector<int> errors;
        for (int i = 0; i < n; ++i) {
            if (tags[i] == 0) continue;
            int k = tags[i];
            auto& range = tag_ranges[k];
            if (i < range.first || i >= range.second) {
                errors.push_back(i);
            }
        }
        sort(errors.begin(), errors.end());
    
        unordered_map<int, pair<int, int>> error_range;
        for (int i : errors) {
            error_range[i] = tag_ranges[tags[i]];
        }
    
        for (size_t a = 0; a < errors.size() && swap_limit > 0; ++a) {
            int i = errors[a];
            if (tags[i] == 0) continue;
            auto [left_k, right_k] = error_range[i];
            auto lower = lower_bound(errors.begin(), errors.end(), left_k);
            auto upper = upper_bound(errors.begin(), errors.end(), right_k - 1);
            for (auto it = lower; it != upper; ++it) {
                if (it - errors.begin() <= a) continue;
                int j = *it;
                if (tags[j] == 0) continue;
                auto [left_m, right_m] = tag_ranges[tags[j]];
                if (i >= left_m && i < right_m) {
                    result.emplace_back(i, j);
                    swap(tags[i], tags[j]);
                    if (--swap_limit == 0) return result;
                    tags[i] = tags[j] = 0; // 标记为已处理
                    break;
                }
            }
        }
    
        // Phase 3: 预处理可用位置，处理剩余错误
        unordered_map<int, vector<int>> free_slots, wrong_slots;
        for (const auto& entry : tag_ranges) {
            int k = entry.first;
            int left = entry.second.first;
            int right = entry.second.second;
            vector<int> free, wrong;
            for (int j = left; j < right; ++j) {
                if (tags[j] == 0) free.push_back(j);
                else if (tags[j] != k) wrong.push_back(j);
            }
            free_slots[k] = free;
            wrong_slots[k] = wrong;
        }
    
        for (int i = 0; i < n && swap_limit > 0; ++i) {
            if (tags[i] == 0) continue;
            int k = tags[i];
            auto& range = tag_ranges[k];
            if (i >= range.first && i < range.second) continue;
    
            auto& free = free_slots[k];
            auto& wrong = wrong_slots[k];
            if (!free.empty()) {
                int j = free.back();
                free.pop_back();
                result.emplace_back(i, j);
                swap(tags[i], tags[j]);
                swap_limit--;
            } else if (!wrong.empty()) {
                int j = wrong.back();
                wrong.pop_back();
                result.emplace_back(i, j);
                swap(tags[i], tags[j]);
                swap_limit--;
            }
        }
    
        return result;
    }

}


class FirstRecycleSchedule {
public:
    virtual std::vector<std::pair<int, int>> getSchedule(int swap_limit, int n,
        const std::vector<std::tuple<int, int, int, double>>& objs) = 0;
};

class GreedyFirstRecycleSchedule: public FirstRecycleSchedule {
    std::vector<std::pair<int, int>> getSchedule(int swap_limit, int n,
        const std::vector<std::tuple<int, int, int, double>>& objs) override {
        // 1) 先构建初始状态
        struct Ball { int id, tag, box; double profit; };
        std::vector<Ball> B;
        B.reserve(objs.size());
        for (auto const &t : objs) {
            int id = std::get<0>(t),
                tag = std::get<1>(t) - 1,
                box= std::get<2>(t) - 1;
            double p = std::get<3>(t);
            EXCEPTION_CHECK(tag<0||tag>=n, "小球标签越界");
            EXCEPTION_CHECK(box<0||box>=n, "盒子编号越界");
            EXCEPTION_CHECK(p<0,       "小球利润不能小于0");
            B.push_back({id,tag,box,p});
        }
        // 盒子→当前小球列表
        std::vector<std::vector<int>> inBox(n);
        for (int i=0;i<(int)B.size();i++){
            inBox[B[i].box].push_back(i);
        }
        // 标记哪些已经在正确盒子（初始已获利）
        std::vector<bool> inPlace(B.size(),false);
        for(int i=0;i<(int)B.size();i++){
            if (B[i].box==B[i].tag) inPlace[i]=true;
        }

        // 2) 收集所有“还没获利”的小球索引，并按利润降序
        std::vector<int> mis;
        for(int i=0;i<(int)B.size();i++)
            if(!inPlace[i]) mis.push_back(i);
        std::sort(mis.begin(), mis.end(),
            [&](int a,int b){ return B[a].profit>B[b].profit; });

        std::vector<std::pair<int,int>> swaps;
        swaps.reserve(swap_limit);

        // 3) 对每个高利润的错位小球，尝试把它换到对应盒子
        for(int idx: mis) {
            if (swap_limit==0) break;
            auto &ball = B[idx];
            int targetBox = ball.tag;
            // 如果它已经放入正确盒子，则跳过
            if (inPlace[idx]) continue;
            // 如果它正好在正确盒子，跳过
            if (ball.box==targetBox) continue;

            // 在 targetBox 中选择一个“代罪羔羊” j：
            // 优先选 mis 放小球（inPlace[j]==false），这样不损失已有利润
            int j = -1;
            for(int cand : inBox[targetBox]){
                if (!inPlace[cand]) { j=cand; break; }
            }
            // 如果没有错位小球，只能牺牲一个已有利润小球
            if (j<0) {
                // 选利润最小的那一个
                double minP=1e300;
                for(int cand: inBox[targetBox]){
                    if (B[cand].profit<minP){
                        minP=B[cand].profit;
                        j=cand;
                    }
                }
            }
            if (j<0) continue; // 目标盒子竟然空了，跳过

            // 计算净收益 = profit[idx] - (inPlace[j]? B[j].profit:0)
            double loss = inPlace[j] ? B[j].profit : 0;
            double gain = ball.profit - loss;
            if (gain<=0) continue; // 不盈

            // 执行交换
            swaps.emplace_back(B[idx].id, B[j].id);
            swap_limit--;

            // 更新两球的盒子
            std::swap(ball.box, B[j].box);
            // 更新 inBox 列表
            auto removeFrom=[&](int bi,int i){
                auto &v=inBox[bi];
                v.erase(std::find(v.begin(),v.end(),i));
            };
            removeFrom(targetBox, j);
            removeFrom(B[j].box, idx);
            inBox[ball.box].push_back(idx);
            inBox[B[j].box].push_back(j);

            // 更新 inPlace
            inPlace[idx] = (ball.box==ball.tag);
            inPlace[j]   = (B[j].box==B[j].tag);
        }

        return swaps;
    }

};



class ImproveFirstRecycleSchedule: public FirstRecycleSchedule {
    std::vector<std::pair<int, int>> getSchedule(int swap_limit, int n,
        const std::vector<std::tuple<int, int, int, double>>& objs) override {

        std::vector<std::pair<int, int>> ans;
        std::vector<std::vector<std::priority_queue<std::pair<double, int>>>> state(
            n, std::vector<std::priority_queue<std::pair<double, int>>>(n)
        );
        for (const std::tuple<int, int, int, double>& obj: objs) {
            int id = std::get<0> (obj);
            int tag = std::get<1> (obj);
            int box = std::get<2> (obj);
            double profit = std::get<3> (obj);
            state[box][tag].push({profit, id});
        }
        for (int _ = 0; _ < swap_limit; ++ _) {
            double max_profit = 0;
            for (int box_index_1 = 0; box_index_1 < n; ++ box_index_1) {
                for (int box_index_2 = 0; box_index_2 < n; ++ box_index_2) {
                    if (box_index_1 == box_index_2) continue;
                    if (state[box_index_1][box_index_2].empty()) continue;
                    if (state[box_index_2][box_index_1].empty()) continue;
                    double profit = 0;
                    if (!state[box_index_1][box_index_2].empty()) {
                        profit += state[box_index_1][box_index_2].top().first;
                    }
                    if (!state[box_index_2][box_index_1].empty()) {
                        profit += state[box_index_2][box_index_1].top().first;
                    }
                    max_profit = std::max(profit, max_profit);
                }
            }
            if (max_profit == 0) break;
            bool success = false;
            for (int box_index_1 = 0; box_index_1 < n; ++ box_index_1) {
                for (int box_index_2 = 0; box_index_2 < n; ++ box_index_2) {
                    if (box_index_1 == box_index_2) continue;
                    if (state[box_index_1][box_index_2].empty()) continue;
                    if (state[box_index_2][box_index_1].empty()) continue;
                    int profit = 0;
                    if (!state[box_index_1][box_index_2].empty()) {
                        profit += state[box_index_1][box_index_2].top().first;
                    }
                    if (!state[box_index_2][box_index_1].empty()) {
                        profit += state[box_index_2][box_index_1].top().first;
                    }
                    if (profit == max_profit) {
                        std::pair<double, int> t1 = state[box_index_1][box_index_2].top();
                        std::pair<double, int> t2 = state[box_index_2][box_index_1].top();
                        state[box_index_1][box_index_2].pop();
                        state[box_index_2][box_index_1].pop();
                        int t = t1.second;
                        t1.second = t2.second;
                        t2.second = t;

                        state[box_index_1][box_index_1].push(t2);
                        state[box_index_2][box_index_2].push(t1);
                        ans.push_back({t1.second, t2.second});
                        success = true;
                    }
                    if (success) break;
                }
                if (success) break;
            }
        }
        return ans;
    }
};

