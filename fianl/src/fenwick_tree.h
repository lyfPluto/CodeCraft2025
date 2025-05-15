#pragma once

#include <vector>


class FenwickTree {
    private:
        std::vector<int> tree;
    public:
        // 构造函数：初始化大小为n（外部0-based，内部1-based，数组大小为n+1）
        FenwickTree(int n) : tree(n + 1, 0) {}
    
        // 区间修改：将位置pos（0-based）及之后的所有元素加上val
        void update(int pos, int val) {
            pos++; // 转为1-based索引
            for (; pos < tree.size(); pos += pos & -pos) {
                tree[pos] += val;
            }
        }
    
        // 单点查询：返回位置x（0-based）的值
        int query(int x) {
            x++; // 转为1-based索引
            int sum = 0;
            for (; x > 0; x -= x & -x) {
                sum += tree[x];
            }
            return sum;
        }
    };
    