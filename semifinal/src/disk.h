#pragma once

#include "wrong.h"
#include "disk_base.h"
#include "time_check.h"
#include "tool.h"
#include "field.h"
#include "disk_tool.h"
#include "point_schedule.h"
class Disk: public DiskBase {
public:
    Disk(int _id, int _V, int _G, int _swap_limit): DiskBase(_id, _V, _G), /*priCost(1), */
        field_belong(_V, nullptr), readProfit(_V, 0), swap_limit(_swap_limit) {
            current_field.fill(nullptr);
            preCost.fill(1);
            profit_amount = 0;
        };
    void save(ObjectBase* obj, field* f) { 
        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        EXCEPTION_CHECK(std::find(all_field.begin(), all_field.end(), f) == all_field.end(), "要存储对象的域不属于该磁盘。");
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
    }


    void remove(ObjectBase* obj) override {
        for(int del_index: get_obj_pos(obj)) {
            field_belong[del_index] -> add_remain_empty(1);
        }
        set_profit(obj, 0);
        DiskBase::remove(obj);
    }
    void move(int index, int head_id) override {
        preCost[head_id] = 1;
        DiskBase::move(index, head_id);

    }
    void pass(int head_id) override {
        preCost[head_id] = 1;
        DiskBase::pass(head_id);
    }
    double profit_amount;
    void read(int head_id) override {
        
        if (preCost[head_id] == 1) preCost[head_id] = 64;
        else {
            preCost[head_id] = std::max(16, static_cast<int>(std::ceil(preCost[head_id] * 0.8)));
        }
        profit_amount += readProfit[getHeadPos(head_id)];
        DiskBase::read(head_id);
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
        all_field.push_back(f);
        return f;
    }
    std::string last_ope;
    
