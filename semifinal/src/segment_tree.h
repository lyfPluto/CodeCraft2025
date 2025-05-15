#pragma once


#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

template<typename T>
class SegmentTree {
public:
    // 构造函数：构造区间 [l, r]
    SegmentTree(int l, int r)
        : left(l), right(r), mid((l + r) / 2),
            left_child(nullptr), right_child(nullptr),
            sum(0), minVal(0), maxVal(0), val(0) {}

    // 区间求和
    T getSum(int l, int r) {
        if (l == left && r == right) return sum;
        T ans = val * static_cast<T>(r - l + 1);
        if (left_child != nullptr) {
            if (r <= mid)
                ans += left_child -> getSum(l, r);
            else if (l <= mid)
                ans += left_child -> getSum(l, mid);
        }
        if (right_child != nullptr) {
            if (l >= mid + 1)
                ans += right_child -> getSum(l, r);
            else if (r >= mid + 1)
                ans += right_child -> getSum(mid + 1, r);
        }
        return ans;
    }

    // 区间最小值
    T getMin(int l, int r) {
        if (l == left && r == right)
            return minVal;
        if (r <= mid) {
            if (left_child != nullptr)
                return val + left_child -> getMin(l, r);
            else
                return val;
        }
        if (l >= mid + 1) {
            if (right_child != nullptr)
                return val + right_child -> getMin(l, r);
            else
                return val;
        }
        T left_min = (left_child == nullptr ? val : val + left_child->getMin(l, mid));
        T right_min = (right_child == nullptr ? val : val + right_child->getMin(mid + 1, r));
        return std::min(left_min, right_min);
    }

    // 区间最大值
    T getMax(int l, int r) {
        if (l == left && r == right)
            return maxVal;
        if (r <= mid) {
            if (left_child != nullptr)
                return val + left_child -> getMax(l, r);
            else
                return val;
        }
        if (l >= mid + 1) {
            if (right_child != nullptr)
                return val + right_child -> getMax(l, r);
            else
                return val;
        }
        T left_max = (left_child == nullptr ? val : val + left_child -> getMax(l, mid));
        T right_max = (right_child == nullptr ? val : val + right_child -> getMax(mid + 1, r));
        return std::max(left_max, right_max);
    }



    // add: 给单个位置增加值 v
    void add(int index, T v) {
        add(index, index, v);
    }

    // add: 给区间 [l, r] 增加值 v
    void add(int l, int r, T v) {
        if (l == left && r == right) {
            val += v;
            reset();
            return;
        }
        if (r <= mid) {
            if (left_child == nullptr)
                left_child = new SegmentTree<T>(left, mid);
            left_child->add(l, r, v);
            reset();
            return;
        }
        if (l >= mid + 1) {
            if (right_child == nullptr)
                right_child = new SegmentTree<T>(mid + 1, right);
            right_child->add(l, r, v);
            reset();
            return;
        }
        if (left_child == nullptr)
            left_child = new SegmentTree<T>(left, mid);
        if (right_child == nullptr)
            right_child = new SegmentTree<T>(mid + 1, right);
        left_child->add(l, mid, v);
        right_child->add(mid + 1, r, v);
        reset();
    }

    // set: 设置单个位置为 v
    void set(int index, T v) {
        set(index, index, v);
    }

    // set: 设置区间 [l, r] 的值为 v，返回修改前后的差值（总变化量）
    T set(int l, int r, T v) {
        if (l == left && r == right) {
            T diff = static_cast<T>(r - l + 1) * v - sum;
            val = v;
            if (left_child != nullptr) { delete left_child; left_child = nullptr; }
            if (right_child != nullptr) { delete right_child; right_child = nullptr; }
            reset();
            return diff;
        }
        if (r <= mid) {
            if (left_child == nullptr)
                left_child = new SegmentTree<T>(left, mid);
            T diff = left_child->set(l, r, v - val);
            reset();
            return diff;
        }
        if (l >= mid + 1) {
            if (right_child == nullptr)
                right_child = new SegmentTree<T>(mid + 1, right);
            T diff = right_child->set(l, r, v - val);
            reset();
            return diff;
        }
        if (left_child == nullptr)
            left_child = new SegmentTree<T>(left, mid);
        if (right_child == nullptr)
            right_child = new SegmentTree<T>(mid + 1, right);
        T diff = left_child->set(l, mid, v - val);
        diff += right_child->set(mid + 1, r, v - val);
        reset();
        return diff;
    }

    // getAll: 返回区间内所有叶子值，存储在 vector 中
    std::vector<T> getAll() {
        std::vector<T> ans;
        if (left == right) {
            ans.push_back(val);
            return ans;
        }
        if (left_child != nullptr) {
            std::vector<T> tmp = left_child -> getAll();
            for (auto num : tmp)
                ans.push_back(num + val);
        } else {
            for (int i = left; i <= mid; ++ i)
                ans.push_back(val);
        }
        if (right_child != nullptr) {
            std::vector<T> tmp = right_child -> getAll();
            for (auto num : tmp)
                ans.push_back(num + val);
        } else {
            for (int i = mid + 1; i <= right; ++i)
                ans.push_back(val);
        }
        return ans;
    }

    // toString: 返回线段树的描述信息
    std::string toString() {
        std::ostringstream oss;
        oss << "left: " << left << " right: " << right << " val: " << val 
            << " sum: " << sum << " max: " << maxVal << " min: " << minVal << "\n";
        if (left_child != nullptr)
            oss << left_child->toString();
        if (right_child != nullptr)
            oss << right_child->toString();
        return oss.str();
    }
private:
    int left, right, mid;
    SegmentTree<T>* left_child;
    SegmentTree<T>* right_child;
    T sum;   // 区间和
    T minVal;    // 区间最小值
    T maxVal;    // 区间最大值
    T val;   // 当前节点的加数（懒标记思想，这里直接存放该区间整体加的值）

    // 更新当前节点的数值：根据叶子情况及子节点信息更新 mn、mx、sum
    void reset() {
        if (left == right) {
            minVal = val;
            maxVal = val;
            sum = val;
            return;
        }
        if (left_child == nullptr) {
            minVal = val;
            maxVal = val;
            sum = val * static_cast<T>(mid - left + 1);
        } else {
            minVal = left_child -> getMin(left, mid) + val;
            maxVal = left_child -> getMax(left, mid) + val;
            sum = left_child -> getSum(left, mid) + val * static_cast<T>(mid - left + 1);
        }
        if (right_child != nullptr) {
            minVal = std::min(minVal, val + right_child -> getMin(mid + 1, right));
            maxVal = std::max(maxVal, val + right_child -> getMax(mid + 1, right));
            sum += right_child->getSum(mid + 1, right) + val * static_cast<T>(right - mid);
        } else {
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
            sum += val * static_cast<T>(right - mid);
        }
    }

};


