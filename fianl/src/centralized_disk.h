#pragma once

#include "wrong.h"
#include "disk_base.h"
#include "time_check.h"
#include "tool.h"
#include "disk_tool.h"
#include "point_schedule.h"
#include "write_tool.h"
#include "first_recycle_schedule.h"

class CentralizedDisk: public DiskBase {
public:
    CentralizedDisk(int _id, int _V, int _G, int _swap_limit): DiskBase(_id, _V, _G),
        readProfit(_V, 0), swap_limit(_swap_limit), headBelong(_V, -1) {
            preCost.fill(1);
            int head_size = 0.92 * _V / 6;
            for (int head_id = 0; head_id < HyperParameters::head_num; ++ head_id) {
                headRange[head_id] = {head_size * head_id, head_size * (head_id + 1)};
            }
            headRemainEmpty.fill(head_size);
            for (int head_id = 0; head_id < HyperParameters::head_num; ++ head_id) {
                int left = headRange[head_id].first;
                int right = headRange[head_id].second;
                for (int index = left; index < right; ++ index) {
                    headBelong[index] = head_id;
                }
            }
            backup_start = head_size * HyperParameters::head_num;
            for (int head_id = 0; head_id < HyperParameters::head_num; ++ head_id) {
                int left = headRange[head_id].first;
                int right = headRange[head_id].second;
                //right *= 0.6;
                for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                    int start = left + (tag - 1) * (right - left) / GlobalInfo::M;
                    tag_start[head_id][tag] = start;
                }                
            }

        };
    void save(ObjectBase* obj) { 
        if (GlobalVariable::epoch >= 1800) {
            //tag_start_flush();
        }
        EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");

        int head_choice = Tool::highestScoreChoice(HyperParameters::head_num, [&](int head_id) -> int {
            return headRemainEmpty[head_id];
        });


        if (headRemainEmpty[head_choice] < obj -> size) {
            throw OutOfDiskSpace_Exception();
        }
        int tag = obj -> tag;
        if (tag == 0) {
            tag = rand() % GlobalInfo::M + 1;
        }
        int left = headRange[head_choice].first;
        int right = headRange[head_choice].second;


        //int query_pos = left + (tag - 1) * (right - left) / GlobalInfo::M;
        int query_pos = tag_start[head_choice][tag];
        if (query_pos >= right) {
            query_pos = left;
        }
        std::vector<int> ans;
        std::vector<int> save_pos;
        try {
            save_pos = WriteTool::centralized_storage(
                headRange[head_choice].second - headRange[head_choice].first,
                obj -> getSize(),
                [&](int index) -> int {
                    return !isEmpty(left + (query_pos + index - left) % (right - left));
                }
            );
        } catch(...) {
            save_pos = WriteTool::separate_storage(
                headRange[head_choice].second - headRange[head_choice].first,
                obj -> getSize(),
                [&](int index) -> int {
                    return !isEmpty(left + (query_pos + index - left) % (right - left));
                }
            );
        }


        for (int index = 0; index < obj -> getSize(); ++ index) {
            int disk_save_pos = left + (query_pos + save_pos[index] - left) % (right - left);
            save_obj({obj, index}, disk_save_pos);
            ans.push_back(disk_save_pos);
        } 


        headRemainEmpty[head_choice] -= obj -> size;
        write_output.disk_id.push_back(id);                
        write_output.disk_pos.push_back(ans);
    }

    void save_backup(ObjectBase* obj) { 
        //EXCEPTION_CHECK(objExist(obj), "要存储的对象已经位于磁盘中。");
        if (objExist(obj)) {
            throw OutOfDiskSpace_Exception();
        }
        int query_pos = backup_start;
        std::vector<int> ans;
        for (int index = 0; index < obj -> getSize(); ++ index) {
            while (!isEmpty(query_pos)) {
                query_pos ++;
                if (query_pos >= size) {
                    throw OutOfDiskSpace_Exception();
                }
            }
            save_obj({obj, index}, query_pos);
            ans.push_back(query_pos);
        }
        write_output.disk_id.push_back(id);                
        write_output.disk_pos.push_back(ans);
    }



    void remove(ObjectBase* obj) override {
        for(int del_index: get_obj_pos(obj)) {
            if (headBelong[del_index] != -1) {
                headRemainEmpty[headBelong[del_index]] ++;
            }
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

    void read(int head_id) override {
        if (preCost[head_id] == 1) preCost[head_id] = 64;
        else {
            preCost[head_id] = std::max(16, static_cast<int>(std::ceil(preCost[head_id] * 0.8)));
        }
        DiskBase::read(head_id);
    }

    void step(int head_id) {
        EXCEPTION_CHECK(head_id < 0 || head_id >= HyperParameters::head_num, "Disk::step: 磁头索引超出范围");
        int last_pos = get_head_pos(head_id);
        int left = headRange[head_id].first;
        int right = headRange[head_id].second;
        if (last_pos >= right) {
            move(left, head_id);
            return;
        }
        forward_step(head_id, token);
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
            readProfit[index] = newProfit;
        }
        TimeCheck::end("set_profit");
    }

    void set_profit(int index, double newProfit) {
        TimeCheck::start("set_profit");
        readProfit[index] = newProfit;
        TimeCheck::end("set_profit");
        
    }
    
    

    inline void set_profit(ObjectBase* obj, int obj_index, double newProfit) {
        int pos = get_obj_pos(obj)[obj_index];
        readProfit[pos] = newProfit;
    }


    void recycle() {
        
        std::unordered_map<int, int> tag_aim;
        int head_choice = rand() % HyperParameters::head_num;
        int left = headRange[head_choice].first;
        int right = headRange[head_choice].second;
        std::vector<int> tags(right - left, 0);
        for (int index = left; index < right; ++ index) {
            if (isEmpty(index)) {
                continue;
            }
            int obj_id = getObj(index) -> id;
            if (GlobalInfo::obj_tags[obj_id] != 0) {
                tags[index - left] = GlobalInfo::obj_tags[obj_id];
                continue;
            }
            tags[index - left] = TagPredict::predict[obj_id];
        }
        int tag_size = (right - left) / GlobalInfo::M;
        std::unordered_map<int, std::pair<int, int>> tag_ranges;
        for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
            if (tag < GlobalInfo::M) {
                tag_ranges[tag] = {tag_start[head_choice][tag] - left,  tag_start[head_choice][tag + 1] - left};
            } else {
                tag_ranges[tag] = {tag_start[head_choice][tag] - left,  right - left};
            }
            
            
            //tag_ranges[tag] = {(tag - 1) * tag_size, tag * tag_size};
        }
        std::vector<std::pair<int, int>> swap_infos = RecycleTool::improve_recycle(right - left, swap_limit, tags, 
            tag_ranges);
        for (std::pair<int, int>& swap_info: swap_infos) {
            int tag_1 = isEmpty(left + swap_info.first)?0:TagPredict::predict[getObj(left + swap_info.first) -> id];
            int tag_2 = isEmpty(left + swap_info.second)?0:TagPredict::predict[getObj(left + swap_info.second) -> id];
            debug << "---------------------" << newLine;
            debug << swap_info.first << "  " << tag_1 << newLine;
            debug << swap_info.second << "  " << tag_2 << newLine;
            swap(left + swap_info.first, left + swap_info.second);
        }
        return;
        
        /*
        debug << "start firat recycle" << newLine;
        debug << "----------------------------------------" << newLine;
        //FirstRecycleSchedule* recycle_schedule = new GreedyFirstRecycleSchedule();
        FirstRecycleSchedule* recycle_schedule = new ImproveFirstRecycleSchedule();
        int head_choice = rand() % HyperParameters::head_num;
        int left = headRange[head_choice].first;
        int right = headRange[head_choice].second;
        const std::unordered_map<int, int>& tag_limit = tag_start[head_choice];
        std::vector<std::tuple<int, int, int, double>> obj_candicate;
        for (int index = left; index < right; ++ index) {
            if (isEmpty(index)) continue;
            int obj_id = getObj(index) -> id;
            if (GlobalInfo::obj_tags[obj_id] != 0) continue;
            int tag_pre = TagPredict::predict[obj_id];
            int pos_tag = 1 + (index - left) * GlobalInfo::M / (right - left);
            EXCEPTION_CHECK(pos_tag <= 0 || pos_tag > GlobalInfo::M, "CentralizedDisk::recycle: 对象标签异常");
            obj_candicate.push_back({index, tag_pre - 1, pos_tag - 1, 1});
            debug << "{" << index << "," << tag_pre << "," << pos_tag << "}";
        }
        
        const std::vector<std::pair<int, int>>& swap_infos = 
            recycle_schedule -> getSchedule(swap_limit, GlobalInfo::M, obj_candicate);
        for (const std::pair<int, int>& swap_info: swap_infos) {
            int swap_first = swap_info.first;
            int swap_second = swap_info.second;
            int pos_tag_1 = 1 + (swap_first - left) * GlobalInfo::M / (right - left);
            int pos_tag_2 = 1 + (swap_second - left) * GlobalInfo::M / (right - left);
            int tag_pre_1 = TagPredict::predict[TagPredict::predict[swap_first]];
            int tag_pre_2 = TagPredict::predict[TagPredict::predict[swap_second]];
            debug << "swap: " << swap_first - left << "  " << tag_pre_1 << newLine;
            debug << Tab  << swap_second - left << "  " << tag_pre_2 << newLine; 


            swap(swap_info.first, swap_info.second);
        }

        return;*/
        

    }

