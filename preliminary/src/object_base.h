#pragma once

#include <vector>
#include <queue>

#include <map>
#include "debug.h"
#include "wrong.h"
#include "io.h"
#include "time_check.h"
Debug objLog("object_log");



class ObjectBase {
public:
ObjectBase(int _id, int _tag, int _size): id(_id), tag(_tag), size(_size), req_num(0), readCount(_size, 0), undone_read(_size) {}
    int getSize() {
        return size;
    }
    int getId() const {
        return id;
    }

    virtual void read_notify(int index) {
        if constexpr (!submit) {
            if(index >= size) throw Exception("调用Object::read_notify函数出错，读取的位置不存在。");
            if(index < 0) throw Exception("调用Object::read_notify函数出错，index不能为负数。");            
        }


        opeQueue.push({false, index, epoch});
        readCount[index] ++;
        if (readCount[index] == 1) undone_read --;
        if (undone_read != 0) return;
        while (!opeQueue.empty()) {
            if (std::get<0>(opeQueue.front())) {
                -- req_num;
                read_out.read_complete(std::get<1>(opeQueue.front()));
                opeQueue.pop();
            } else {
                readCount[std::get<1>(opeQueue.front())] --;
                if (readCount[std::get<1>(opeQueue.front())] == 0) {
                    undone_read ++;
                    opeQueue.pop();
                    return;
                }
                opeQueue.pop();
            }
        }
    }

    virtual void add_request(int request_id) {
        ++ req_num;
        opeQueue.push({true, request_id, epoch});
    }
    std::vector<int> get_undone_request() {
        std::vector<int> ans;
        std::queue<std::tuple<bool, int, int>> copy = opeQueue;
        while (!copy.empty()) {
            if(std::get<0>(copy.front())) ans.push_back(std::get<1>(copy.front()));
            copy.pop();
        }
        EXCEPTION_CHECK(ans.size() != req_num, "req_num数值异常");
        for (int discard_request_id: undone_request) {
            ans.push_back(discard_request_id);
        }
        
        return ans;
    }

    const int id;
    const int tag;
    const int size;

protected:

    inline int get_undone_request_num() {
        while (!opeQueue.empty()) {
            if (std::get<0>(opeQueue.front())) {
                if (epoch - std::get<2>(opeQueue.front()) < 105) return req_num;
                undone_request.push_back(std::get<1>(opeQueue.front()) );
                -- req_num;
                opeQueue.pop();
            }
            else {
                readCount[std::get<1>(opeQueue.front())] --;
                if (readCount[std::get<1>(opeQueue.front())] == 0) {
                    undone_read ++;
                }
                opeQueue.pop();
            }
        }
        return req_num;
    }



private:

    int req_num;    
    std::queue<std::tuple<bool, int, int>> opeQueue; // true表示读请求，false表示读通知。
    std::vector<int> readCount;
    std::vector<int> undone_request;
    int undone_read;




};