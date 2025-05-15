#pragma once

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <sstream>

#include "wrong.h"
#include "disk_base.h"
#include "time_check.h"
#include "reward_query.h"

class Disk;
class field;
namespace InfoView {
    std::string field_view(field* f);
    std::string tag_dis_view(unordered_map<int, vector<tuple<field*, field*, field*>>>& diskByLabel);
}




std::vector<Disk*> disks;
class field {
public:
    field(int _tag, int _start, int _size, Disk* _disk): tag(_tag) {
        start = _start;
        size = _size;
        remainEmpty = _size;
        profit = 0;
        disk = _disk;
        owner_flag = new bool(false);
        benefit_index_delta = 0;

    }
    void addProfit(double delta) {
        profit += delta;
    }
    double getProfit() {
        return profit;
    }
    void save(ObjectBase* obj);
    void advanceStart();
    void retreatEnd();

    const int tag;
    bool* owner_flag;
    void add_profit_index(int index);
    void erase_profit_index(int index);


    int benefit_size() const;
    int get_benefit_begin() const;
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



    friend std::string InfoView::field_view(field* f);
private:
    std::set<int> benefit_index;
    double profit;
    int remainEmpty;    
    int start;
    int size;
    Disk* disk;
    int benefit_index_delta;
};


class Disk: public DiskBase {
public:
    Disk(int _id, int _V, int _G): DiskBase(_id, _V, _G), priCost(1), 
        field_belong(_V, nullptr), readProfit(_V, 0) {
            current_field = nullptr;
            profit_amount = 0;
        };
    void save(ObjectBase* obj, field* f) { 
        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        EXCEPTION_CHECK(all_field.find(f) == all_field.end(), "要存储对象的域不属于该磁盘。");
        EXCEPTION_CHECK(f -> get_remain_empty() < obj -> size, "域空间不足，无法保存对象。");
        int query_pos = f -> get_start();
        int field_end = f -> get_end();
        std::vector<int> ans;
        for (int index = 0; index < obj -> getSize(); ++ index) {
            while (!isEmpty(query_pos)) {
                query_pos = (query_pos + 1) % size;
                EXCEPTION_CHECK(query_pos == field_end, "remainEmpty足够，但是域空间不足。");
            }
            save_obj({obj, index}, query_pos);
            ans.push_back(query_pos);
            query_pos = (query_pos + 1) % size;
        }
        f -> add_remain_empty(- obj -> size);
        write_output.disk_id.push_back(id);                
        write_output.disk_pos.push_back(ans);
        //field_adjust(f);


    }


    void remove(ObjectBase* obj) override {
        for(int del_index: get_obj_pos(obj)) {
            field_belong[del_index] -> add_remain_empty(1);
        }
        set_profit(obj, 0);
        DiskBase::remove(obj);
    }
    void move(int index) override {
        priCost = 1;
        DiskBase::move(index);

    }
    void pass() override {
        priCost = 1;
        DiskBase::pass();
    }
    double profit_amount;
    void read() override {
        
        if (priCost == 1) priCost = 64;
        else {
            priCost = std::max(16, static_cast<int>(std::ceil(priCost * 0.8)));
        }
        profit_amount += readProfit[getPos()];
        DiskBase::read();
    }

    field* add_field(int _tag, int _start, int _size) { // 标签为-1表示备份域
        //debug << "call add_field: " << id << " " << _tag << " " << _start << " " << _size << newLine; 
        EXCEPTION_CHECK(_start >= size || _start < 0, "域的起始位置越过磁盘边界。");
        for (int i = 0; i < _size; i ++) {
            EXCEPTION_CHECK(field_belong[(_start + i) % size] != nullptr, "磁盘的非空位置不能创建域。");
        }
        field* f = new field(_tag, _start, _size, this);
        for (int i = 0; i < _size; i ++) field_belong[(_start + i) % size] = f;
        if (fields.find(_tag) == fields.end()) fields[_tag] = std::vector<field*>();
        fields[_tag].push_back(f);
        all_field.insert(f);
        return f;
    }
    std::string last_ope;
    
