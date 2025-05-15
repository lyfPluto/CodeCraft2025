#pragma once
#include "disk.h"
#include "field_group.h"


void field::save(ObjectBase* obj) {
    disk -> save(obj, this);
}
int field::get_end()  {
    return (start + size) % ( disk -> size);
}
int field::get_benefit_begin() const {
    EXCEPTION_CHECK(benefit_index.empty(), "benefit_index为空，无法获取起始位置。");
    return (start + *benefit_index.begin() + benefit_index_delta) % disk -> size;
}
int field::get_benefit_end() const {
    EXCEPTION_CHECK(benefit_index.empty(), "benefit_index为空，无法获取起始位置。");
    return (start + *benefit_index.rbegin() + benefit_index_delta) % disk -> size;

}
int field::benefit_size() const{
    if (benefit_index.empty()) return 0;
    return (*benefit_index.rbegin() - *benefit_index.begin() + disk -> size) % disk -> size + 1;
}
void field::add_profit_index(int index) {
    EXCEPTION_CHECK(
        benefit_index.find(((index + disk -> size - start) % disk -> size) - benefit_index_delta) != benefit_index.end(),
        "add_profit_index: 要插入的索引已存在");
    benefit_index.insert(((index + disk -> size - start) % disk -> size) - benefit_index_delta);
}
void field::erase_profit_index(int index) {
    EXCEPTION_CHECK(
        benefit_index.find(((index + disk -> size - start) % disk -> size) - benefit_index_delta) == benefit_index.end(),
        "erase_profit_index: 要删除的索引不存在");
    benefit_index.erase(((index + disk -> size - start) % disk -> size) - benefit_index_delta);
}
void field::advanceStart() {
    start = (start + disk -> size - 1) % (disk -> size);
    benefit_index_delta ++;
    ++ size;
}
void field::retreatEnd() {
    if (size <= 1) {


        debug << disk -> debug_state();

        debug << "epoch: " << GlobalVariable::epoch << newLine;
        debug << "start: " << start << newLine;
        debug << "size: " << size << newLine;
        debug << "tag: " << tag << newLine;
        debug << "\n\n\n\n\n\n\n\n\n";
    }
    
    EXCEPTION_CHECK(size <= 1, "retreatEnd: 域大小不能为空");
    if (benefit_index.find(size - benefit_index_delta) != benefit_index.end()) {
        benefit_index.erase(size - benefit_index_delta);
    }
    -- size;
}

void field::set_request_permit(bool permit) {
    if (permit != request_permit) {
        int disk_size = disk -> size;
        for (int index = 0; index < size; index ++) {
            ObjectBase* obj = disk -> getObj((index + start) % disk_size);
            if (obj != nullptr) obj -> set_request_permit(permit);
        }
    }
    request_permit = permit;
}

bool field::lock(int holder) {
    return group_belong -> lock(holder);
}
void field::unlock(int holder) {
    group_belong -> unlock(holder);
}
bool field::is_lock() const {
    return group_belong -> is_lock();
}
double field::get_inferiority() const {
    return group_belong -> get_inferiority();
}