protected:
    void tag_start_flush() {
        for (int head_index = 0; head_index < HyperParameters::head_num; ++ head_index) {
            int left = headRange[head_index].first;
            int right = headRange[head_index].second;
            int remain_empty = headRemainEmpty[head_index];
            int new_right = left + (right - left - remain_empty);
            new_right *= 1.1;
            if (new_right >= right) return;
            for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
                int start = left + (tag - 1) * (new_right - left) / GlobalInfo::M;
                tag_start[head_index][tag] = start;
            }       
        }
    }

    void swap(int index1, int index2) override {
        if (!isEmpty(index1)) {
            if (headBelong[index1] != -1) {
                headRemainEmpty[headBelong[index1]] ++;
            }
        }
        if (!isEmpty(index2)) {
            if (headBelong[index2] != -1) {
                headRemainEmpty[headBelong[index2]] ++;
            }
        }
        DiskBase::swap(index1, index2);
        if (!isEmpty(index1)) {
            if (headBelong[index1] != -1) {
                headRemainEmpty[headBelong[index1]] --;
            }
        }
        if (!isEmpty(index2)) {
            if (headBelong[index2] != -1) {
                headRemainEmpty[headBelong[index2]] --;
            }
        }
        double profit1 = readProfit[index1];
        double profit2 = readProfit[index2];
        set_profit(index1, profit2);
        set_profit(index2, profit1);
    }
private:
    std::array<int, HyperParameters::head_num> preCost;
    std::array<std::pair<int, int>, HyperParameters::head_num> headRange;
    std::array<int, HyperParameters::head_num> headRemainEmpty;
    std::vector<double> readProfit;
    std::vector<int> headBelong;
    int backup_start;
    // 每个标签的起始保存位置
    std::array<std::unordered_map<int, int>, HyperParameters::head_num> tag_start;
    

    const int swap_limit;


};


std::ostream& operator<<(std::ostream &os, const CentralizedDisk &d) {
    os << d.toString();
    return os;
}



void CentralizedDisk::forward_step(int head_id, int remain) {
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