    void step() {
        double move_dis = 0;
        last_ope = "";
        int remain = token;
        if (current_field == nullptr) {
            field* field_choice = nullptr;
            double max_profit = -1;
            for (auto& entry: fields) {
                if (entry.first == -1) continue;
                for (auto& f: entry.second) {
                    if (*f -> owner_flag) continue;
                    double f_profit;
                    if (f == field_belong[getPos()]) {
                        if (f -> benefit_size() == 0) {
                            f_profit = -1;
                        } else {
                            f_profit = f -> getProfit() / (f -> benefit_size());
                        }
                    } else {
                        if (f -> benefit_size() == 0) {
                            f_profit = 0;
                        } else {
                            f_profit = f -> getProfit() / (f -> benefit_size() + token/5);
                        }
                    }

                    if (f_profit > max_profit) {
                        max_profit = f_profit;
                        field_choice = f;
                    }
                }
            }
            EXCEPTION_CHECK(field_choice == nullptr, "调用step函数时域选择异常。");
            if constexpr (false && !submit) {
                if (id == 1) {
                    debug << newLine;
                    debug << "epoch: " << epoch << "  start: " << field_choice -> get_start() << "  tag: " 
                        << field_choice -> tag << "  pos: " << getPos() << "  aim_pos: " 
                        << field_choice -> benefit_size() << newLine;
                }                
            }

            
            if (field_choice != field_belong[getPos()]) {
                if (field_choice ->benefit_size() == 0) {
                    return;
                }
                current_field = field_choice;
                EXCEPTION_CHECK(*current_field -> owner_flag, "owner_flag值异常。");
                *current_field -> owner_flag = true;
                //move((field_choice -> get_start() + field_choice -> get_benefit_begin()) % size);
                move(field_choice -> get_benefit_begin());
                last_ope += "j " + std::to_string(field_choice -> get_start());
                return;
            } else {
                if (field_choice ->benefit_size() == 0) {
                    return;
                }
                current_field = field_choice;
                EXCEPTION_CHECK(*current_field -> owner_flag, "owner_flag值异常。");
                *current_field -> owner_flag = true;
                last_ope += "j " + std::to_string(field_choice -> get_start());
            }


        }
        while (true) {
            bool pass_flag = true;
            int query_dis = get_query_dis();
            for (int i = 0; i < query_dis; i ++) {
                if ((getObj((getPos() + i) % size) != nullptr) && readProfit[(getPos() + i) % size] > 0)  
                    pass_flag = false;
            }
            if ((priCost == 1) && (isEmpty() || (readProfit[getPos()] == 0))) pass_flag = true;

            if (pass_flag) {
                if (remain <= 0) {
                    return;
                }
                remain --;
                last_ope += "p";
                pass();
                if ((current_field != nullptr) && (((getPos() + size - current_field->get_start()) % size) >= current_field->get_size())) {
                    EXCEPTION_CHECK(!*current_field -> owner_flag, "owner_flag值异常。");
                    *current_field -> owner_flag = false;
                    current_field = nullptr;
                }
                move_dis += 1;
            } else {
                int need;
                if (priCost == 1) need = 64;
                else need = std::max(16, static_cast<int>(std::ceil(priCost * 0.8)));
                if (remain < need) {
                    return;
                }
                read();
                if ((current_field != nullptr) && (((getPos() + size - current_field->get_start()) % size) >= current_field->get_size())) {
                    EXCEPTION_CHECK(!*current_field -> owner_flag, "owner_flag值异常。");
                    *current_field -> owner_flag = false;
                    current_field = nullptr;
                }
                move_dis += 1;
                last_ope += "r";
                remain -= need;                    
            }
        }
    }


    std::string toString() const {
        std::string ans = "";
        for(int i = 0; i < size; i ++) {
            std::pair<ObjectBase*, int> obj_block = getObjblock(i);
            if (obj_block.first == nullptr) {
                ans += "(null) ";
            } else {
                ans += "(" + std::to_string(obj_block.first -> getId()) + "," + std::to_string(obj_block.second) + ") ";
            }
        }
        return ans;

    }

    void set_profit(ObjectBase* obj, double newProfit) {
        TimeCheck::start("set_profit");
        for (const int& index : get_obj_pos(obj)) {
            field& f = *field_belong[index];  // 获取 field 引用
            f.addProfit(newProfit - readProfit[index]);
            double oldProfit = readProfit[index];
            readProfit[index] = newProfit;
            if (f.tag == -1) continue;
            if (oldProfit != 0 && newProfit == 0) {
                f.erase_profit_index(index);
            }
            if (oldProfit == 0 && newProfit != 0) {
                f.add_profit_index(index);
            }
        }
    
        TimeCheck::end("set_profit");
    }
    
    

    inline void set_profit(ObjectBase* obj, int obj_index, double newProfit) {
        throw Exception("调用未完善的函数: set_profit。");
        int pos = get_obj_pos(obj)[obj_index];
        if (field_belong[pos] -> tag == -1) return;
        field_belong[pos] -> addProfit(newProfit - readProfit[pos]);
        field& f = *field_belong[pos];
        if (newProfit == 0) {
            f.erase_profit_index(pos);
        } else {
            f.add_profit_index(pos);
        }
        readProfit[pos] = newProfit;
    }


    std::string debug_state();


private:
    int priCost;
    std::vector<field*> field_belong;
    std::map<int, std::vector<field*>> fields;
    std::vector<double> readProfit;
    field* current_field;
    std::unordered_set<field*> all_field;

