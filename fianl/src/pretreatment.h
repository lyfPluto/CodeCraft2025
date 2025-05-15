#pragma once

#include <vector>
#include <utility>
#include "debug.h"
#include "wrong.h"
#include "tool.h"
class DiskPretreatment {
public:
    DiskPretreatment(const std::vector<double> _delta, const std::vector<double> _length, const std::vector<double> _frequency):
        delta(_delta), length(_length), frequency(_frequency) {
            strategy = Tool::generateRandomMatrix(_delta.size(), _length.size());
        }
    DiskPretreatment(const DiskPretreatment& other) = delete;
    void load_strategy(const std::vector<std::vector<double>> _strategy) {
        strategy = _strategy;
    }
    double step(std::vector<std::vector<double>>& state) {
        int field_num = state.size();
        EXCEPTION_CHECK(field_num != length.size(), "DiskPretreatment::step: state长度错误");
        std::vector<double> remain(length);
        for (int field_index = 0; field_index < field_num; field_index ++) {
            for (const double& s: state[field_index]) remain[field_index] -= s;
        }
        std::vector<std::vector<double>> dis = Tool::allocateProportions(delta, strategy, remain);
        int tag_num = dis.size();
        for (int tag_index = 0; tag_index < tag_num; ++ tag_index) {
            for (int field_index = 0; field_index < field_num; ++ field_index) {
                state[field_index][tag_index] += dis[tag_index][field_index];
            }
        }
        return loss(state);
    }
    std::vector<std::vector<double>> get_next_state(const std::vector<std::vector<double>>& state) {
        std::vector<std::vector<double>> ans = state;
        int field_num = ans.size();
        EXCEPTION_CHECK(field_num != length.size(), "DiskPretreatment::step: state长度错误");
        std::vector<double> remain(length);
        for (int field_index = 0; field_index < field_num; field_index ++) {
            for (const double& s: ans[field_index]) remain[field_index] -= s;
        }
        std::vector<std::vector<double>> dis = Tool::allocateProportions(delta, strategy, remain);
        int tag_num = dis.size();
        for (int tag_index = 0; tag_index < tag_num; ++ tag_index) {
            for (int field_index = 0; field_index < field_num; ++ field_index) {
                ans[field_index][tag_index] += dis[tag_index][field_index];
            }
        }
        return ans;
    }
    std::vector<double*> strategy_flatten() {
        std::vector<double*> ans;
        for(auto& s: strategy) {
            for(double& num: s) ans.push_back(&num);
        }
        return ans;;
    }
    double loss(const std::vector<std::vector<double>>& state) const {
        double ans = 0;
        int tag_num = delta.size();
        int field_num = length.size();
        EXCEPTION_CHECK(state.size() != field_num, "Evalute::loss: 状态组长度与域数不相等");
        std::vector<double> state_tag_sum(tag_num, 0);
        for (auto& f: state) {
            EXCEPTION_CHECK(f.size() != tag_num, "Evalute::loss: 状态中标签数组长度与标签数不相等");
            for (int tag_index = 0; tag_index < tag_num; tag_index ++) {
                state_tag_sum[tag_index] += f[tag_index];
            }
        }
        for (int field_index = 0; field_index < field_num; field_index ++) {
            double f = 0;
            for (int tag_index = 0; tag_index < tag_num; tag_index ++) {
                f += state[field_index][tag_index] * frequency[tag_index];
            }
            ans += sqrt(f * length[field_index]);
        }
        ans = ans * ans;
        return ans;
    }
    const std::vector<double> delta; // size = tag_num
    const std::vector<double> length; // size = field_num
    const std::vector<double> frequency; // size = tag_num
    std::vector<std::vector<double>> strategy; // size = [tag_num, field_num]    
    
private:



};