    void step(int head_id) {
        EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "Disk::step: 磁头索引超出范围");
        double move_dis = 0;
        last_ope = "";
        int last_pos = get_head_pos(head_id);
        int remain = token;
        if (current_field[head_id] == nullptr) {
            field* field_choice = nullptr;
            double max_profit = -1;
            field_choice = Tool::highestScoreChoice<field>(all_field, [&](const field* f) -> double {
                if (f -> tag == -1) return std::numeric_limits<double>::quiet_NaN();
                if (f -> is_lock()) return std::numeric_limits<double>::quiet_NaN();
                //return - f -> get_inferiority();
                double f_profit;
                if (f == field_belong[getHeadPos(head_id)]) {
                    if (f -> benefit_size() == 0) {
                        f_profit = std::numeric_limits<double>::quiet_NaN();
                    } else {
                        f_profit = f -> getProfit() / (f -> benefit_size());
                    }
                } else {
                    if (f -> benefit_size() == 0) {
                        f_profit = std::numeric_limits<double>::quiet_NaN();
                    } else {
                        f_profit = f -> getProfit() / (f -> benefit_size() + token/5);
                    }
                }
                return f_profit;
            });
            if (field_choice == nullptr) {
                forward_step(head_id, token);
                DataCollector::head_move(id, head_id, get_head_pos(head_id), last_ope);
                return;
            }
            if (field_choice != field_belong[getHeadPos(head_id)]) {

                current_field[head_id] = field_choice;
                current_field[head_id] -> lock(id);
                move(field_choice -> get_benefit_begin(), head_id);
                last_ope += "j " + std::to_string(field_choice -> get_benefit_begin());
                return;
            } else {
                current_field[head_id] = field_choice;
                current_field[head_id] -> lock(id);
                //last_ope += "j " + std::to_string(field_choice -> get_start());
            }
            DataCollector::field_switch(id, head_id, field_choice -> tag, field_choice -> benefit_size(), 
                ((double)field_choice -> get_benefit_count()) / field_choice -> benefit_size());
        }
        forward_step(head_id, token);
        if (current_field[head_id] ->get_benefit_count() == 0) {
            current_field[head_id] -> unlock(id);
            current_field[head_id] = nullptr;
        }
        else {
            int temp = (current_field[head_id]->get_benefit_end() + size - current_field[head_id]->get_start()) % size;
            if ((current_field[head_id] != nullptr) && (((getHeadPos(head_id) + size - current_field[head_id]->get_start()) % size) >= temp)) {
                current_field[head_id] -> unlock(id);
                current_field[head_id] = nullptr;
            }
        }
        DataCollector::head_move(id, head_id, get_head_pos(head_id), last_ope);

    }

    void forward_step(int head_id, int remain);
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

    void set_profit(int index, double newProfit) {
        TimeCheck::start("set_profit");

        field& f = *field_belong[index];  // 获取 field 引用
        f.addProfit(newProfit - readProfit[index]);
        double oldProfit = readProfit[index];
        readProfit[index] = newProfit;
        if (f.tag == -1) {
            TimeCheck::end("set_profit");
            return;
        }
        if (oldProfit != 0 && newProfit == 0) {
            f.erase_profit_index(index);
        }
        if (oldProfit == 0 && newProfit != 0) {
            f.add_profit_index(index);
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

    double get_profit_sum() {
        double ans = 0;
        for (field* f: all_field) {
            if (f -> tag != -1) ans += f -> getProfit();
        }
        return ans;
    }
    void recycle() {
        if (GlobalVariable::epoch / 1800 + 1 >= GlobalVariable::read_frequency[1].size()) return;
        debug << "start recycle" << newLine;
        std::vector<double> field_scores;
        for (field* f: all_field) {
            if (f -> tag == -1) {
                field_scores.push_back(0);
                continue;
            }
            field_scores.push_back(GlobalVariable::read_frequency[f -> tag][GlobalVariable::epoch / 1800 + 1]);
        }
        Tool::sortByScore(all_field, field_scores);
        int remain = swap_limit;
        for (field* f: all_field) {
            if (remain <= 0) {
                EXCEPTION_CHECK(remain < 0, "Disk::recycle: remain值异常");
                return;
            }
            std::vector<std::pair<int, int>> swaps = 
                DiskTool::defragmentation(f -> get_size(), [&f, this](int index) -> bool {
                    return !isEmpty((f -> get_start() + index) % size);
                }, remain);
            for (std::pair<int, int> s: swaps) {
                swap((s.first + f -> get_start()) % size, (s.second + f -> get_start()) % size);
            }
        }
        

        debug << "end recycle" << newLine;

    }

protected:
    void swap(int index1, int index2) override {
        debug << "start swap: " << index1 << " " << index2 << newLine;
        field* field_1 = field_belong[index1];
        field* field_2 = field_belong[index2];      
        if (!isEmpty(index1)) field_1 -> add_remain_empty(1);
        if (!isEmpty(index2)) field_2 -> add_remain_empty(1);
        DiskBase::swap(index1, index2);
        if (!isEmpty(index1)) field_1 -> add_remain_empty(-1);
        if (!isEmpty(index2)) field_2 -> add_remain_empty(-1);
        double profit1 = readProfit[index1];
        double profit2 = readProfit[index2];
        field_1 -> addProfit(profit2 - profit1);
        field_2 -> addProfit(profit1 - profit2);
        set_profit(index1, profit2);
        set_profit(index2, profit1);
        debug << "end swap" << newLine;
    }
private:
    std::array<int, HyperParameters::head_num> preCost;
    std::vector<field*> field_belong;
    std::map<int, std::vector<field*>> fields;
    std::vector<double> readProfit;
    std::array<field*, HyperParameters::head_num> current_field;
    std::vector<field*> all_field;
    std::queue<field*> fieldReadPlan;
    const int swap_limit;



    int get_query_dis(int head_id) {
        int query_dis;
        switch(preCost[head_id]) {
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






std::string Disk::debug_state() {
    if constexpr (submit) return "";
    std::ostringstream oss;
    oss << "**************************************\n";
    oss << "磁盘id:" << id << "  head_pos: " << Tool::toString<int, 2>(getHeadPos()) << "  remain_empty: " << remainEmpty() << "\n";
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
        auto headPos = getHeadPos();
        if (std::find(headPos.begin(), headPos.end(), index) != headPos.end()) {
            oss << " | " << "磁头";
        }

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
void Disk::forward_step(int head_id, int remain) {
    /*
    debug << "start forward_step" << newLine;
    EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "Disk::step: 磁头索引超出范围");
    std::vector<bool> opes = pointSchedule(token, [&](int index) {
        return readProfit[(get_head_pos(head_id) + index) % size] > 0;
    }, 10, preCost[head_id]);

    if (GlobalVariable::epoch > 10000) {
        debug << "preCost:" << preCost[head_id] << newLine;
        for (int i = 0; i < 50; i ++) {
            debug << ((readProfit[(get_head_pos(head_id) + i) % size] > 0)?"1":"0");
        }
        debug << newLine;
        for(bool b: opes) {
            debug << (b?"r":"p") << " ";
        }        
    }    

    for (bool ope: opes) {
        if (ope) {
            read(head_id);
            last_ope += "r";
        } else {
            pass(head_id);
            last_ope += "p";
        }
    }

    debug << "end forward_step" << newLine;
    return;*/
    if (remain <= 0) {
        return;
    }
    double move_dis = 0;
    last_ope = "";
    int last_pos = get_head_pos(head_id);
    while (true) {
        bool pass_flag = true;
        int query_dis = get_query_dis(head_id);
        for (int i = 0; i < query_dis; i ++) {
            if ((getObj((getHeadPos(head_id) + i) % size) != nullptr) && readProfit[(getHeadPos(head_id) + i) % size] > 0)  
                pass_flag = false;
        }
        if ((preCost[head_id] == 1) && (isEmpty(getHeadPos(head_id)) || (readProfit[getHeadPos(head_id)] == 0))) pass_flag = true;

        if (pass_flag) {
            if (remain <= 0) {
                return;
            }
            remain --;
            last_ope += "p";
            pass(head_id);
            move_dis += 1;
        } else {
            int need;
            if (preCost[head_id] == 1) need = 64;
            else need = std::max(16, static_cast<int>(std::ceil(preCost[head_id] * 0.8)));
            if (remain < need) {
                return;
            }
            read(head_id);
            move_dis += 1;
            last_ope += "r";
            remain -= need;                    
        }
    }

}