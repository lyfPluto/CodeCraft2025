#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <unordered_set>
#include <unordered_map>

#include "debug.h"
#include "wrong.h"

using namespace std;

// 节点保存区间内的最大值及其下标
struct Node {
    double maxVal;
    int idx;
};

class SegmentTree {
public:
    int n;      // 原始数组大小
    int base;   // 叶子节点起始下标（2的幂）
    vector<Node> tree; // 线段树数组，使用迭代实现，存储在 [1, 2*base-1]
    
    // 构造函数：基于数组 A 构建线段树
    SegmentTree(const vector<double>& A) {

        n = A.size();
        base = 1;
        while (base < n) {
            base *= 2;
        }

        // 分配大小为 2*base 的数组，初始值为 -∞ 和无效下标
        tree.assign(2 * base, { -std::numeric_limits<double>::infinity(), -1 });
        // 填写叶子节点：下标从 base 开始，叶子 i 对应 A[i] (0-indexed)
        for (int i = 0; i < n; i++) {
            tree[base + i] = { A[i], i };
        }

        // 若 A.size() < base, 剩下的叶子保持为 -∞
        // 迭代地构建内部节点
        for (int i = base - 1; i >= 1; i--) {
            tree[i] = merge(tree[2 * i], tree[2 * i + 1]);
        }

    }
    
    // 合并两个节点，返回较大值及其下标
    Node merge(const Node& leftNode, const Node& rightNode) {
        return (leftNode.maxVal >= rightNode.maxVal) ? leftNode : rightNode;
    }
    
    // 非递归查询区间 [ql, qr] (0-indexed)
    Node query(int ql, int qr) {
        if constexpr (!submit) {
            if (!(ql >= 0 && qr < n && ql <= qr)) {
                throw Exception("调用SegmentTree::query函数时参数异常。");
            }            
        }

        ql += base; // 转换到叶子节点下标
        qr += base;
        Node res = { -std::numeric_limits<double>::infinity(), -1 };
        while (ql <= qr) {
            if (ql % 2 == 1) {
                res = merge(res, tree[ql]);
                ql++;
            }
            if (qr % 2 == 0) {
                res = merge(res, tree[qr]);
                qr--;
            }
            ql /= 2;
            qr /= 2;
        }
        return res;
    }
    
    // 非递归单点更新，将下标 pos (0-indexed) 更新为 newVal
    void update(int pos, double newVal) {
        if constexpr (!submit) {
            if (!(pos >= 0 && pos < n)) {
                throw Exception("调用SegmentTree::update函数时参数异常。");
            }            
        }
        int idx = pos + base;
        tree[idx] = { newVal, pos };
        idx /= 2;
        while (idx >= 1) {
            tree[idx] = merge(tree[2 * idx], tree[2 * idx + 1]);
            idx /= 2;
        }
    }

    void update(int left, int right, double* newVal, int start) {
        return;
        //result << "call update:  " << left << " " << right << " " << start << newLine;
        if (left > right) {
            update(left, n - 1, newVal, 0);
            update(0, right, newVal, n - left);
        }
        int left_idx = left + base;
        int right_idx = right + base;
        int index = 0;
        for (int idx = left_idx; idx <= right_idx; idx ++) {
            tree[idx] = { newVal[index + start], left + index };
            index ++;
        }
        left_idx /= 2;
        right_idx /= 2;
        while (right_idx >= 1) {
            if (left_idx == 0) left_idx = 1;
            for (int i = left_idx; i <= right_idx; i ++) {
                tree[i] = merge(tree[2 * i], tree[2 * i + 1]);
            } 
            left_idx /= 2;
            right_idx /= 2;
        }
    }
private:


};

class RewardQuery {
public:
    int n;
    double stepCost;
    vector<double> profit;
    vector<double> excepedProfit;
    vector<double> A;     // A[i] = nums[i] - α * i
    vector<double> discount;
    double smoothDiscount;
    SegmentTree* seg;     // 线段树

    RewardQuery(int _n, double _stepCost, double _smoothDiscount)
    : n(_n), stepCost(_stepCost), smoothDiscount(_smoothDiscount), profit(_n, 0), excepedProfit(_n, 0)
{
    for (int i = 0; i < 20; i ++) {
        discount.push_back(pow(smoothDiscount, i));
    }
    A.resize(n);
    for (int i = 0; i < n; i++) {
        A[i] = - stepCost * i;
    }
    seg = new SegmentTree(A);

}


    RewardQuery(const vector<double>& nums, double _stepCost, double _smoothDiscount)
        : n(nums.size()), stepCost(_stepCost), smoothDiscount(_smoothDiscount)
        , profit(nums.size(), 0), excepedProfit(nums.size(), 0)
    {
        throw Exception("4444444444444");
        A.resize(n);
        for (int i = 0; i < n; i++) {
            A[i] = excepedProfit[i] - stepCost * i;
        }
        seg = new SegmentTree(A);
    }
    
    ~RewardQuery() {
        delete seg;
    }
    
    // 查询给定下标 i 时，找到最佳 j 使得 nums[j] - α * (j - i) 最大
    pair<int, double> query(int i) {
        profit_flush();

        if (i < 0 || i >= n) {
            throw Exception("调用RewardQuery::query函数时参数越界。");
        }
        
        // 查询 j >= i 的最大值
        //double base = A[i];
        double base = - stepCost * i;
        double candidate1 = -std::numeric_limits<double>::infinity();
        int j1 = -1;
        Node res1 = seg->query(i, n - 1);
        candidate1 = res1.maxVal - base;
        j1 = res1.idx;

        // 查询 j < i 的最大值
        double candidate2 = -std::numeric_limits<double>::infinity();
        int j2 = -1;
        if (i > 0) {
            Node res2 = seg->query(0, i - 1);
            candidate2 = res2.maxVal - base - stepCost * n;
            j2 = res2.idx;
        }
        // 返回最大值和下标
        if (candidate1 >= candidate2)
            //return make_pair(j1, candidate1);
            return make_pair(j1, excepedProfit[j1]);
        else
            //return make_pair(j2, candidate2);
            return make_pair(j2, excepedProfit[j2]);
    }
    

    std::unordered_map<int, double> reviseStore;
    void profit_flush() {
        if (reviseStore.empty()) return;
        std::unordered_set<int> revise;
        for (const auto& entry: reviseStore) {
            int pos = entry.first;
            double newVal = entry.second;
            double delta = newVal - profit[pos];
            profit[pos] = newVal;
            double temp[10];
            for (int i = 0; i < 10; i ++) {
                int index = (pos + n - i) % n;

                excepedProfit[index] += delta * discount[i];
                double newA = excepedProfit[index] - stepCost * (index);
                temp[9 - i] = newA;
            }
            seg -> update((pos + n - 9) % n, pos, temp, 0);

        }
        reviseStore.clear();

    }

    inline void set_profit(int pos, double newVal) {
        profit[pos] = newVal;
        return;
        //TimeCheck::start("set_profit");
        //TimeCheck::end("set_profit");
        reviseStore[pos] = newVal;
        
    }
    inline void add_profit(int pos, double delta) {
        set_profit(pos, profit[pos] + delta);
    }
    inline double get_profit(int index) {
        return profit[index];
        profit_flush();
        return profit[index];
    }

    inline double get_excepted_profit(int index) {
        profit_flush();
        return excepedProfit[index];
    }


    double get_step_cost() const {
        return stepCost;
    }
    void set_step_cost(double newVal) {
        stepCost = newVal;
    }



private:



};