    std::set<field*> temp_debug;
    void field_adjust(field* f) {
        _field_adjust(f);
        return;
        debug << "调整前" << newLine;
        for (field* f: temp_debug) {
            debug << InfoView::field_view(f) << newLine << "=========================" << newLine;
        }
        _field_adjust(f);
        debug << "调整后" << newLine;
        for (field* f: temp_debug) {
            debug << InfoView::field_view(f) << newLine << "=========================" << newLine;
        }
        temp_debug.clear();
    }
    void _field_adjust(field* const f) {
        EXCEPTION_CHECK(all_field.find(f) == all_field.end(), "要调整的域不属于该磁盘。");
        double disk_spare = ((double) remainEmpty()) / size;
        if (((((double)f -> get_remain_empty())/ f -> get_size()) > 0.5 * disk_spare) && (f -> get_remain_empty() > 1)) return;
        temp_debug.insert(f);
        int start = f -> get_start();
        int start_pre = (start + size - 1) % size;
        field* const pre_field = field_belong[start_pre];
        if (pre_field != nullptr) {
            pre_field -> retreatEnd();
            if (isEmpty(start_pre)) {
                pre_field -> add_remain_empty(-1);
            } else {
                if (readProfit[start_pre] != 0) {
                    f -> addProfit(- readProfit[start_pre]);
                }
            }
            field_adjust(pre_field);
        }
        f -> advanceStart();
        if (isEmpty(start_pre)) {
            f -> add_remain_empty(1);
        } else {
            if (readProfit[start_pre] != 0) {
                f -> addProfit(readProfit[start_pre]);
                f -> add_profit_index(start_pre);
            }
        }
        field_belong[start_pre] = f;
        field_adjust(f);

    }


    int get_query_dis() {
        int query_dis;
        switch(priCost) {
            case 1:
                query_dis = 1;
                break;
            case 64:
                query_dis = 3;
                break;
            case 52:
                query_dis = 5;
                break;
            case 42:
                query_dis = 6;
                break;
            case 34:
                query_dis = 8;
                break;
            case 28:
                query_dis = 9;
                break;
            case 23:
                query_dis = 10;
                break;
            case 19:
                query_dis = 10;
                break;
            case 16:
                query_dis = 10;
                break;
            default:
                throw Exception("调用get_query_dis函数异常，priCost不在查询范围。");
        }
        return query_dis;

    }



};


std::ostream& operator<<(std::ostream &os, const Disk &d) {
    os << d.toString();
    return os;
}



void field::save(ObjectBase* obj) {
    disk -> save(obj, this);
}
int field::get_end()  {
    return (start + size) % ( disk -> size);
}
int field::get_benefit_begin() const { // 相对于域起始位置的坐标。
    EXCEPTION_CHECK(benefit_index.empty(), "benefit_index为空，无法获取起始位置。");
    //return (*benefit_index.begin() + benefit_index_delta + disk -> size - start) % disk -> size;
    return (start + *benefit_index.begin() + benefit_index_delta) % disk -> size;
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

        debug << "epoch: " << epoch << newLine;
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


std::string Disk::debug_state() {
    if constexpr (submit) return "";
    std::ostringstream oss;
    oss << "**************************************\n";
    oss << "磁盘id:" << id << "  pos: " << std::to_string(getPos()) << "  remain_empty: " << remainEmpty() << "\n";
    oss << "last_ope: " << last_ope << "\n";
    int test = 0;
    for (int index = 0; index < size; index++) if (isEmpty(index)) test ++;
    if (test != remainEmpty()) {
        throw Exception("磁盘剩余空间计算异常。");
    }
    for (int index = 0; index < size; index++) {
        oss << " | ";
        if (index % 100 == 0)
            oss << " | 磁盘块号,对象id,对象tag,对象块号,对象请求数 | ";
        if (index == getPos())
            oss << " | " << "磁头";
        std::pair<ObjectBase*, int> obj_block = getObjblock(index);
        oss << std::to_string(index) << ",";
        if (obj_block.first) {
            oss << std::to_string(obj_block.first->id) << ",";
            oss << std::to_string(obj_block.first->tag) << ",";
            oss << std::to_string(obj_block.second) << ",";
            oss << std::to_string(readProfit[index]);
        }
    }
    oss << "\n                  ====================\n";
    for (auto& entry: fields) {
        for (auto& f: entry.second) {
            oss << "[" << f -> tag << "," << f -> get_start() << "," << f -> get_size() << "," 
                << f -> get_remain_empty() << "," << f -> getProfit() << "]  ";
                if (f->benefit_size() != 0) {
                    oss <<"("<< f -> get_benefit_begin() <<","<< f -> benefit_size() << ")   ";
                }
        }
    }
    oss << "\n\n===";
    for (field* f: all_field) {
        oss << "[" << f -> tag << "," << f -> get_start() << "," << f -> get_size() << "," 
        << f -> get_remain_empty() << "," << f -> getProfit() << "]  ";
        if (f->benefit_size() != 0) {
            oss <<"("<< f -> get_benefit_begin() <<","<< f -> benefit_size() << ")   ";
        }
    }
    double sum_profit = 0;
    for (auto& entry: fields) {
        if (entry.first == -1) continue;
        for (auto& f: entry.second) {
            sum_profit += f -> getProfit();
        }
    }
    oss << "\nsum_profit: " << sum_profit;
    oss << "\n**************************************\n";
    std::string ans = oss.str();
    return ans;
} 
