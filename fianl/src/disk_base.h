#pragma once

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <unordered_map>

#include "wrong.h"
#include "object_base.h"

class DiskBase {
public:
    DiskBase(int _id, int _V, int _G): id(_id), size(_V), remain_empty(_V), token(_G),
        obj_store(_V, nullptr), index_store(_V, -1), obj_pos() {
            head_pos.fill(0);
        };
    virtual void remove(ObjectBase* obj) {

        EXCEPTION_CHECK(obj_pos.find(obj) == obj_pos.end(), "要删除的对象不存在于磁盘中。");
        for(int del_index: obj_pos[obj]) {
            obj_store[del_index] = nullptr;
            index_store[del_index] = -1;
        }
        remain_empty += obj_pos[obj].size();
        obj_pos.erase(obj);
    }
    int getHeadPos(int head_id) { // 返回磁头位置
        EXCEPTION_CHECK(head_id < 0 || head_id >= head_pos.size(), "DiskBase::getHeadPos: 磁头索引超出范围");
        return head_pos[head_id];
    }
    std::array<int, HyperParameters::head_num> getHeadPos() {
        return head_pos;
    }


    
    int remainEmpty() { // 磁盘剩余容量
        return remain_empty;
    }
    bool objExist(ObjectBase* obj) { // 对象是否存在磁盘上
        auto a = obj_pos.find(obj);
        auto b = obj_pos.end();
        //bool ans = obj_pos.find(obj) != obj_pos.end();
        bool ans = a != b;
        return ans;
    }
    std::pair<ObjectBase*, int> getObjblock(int index) const {
        return std::make_pair(obj_store[index], index_store[index]);
    }

    ObjectBase* getObj(int index) {
        return obj_store[index];
    }
    void save_obj(std::pair<ObjectBase*, int> obj_block, int index) {

        if constexpr (!submit) {
            if (index >= size || index < 0) {
                throw Exception("调用save_obj函数的索引越界。" + std::to_string(index));
            }
            if (obj_block.first == nullptr) {
                throw Exception("尝试保存空对象。");
            }
            if (obj_block.second < 0) {
                throw Exception("调用save_obj函数时对象标签异常。");
            }
            if (index_store[index] != -1) {
                throw Exception("无法在非空磁盘块中保存对象块。");
            }            
        }
        if(obj_pos.find(obj_block.first) == obj_pos.end()) {
            obj_pos.emplace(obj_block.first, std::vector<int> ());
        }
        obj_store[index] = obj_block.first;
        index_store[index] = obj_block.second;
        obj_pos.at(obj_block.first).push_back(index);
        -- remain_empty;
    }
    int getToken() {
        return token;
    }

    virtual void pass(int head_id) {
        EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "DiskBase::pass: 磁头索引超出范围");
        read_out.pass(id, head_id);
        head_pos[head_id] = (head_pos[head_id] + 1) % size;
    }
    virtual void read(int head_id) {

        EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "DiskBase::read: 磁头索引超出范围");
        read_out.read(id, head_id);
        if (obj_store[head_pos[head_id]]) obj_store[head_pos[head_id]] -> read_notify(index_store[head_pos[head_id]]);
        head_pos[head_id] = (head_pos[head_id] + 1) % size;
    }
    virtual void move(int index, int head_id) {
        EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "DiskBase::move: 磁头索引超出范围");
        EXCEPTION_CHECK(index < 0 || index >= size, "磁头移动超出范围。")
        read_out.jump(id, index, head_id);
        head_pos[head_id] = index;
    }


    const std::vector<int>& get_obj_pos(ObjectBase* obj) {
        EXCEPTION_CHECK(obj_pos.find(obj) == obj_pos.end(), "DiskBase::get_obj_pos: 对象不存在于磁盘中。");
        return obj_pos[obj];
    }
    inline bool isEmpty(int index) {
        return obj_store[index] == nullptr;
    }
    inline int get_head_pos(int head_id) {
        EXCEPTION_CHECK(head_id < 0 || head_id >= 2, "DiskBase::get_head_pos: head_id超出范围");
        return head_pos[head_id];
    }
    virtual void set_profit(ObjectBase* obj, double newProfit) = 0;

    const int id;
    const int size; // 磁道数量
    const int token; // 令牌数量
    
protected:
    virtual void swap(int index1, int index2) {
        EXCEPTION_CHECK(index1 < 0 || index1 >= size, "DiskBase::swap: index1索引越界");
        EXCEPTION_CHECK(index2 < 0 || index2 >= size, "DiskBase::swap: index2索引越界");
        EXCEPTION_CHECK(index1 == index2, "DiskBase::swap: 不能在两个相同位置swap");
        ObjectBase* obj_1 = obj_store[index1];
        ObjectBase* obj_2 = obj_store[index2];        
        obj_store[index1] = obj_2;
        obj_store[index2] = obj_1;
        if (obj_1 != obj_2) {
            if (obj_1 != nullptr) {
                std::vector<int>& obj_pos_1 = obj_pos[obj_1];
                EXCEPTION_CHECK(std::count(obj_pos_1.begin(), obj_pos_1.end(), index1) != 1, "DiskBase::swap: obj_pos_1异常");
                obj_pos_1.erase(std::remove(obj_pos_1.begin(), obj_pos_1.end(), index1), obj_pos_1.end());
                obj_pos_1.push_back(index2);
            }
            if (obj_2 != nullptr) {
                std::vector<int>& obj_pos_2 = obj_pos[obj_2];
                EXCEPTION_CHECK(std::count(obj_pos_2.begin(), obj_pos_2.end(), index2) != 1, "DiskBase::swap: obj_pos_2异常");
                obj_pos_2.erase(std::remove(obj_pos_2.begin(), obj_pos_2.end(), index2), obj_pos_2.end());
                obj_pos_2.push_back(index1);
            }            
        }

        int temp = index_store[index1];
        index_store[index1] = index_store[index2];
        index_store[index2] = temp;
        recycle_record[id].push_back({index1, index2});
    }

private:
    std::vector<ObjectBase*> obj_store;
    int remain_empty;
    std::array<int, 2> head_pos;
    std::vector<int> index_store;
    std::unordered_map<ObjectBase*, std::vector<int>> obj_pos;
};


