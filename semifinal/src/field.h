#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <set>

#include "object_base.h"
#include "functions.h"




class field {
    public:
        field(int _tag, int _start, int _size, Disk* _disk): tag(_tag) {
            start = _start;
            size = _size;
            remainEmpty = _size;
            profit = 0;
            disk = _disk;
            benefit_index_delta = 0;
            request_permit = true;
            group_belong = nullptr;
        }
        void addProfit(double delta) {
            profit += delta;
        }
        double getProfit() const {
            return profit;
        }
        void save(ObjectBase* obj);
        void advanceStart();
        void retreatEnd();
    
        const int tag;
        void add_profit_index(int index);
        void erase_profit_index(int index);
        int benefit_size() const;
        int benefit_count() const {
            return benefit_index.size();
        };
        int get_benefit_begin() const;
        int get_benefit_end() const;
        int get_remain_empty() const {
            return remainEmpty;
        }
        void add_remain_empty(int delta) {
            remainEmpty += delta;
        }
        int get_start() {
            return start;
        }
        int get_size() {
            return size;
        }
        int get_end();
        Disk* get_disk() const {
            return disk;
        }
        bool get_request_permit() {
            return request_permit;
        }
        void set_request_permit(bool permit);
        int get_benefit_count() {
            return benefit_index.size();
        }
        void add_group(FieldGroup* group) {
            EXCEPTION_CHECK(group_belong != nullptr, "field::add_group: 重复给域添加域组");
            group_belong = group;
        }
        bool lock(int holder);
        void unlock(int holder);
        bool is_lock() const;
        double get_inferiority() const;
        friend std::string InfoView::field_view(field* f);
    private:
        bool request_permit;
        std::set<int> benefit_index;
        double profit;
        int remainEmpty;    
        int start;
        int size;
        Disk* disk;
        int benefit_index_delta;


        FieldGroup* group_belong;

    };