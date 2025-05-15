#pragma once

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include "wrong.h"
#include "object_base.h"




template<class T> struct ptr_less  // 比较器
{
    bool operator()(const T* lhs, const T* rhs) const
    {
        return lhs -> getId() < rhs -> getId(); 
    }
};
class DiskBase {
public:
    DiskBase(int _id, int _V, int _G): id(_id), size(_V), remain_empty(_V), token(_G), pos(0),
        obj_store(_V, nullptr), index_store(_V, -1), obj_pos() {};
    virtual void remove(ObjectBase* obj) {

        EXCEPTION_CHECK(obj_pos.find(obj) == obj_pos.end(), "要删除的对象不存在于磁盘中。");
        for(int del_index: obj_pos[obj]) {
            obj_store[del_index] = nullptr;
            index_store[del_index] = -1;
        }
        remain_empty += obj_pos[obj].size();
        obj_pos.erase(obj);
    }
    int getPos() { // 返回磁头位置
        return pos;
    }
    
    int remainEmpty() { // 磁盘剩余容量
        return remain_empty;
    }
    bool objExist(ObjectBase* obj) { // 对象是否存在磁盘上
        return obj_pos.find(obj) != obj_pos.end();
    }
    std::pair<ObjectBase*, int> getObjblock(int index) const {
        return std::make_pair(obj_store[index], index_store[index]);
    }
    std::pair<ObjectBase*, int> getObjblock() {
        return std::make_pair(obj_store[pos], index_store[pos]);
    }
    ObjectBase* getObj(int index) {
        return obj_store[index];
    }
    ObjectBase* getObj() {
        return obj_store[pos];
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


    virtual void pass() {
        read_out.pass(id);
        pos = (pos + 1) % size;
    }
    virtual void read() {
        read_out.read(id);
        if (obj_store[pos]) obj_store[pos] -> read_notify(index_store[pos]);
        pos = (pos + 1) % size;
    }

    virtual void move(int index) {
        EXCEPTION_CHECK(index < 0 || index >= size, "磁头移动超出范围呀。")
        read_out.jump(id, index);
        pos = index;
    }
    const std::vector<int>& get_obj_pos(ObjectBase* obj) {
        EXCEPTION_CHECK(obj_pos.find(obj) == obj_pos.end(), "对象不存在于磁盘中。");
        return obj_pos[obj];
    }
    inline bool isEmpty(int index) {
        return obj_store[index] == nullptr;
    }
    inline bool isEmpty() {
        return obj_store[pos] == nullptr;
    }
    const int id;
    const int size; // 磁道数量
    const int token; // 令牌数量
    std::vector<ObjectBase*> obj_store;
private:

    int remain_empty;
    int pos; // 当前磁头的位置

    std::vector<int> index_store;
    //std::map<ObjectBase*, std::vector<int>, ptr_less<ObjectBase>> obj_pos; // 存储对象的磁块的映射关系，方便根据对象查找磁块
    std::unordered_map<ObjectBase*, std::vector<int>> obj_pos;
};


