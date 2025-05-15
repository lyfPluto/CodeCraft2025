#pragma once
#include "object_base.h"
#include "disk.h"
#include "data_collector.h"
class Object;
std::unordered_set<Object*> profit_update;
class Object: public ObjectBase {
public:
    Object(int _id, int _tag, int _size): ObjectBase(_id, _tag, _size), req_count(_size), req_tem_store(0), profit(0)
        {}
    std::vector<Disk*> get_disks() {
        return disks;
    }
    ~Object() {
        for(Disk* disk: disks) {
            disk -> remove(this);
        }
    }
    void addDisk(Disk* disk) {
        disks.push_back(disk);

        
    }
    void add_request(int request_id) override {
        EXCEPTION_CHECK(disks.size() != 3, "给Object添加请求前未写入磁盘。");
        ObjectBase::add_request(request_id);
        DataCollector::request_create(request_id);
        if (!get_request_permit()) {
            DataCollector::request_complete(request_id);
        
            return;
        }
        profit_update.insert(this);
        
    }

    void read_notify(int index) override {
        EXCEPTION_CHECK(disks.size() != 3, "读取Object前未写入磁盘。");
        ObjectBase::read_notify(index); 
        profit_update.insert(this); 
    }

    void profit_flush() {
        if (get_undone_request_num() != profit) {
            profit = get_undone_request_num();
            for(Disk* disk: disks) {
                disk -> set_profit(this, profit);
            }   
        }   

    }
    int get_req_num(int index) {
        return req_count[index];
    }



private:
    std::vector<Disk*> disks;
    std::vector<int> req_count;
    int req_tem_store;
    int profit;

};