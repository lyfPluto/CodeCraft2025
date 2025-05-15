#pragma once

#include "wrong.h"
#include "disk_base.h"
#include "time_check.h"
#include "tool.h"
#include "field.h"
#include "disk_tool.h"
#include "point_schedule.h"
#include "write_tool.h"

void __check(std::vector<bool> v1, std::vector<bool> v2, std::vector<bool> state, int preCost) {
    bool dif = false;
    if (v1.size() != v2.size()) {
        dif = true;
    } else {
        for (int index = 0; index < v1.size(); ++ index) {
            if (v1[index] != v2[index]) {
                dif = true;
                break;
            }
        }
    }
    if (dif) {
        debug << "start __check" << newLine;
        debug << "preCost:" << preCost << newLine;
        for (bool s: state) {
            debug << ((s)?"1":"0");
        }
        debug << newLine;
        for(bool b: v1) {
            debug << (b?"r":"p") << " ";
        }  
        debug << newLine;
        for(bool b: v2) {
            debug << (b?"r":"p") << " ";
        }  
        debug << newLine;
    }
}

class Disk: public DiskBase {
public:
    Disk(int _id, int _V, int _G, int _swap_limit): DiskBase(_id, _V, _G), /*priCost(1), */
        field_belong(_V, nullptr), readProfit(_V, 0), swap_limit(_swap_limit) {
            current_field.fill(nullptr);
            preCost.fill(1);
            profit_amount = 0;
            delete_time = std::vector<int> (_V, 0);
        };
    void save_separate(ObjectBase* obj, field* f) { 
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
    void save(ObjectBase* obj, field* f) { 

        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        EXCEPTION_CHECK(std::find(all_field.begin(), all_field.end(), f) == all_field.end(), "要存储对象的域不属于该磁盘。");

        EXCEPTION_CHECK(f -> get_remain_empty() < obj -> size, "域空间不足，无法保存对象。");
        int query_pos = f -> get_start();
        int field_end = f -> get_end();
        std::vector<int> ans;

        std::vector<int> save_pos;
        try {
            /*
            save_pos = WriteTool::centralized_storage(
                f -> get_size(),
                obj -> getSize(),
                [&](int index) -> int {
                    return !isEmpty(query_pos + index % f -> get_size());
                }
            );   */ 
            save_pos = WriteTool::feature_storage(
                f ->get_size(), 
                obj -> getSize(), 
                [&] (int index) {
                    return !isEmpty(query_pos + index % f -> get_size());
                },
                [&] (int index) {
                    return delete_time[query_pos + index % f -> get_size()];
                },
                GlobalInfo::obj_delete_time[obj -> id]
            );

        } catch(...) {
            save_pos = WriteTool::separate_storage(
                f -> get_size(),
                obj -> getSize(),
                [&](int index) -> int {
                    return !isEmpty(query_pos + index % f -> get_size());
                }
            );  
        }
        for (int index = 0; index < obj -> getSize(); ++ index) {
            int disk_save_pos = (query_pos + save_pos[index] % f -> get_size()) % size;
            save_obj({obj, index}, disk_save_pos);
            ans.push_back(disk_save_pos);
            delete_time[disk_save_pos] = GlobalInfo::obj_delete_time[obj -> id];
        } 
       /*
        for (int index = 0; index < obj -> getSize(); ++ index) {
            while (!isEmpty(query_pos)) {
                query_pos = (query_pos + 1) % size;
                EXCEPTION_CHECK(query_pos == field_end, "remainEmpty足够，但是域空间不足。");
            }
            save_obj({obj, index}, query_pos);
            ans.push_back(query_pos);
            query_pos = (query_pos + 1) % size;
        }*/
        f -> add_remain_empty(- obj -> size);
        write_output.disk_id.push_back(id);                
        write_output.disk_pos.push_back(ans);
    }
    void try_save(ObjectBase* obj, field* f) {
        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        EXCEPTION_CHECK(std::find(all_field.begin(), all_field.end(), f) == all_field.end(), "要存储对象的域不属于该磁盘。");

        EXCEPTION_CHECK(f -> get_remain_empty() < obj -> size, "域空间不足，无法保存对象。");
        int query_pos = f -> get_start();
        int field_end = f -> get_end();
        WriteTool::centralized_storage(
            f -> get_size(),
            obj -> getSize(),
            [&](int index) -> int {
                return !isEmpty(query_pos + index % f -> get_size());
            }
        );

    }

    // 获取保存位置
    std::vector<int> save_first_phase(ObjectBase* obj, field* f) { 

        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        EXCEPTION_CHECK(std::find(all_field.begin(), all_field.end(), f) == all_field.end(), "要存储对象的域不属于该磁盘。");

        EXCEPTION_CHECK(f -> get_remain_empty() < obj -> size, "域空间不足，无法保存对象。");
        int query_pos = f -> get_start();
        int field_end = f -> get_end();

        std::vector<int> save_pos = WriteTool::centralized_storage(
            f -> get_size(),
            obj -> getSize(),
            [&](int index) -> int {
                return !isEmpty(query_pos + index % f -> get_size());
            }
        );
        return save_pos;
    }
    // 指定对象保存位置
    void save_second_phase(ObjectBase* obj, field* f, std::vector<int> save_pos) { 

        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        EXCEPTION_CHECK(std::find(all_field.begin(), all_field.end(), f) == all_field.end(), "要存储对象的域不属于该磁盘。");

        EXCEPTION_CHECK(f -> get_remain_empty() < obj -> size, "域空间不足，无法保存对象。");
        int query_pos = f -> get_start();
        int field_end = f -> get_end();
        std::vector<int> ans;

        for (int index = 0; index < obj -> getSize(); ++ index) {
            int disk_save_pos = (query_pos + save_pos[index] % f -> get_size()) % size;
            save_obj({obj, index}, disk_save_pos);
            ans.push_back(disk_save_pos);
        } 

        f -> add_remain_empty(- obj -> size);
        write_output.disk_id.push_back(id);                
        write_output.disk_pos.push_back(ans);
    }

    void remove(ObjectBase* obj) override {
        for(int del_index: get_obj_pos(obj)) {
            field_belong[del_index] -> add_remain_empty(1);
            delete_time[del_index] = 0;
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

    field* add_field(int _tag, int _start, int _size, FieldType type) { // 标签为-1表示备份域
        //debug << "call add_field: " << id << " " << _tag << " " << _start << " " << _size << newLine; 
        EXCEPTION_CHECK(_start >= size || _start < 0, "域的起始位置越过磁盘边界。");
        for (int i = 0; i < _size; i ++) {
            EXCEPTION_CHECK(field_belong[(_start + i) % size] != nullptr, "磁盘的非空位置不能创建域。");
        }
        field* f = new field(_tag, _start, _size, this, type);
        for (int i = 0; i < _size; i ++) field_belong[(_start + i) % size] = f;
        if (fields.find(_tag) == fields.end()) fields[_tag] = std::vector<field*>();
        fields[_tag].push_back(f);
        all_field.push_back(f);
        return f;
    }


    void step(int head_id) {
        EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "Disk::step: 磁头索引超出范围");
        double move_dis = 0;
        int last_pos = get_head_pos(head_id);
        int remain = token;
        if (current_field[head_id] == nullptr) {
            field* field_choice = nullptr;
            double max_profit = -1;
            field_choice = Tool::highestScoreChoice<field>(all_field, [&](const field* f) -> double {
                if (f -> get_type() == FieldType::backupField) return std::numeric_limits<double>::quiet_NaN();
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
                //DataCollector::head_move(id, head_id, get_head_pos(head_id), last_ope);
                return;
            }
            if (field_choice != field_belong[getHeadPos(head_id)]) {

                current_field[head_id] = field_choice;
                current_field[head_id] -> lock(id);
                move(field_choice -> get_benefit_begin(), head_id);
                return;
            } else {
                current_field[head_id] = field_choice;
                current_field[head_id] -> lock(id);
            }
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
        //DataCollector::head_move(id, head_id, get_head_pos(head_id), last_ope);

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

    void set_profit(ObjectBase* obj, double newProfit) override {
        TimeCheck::start("set_profit");
        for (const int& index : get_obj_pos(obj)) {
            field& f = *field_belong[index];  // 获取 field 引用
            f.addProfit(newProfit - readProfit[index]);
            double oldProfit = readProfit[index];
            readProfit[index] = newProfit;
            if (f.get_type() == FieldType::backupField) continue;
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
        if (f.get_type() == FieldType::backupField) {
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
        if (field_belong[pos]  -> get_type() == FieldType::backupField) return;
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
            if (f -> get_type() != FieldType::backupField) ans += f -> getProfit();
        }
        return ans;
    }
    void recycle() {
        //if (GlobalVariable::epoch / 1800 + 1 >= GlobalVariable::read_frequency[1].size()) return;
        std::vector<double> field_scores;
        for (field* f: all_field) {
            if (f -> get_type() != FieldType::mainField) {
                field_scores.push_back(0);
                continue;
            }
            if (f -> benefit_size() == 0) {
                field_scores.push_back(0);
                continue;
            }
            field_scores.push_back(((double) f -> benefit_count()) / f -> benefit_size());
            //field_scores.push_back(GlobalVariable::read_frequency[f -> tag][GlobalVariable::epoch / 1800 + 1]);
        }
        Tool::sortByScore(all_field, field_scores);
        int remain = swap_limit;
        for (field* f: all_field) {
            if (remain <= 0) {
                EXCEPTION_CHECK(remain < 0, "Disk::recycle: remain值异常");
                return;
            }
            int quota = remain / 2;
            if (quota < 10) quota = remain;
            remain -= quota;
            std::vector<std::pair<int, int>> swaps = 
                DiskTool::defragmentation(f -> get_size(), [&f, this](int index) -> bool {
                    return !isEmpty((f -> get_start() + index) % size);
                }, quota);
            remain += quota;
            for (std::pair<int, int> s: swaps) {
                swap((s.first + f -> get_start()) % size, (s.second + f -> get_start()) % size);
            }
        }
        

    }

    void busy_adjust() {
        if (GlobalVariable::epoch <= 20000) return;
        if (GlobalVariable::epoch >= 75000) return;
        std::vector<field*> field_adjust;
        std::vector<double> scores;
        for (field* f: all_field) {
            if (f -> get_type() == FieldType::mainField) {
                field_adjust.push_back(f);
                f -> set_request_permit(true);
                //scores.push_back(GlobalVariable::read_frequency[f -> tag][GlobalVariable::epoch / 1800 + 1]);
                scores.push_back(GlobalInfo::req_count[f -> tag][(GlobalVariable::epoch - 1) / 100]);
            }
        }
        Tool::sortByScore(field_adjust, scores);
        double scoreSum = std::accumulate(scores.begin(), scores.end(), 0);
        scoreSum /= 200;
        for (int index = field_adjust.size() - 1; index >= 0; -- index) {
            scoreSum -= scores[index];
            if (scoreSum <= 0) break;
            field_adjust[index] -> set_request_permit(false);
        }
        //(*field_adjust.rbegin()) -> set_request_permit(false);
        //(*(field_adjust.rbegin() + 1)) -> set_request_permit(false);
    }
    std::map<int, int> get_read_length() {
        std::map<int, int> ans;
        for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
            ans[tag] = 0;
        }
        for (field* f: all_field) {
            if (f -> get_type() == FieldType::mainField) {
                ans[f -> tag] += f -> benefit_size();
            }
        }
        return ans;

    }

protected:
    void swap(int index1, int index2) override {
        std::swap(delete_time[index1], delete_time[index2]);
        //debug << "start swap: " << index1 << " " << index2 << newLine;
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
        //debug << "end swap" << newLine;
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
    std::vector<int> delete_time;


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
    //oss << "last_ope: " << last_ope << "\n";
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
    EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "Disk::step: 磁头索引超出范围");
    HeadSchedule* headStrage = new OriginHeadSchedule();
    //HeadSchedule* headStrage = new BFSHeadSchedule(2);
    std::vector<bool> opes = headStrage -> getSchedule(token, [&](int index) {
        return readProfit[(get_head_pos(head_id) + index) % size] > 0;
    }, preCost[head_id]);
    for (bool ope: opes) {
        if (ope) {
            read(head_id);
        } else {
            pass(head_id);
        }
    }
}

