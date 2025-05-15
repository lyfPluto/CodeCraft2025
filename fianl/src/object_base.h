#pragma once

#include <vector>
#include <queue>

#include <map>
#include "debug.h"
#include "wrong.h"
#include "io.h"
#include "time_check.h"
#include "data_collector.h"





class ObjectBase {
public:
ObjectBase(int _id, int _tag, int _size): id(_id), tag(_tag), size(_size), req_num(0), readCount(_size, 0), undone_read(_size) {
    request_permit = true;
}
    virtual ~ObjectBase() {
        std::queue<std::tuple<bool, int, int>> copy = opeQueue;
        while (!opeQueue.empty()) {
            if(std::get<0>(opeQueue.front()) && !GlobalVariable::read_complete[std::get<1>(opeQueue.front())]) {
                request_delete.push_back(std::get<1>(opeQueue.front()));
                GlobalVariable::read_complete[std::get<1>(opeQueue.front())] = true;
                DataCollector::request_complete(std::get<1>(opeQueue.front()));
            }
            
            opeQueue.pop();
        }

    }

    int getSize() {
        return size;
    }
    int getId() const {
        return id;
    }

    virtual void read_notify(int index) {
        EXCEPTION_CHECK(index < 0 || index >= size, "ObjectBase::read_notify: 索引越界");
        opeQueue.push({false, index, GlobalVariable::epoch});
        readCount[index] ++;
        if (readCount[index] == 1) undone_read --;
        if (undone_read != 0) return;
        while (!opeQueue.empty()) {
            if (std::get<0>(opeQueue.front())) {
                -- req_num;
                if (!GlobalVariable::read_complete[std::get<1>(opeQueue.front())]) {
                    read_complete.push_back(std::get<1>(opeQueue.front()));
                    GlobalVariable::read_complete[std::get<1>(opeQueue.front())] = true;
                    DataCollector::request_complete(std::get<1>(opeQueue.front()));
                }

                
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
        if (!request_permit) {
            read_busy.push_back(request_id);
            GlobalVariable::read_complete[request_id] = true;
            return;
        }
        ++ req_num;
        opeQueue.push({true, request_id, GlobalVariable::epoch});
    }
    bool get_request_permit() {
        return request_permit;
    }
    void set_request_permit(const bool& permit) {
        request_permit = permit;
    }

    const int id;
    const int tag;
    const int size;
    

protected:

    inline int get_undone_request_num() {
        while (!opeQueue.empty()) {
            if (std::get<0>(opeQueue.front())) {
                if (GlobalVariable::epoch - std::get<2>(opeQueue.front()) < 105) return req_num;
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
    int undone_read;
    bool request_permit;




};