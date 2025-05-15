#pragma once

#include "disk.h"
#include "object.h"

class FieldGroup {
public:
    FieldGroup(const std::initializer_list<field*>& fs, int _tag): fields(3), tag(_tag) {
        EXCEPTION_CHECK(fs.size() != 3, "FieldGroup::FieldGroup: field个数应为3");
        fields.assign(fs.begin(), fs.end());
        request_permit = true;
        inferiority = 1;
        inferiority_increment = 1;
        for (field* f: fields) {
            if (f != nullptr) f -> add_group(this);
        }
        owner = -1;
        GlobalVariable::all_field_group.push_back(this);
        pre_lock_time = -1;
    }
    FieldGroup(const std::vector<field*>& fs, int _tag): fields(fs), tag(_tag) {
        EXCEPTION_CHECK(fs.size() != 3, "FieldGroup::FieldGroup: field个数应为3");
        request_permit = true;
        inferiority = 0;
        inferiority_increment = 1;
        for (field* f: fields) {
            if (f != nullptr) f -> add_group(this);
        }
        owner = -1;
        GlobalVariable::all_field_group.push_back(this);
        pre_lock_time = -1;
    }

    field* operator[] (const int& index) {
        EXCEPTION_CHECK(index < 0 || index >= fields.size(), "FieldGroup::operator[]: 索引越界");
        return fields[index];
    }

    void save(Object* obj);
    static void add_backup_field(field* f) {
        backup_field.push_back(f);
    }
    bool get_request_permit() {
        return request_permit;
    }
    void set_request_permit(const bool& permit) {
        if (permit != request_permit) {
            request_permit = permit;
            for (field* f: fields) {
                if (f != nullptr) f -> set_request_permit(permit);
            }
        }
    }
    double get_benefit_size() {
        int ans = 0;
        for (field* f: fields) {
            if (f != nullptr) ans += f -> benefit_size();
        }
        return ans;
    }
    double remainEmpty() {
        throw Exception("调用未完善的函数: FieldGroup::remainEmpty");
        double ans = 0;
        int count = 0;
        for (field* f: fields) {
            if (f != nullptr) {
                ++ count;
                ans += f -> get_remain_empty();
            }
        }
        if (count == 0) return 0;
        return ans / count;
    }
    bool lock(int holder) {
        EXCEPTION_CHECK(owner != -1, "FieldGroup::lock: 只有锁是被释放状态才可以进行加锁");
        const double scale = std::pow(2, 0.01);
        inferiority += std::pow(scale, GlobalVariable::epoch) * inferiority_increment;
        //inferiority += inferiority_increment;
        owner = holder;
        EXCEPTION_CHECK(pre_lock_time != -1, "FieldGroup::lock: pre_lock_time值异常");
        pre_lock_time = GlobalVariable::epoch;
        info_record = {benefit_size(), benefit_count(), get_profit()};
        return true;
    }
    void unlock(int holder) {
        EXCEPTION_CHECK(owner != holder, "FieldGroup::unlock: 只有该锁所有者可以进行解锁");
        owner = -1;
        EXCEPTION_CHECK(pre_lock_time == -1, "FieldGroup::unlock: pre_lock_time值异常");
        DataCollector::field_group_lock(pre_lock_time, holder, tag, 
            std::get<0>(info_record), std::get<1>(info_record), std::get<2>(info_record));
        pre_lock_time = -1;
    }
    bool is_lock() {
        return owner != -1;
    }
    double get_inferiority() const {
        return inferiority;
    }
    void set_inferiority(double val) {
        inferiority = val;
    }
    void scale_inferiority_increment(double mulple) {
        inferiority_increment *= mulple;
    }
    double get_inferiority_increment() {
        return inferiority_increment;
    }
    void set_inferiority_increment(double val) {
        inferiority_increment = val;
    }
    double get_profit() {
        double ans = -1;
        for (field* f: fields) {
            if (f != nullptr) {
                double profit = f -> getProfit();
                EXCEPTION_CHECK(ans != -1 && ans != profit, "FieldGroup::get_profit: profit校验失败");
                ans = profit;
            }
        }
        EXCEPTION_CHECK(ans == -1, "FieldGroup::get_profit: profit获取失败");
        return ans;
    }
    int benefit_size() {
        std::vector<int> ans;
        for (field* f: fields) {
            if (f != nullptr) {
                int size = f -> benefit_size();
                ans.push_back(size);
                //EXCEPTION_CHECK(ans != -1 && ans != size, "FieldGroup::benefit_size: benefit_size校验失败");
                //ans = size;
            }
        }
        int sum = 0;
        for (int a: ans) sum += a;
        return sum / ans.size();
    }

    int benefit_count() {
        int ans = -1;
        for (field* f: fields) {
            if (f != nullptr) {
                int size = f -> benefit_count();
                EXCEPTION_CHECK(ans != -1 && ans != size, "FieldGroup::benefit_size: benefit_size校验失败");
                ans = size;
            }
        }
        EXCEPTION_CHECK(ans == -1, "FieldGroup::benefit_size: benefit_size获取失败");
        return ans;
    }
    

    const int tag;
private:
    bool request_permit;
    std::vector<field*> fields;
    inline static std::vector<field*> backup_field; // 备份存储区域
    double inferiority;
    double inferiority_increment;
    int owner;
    int pre_lock_time;
    std::tuple<int, int, double> info_record;
};


void FieldGroup::save(Object* obj) {
    // 使用 lambda 表达式作为自定义比较函数进行排序
    std::sort(backup_field.begin(), backup_field.end(), 
        [](const field* a, const field* b) {
            return a->get_remain_empty() > b->get_remain_empty();  // 从大到小排序
            //return a -> get_disk() -> remainEmpty() > b -> get_disk() -> remainEmpty();
        });
    int backup_index = 0;
    std::unordered_set<int> disk_choice;
    std::vector<field*> field_choice;

    for (int field_index = 0; field_index < 3; ++ field_index) {
        if (fields[field_index] == nullptr) {
            if (backup_index == backup_field.size()) {
                throw OutOfDiskSpace_Exception();
            }
            while (disk_choice.find(backup_field[backup_index] -> get_disk() -> id) != disk_choice.end()) {
                ++ backup_index;
                if (backup_index == backup_field.size()) {
                    throw OutOfDiskSpace_Exception();
                }
            }
            if (backup_field[backup_index] -> get_remain_empty() < obj -> size) {
                throw OutOfDiskSpace_Exception();
                if (backup_index == backup_field.size()) {
                    throw OutOfDiskSpace_Exception();
                }
            }
            field_choice.push_back(backup_field[backup_index]); 
            disk_choice.insert(backup_field[backup_index] -> get_disk() -> id);
            ++ backup_index;
        } else {
            if (disk_choice.find(fields[field_index] -> get_disk() -> id) != disk_choice.end()) {
                throw OutOfDiskSpace_Exception();
            }
            if (fields[field_index] -> get_remain_empty() < obj -> size) {
                throw OutOfDiskSpace_Exception();
            }
            field_choice.push_back(fields[field_index]);
            disk_choice.insert(fields[field_index] -> get_disk() -> id);
        }
    }

    try {
        for (field* fc: field_choice) {
            fc -> save(obj); 
            obj -> addDisk(fc -> get_disk());
        }  
    } catch(const Exception& e) {
        throw Exception(e.what());
    } catch(...) {
        throw Exception("未知异常。");
    }



}
