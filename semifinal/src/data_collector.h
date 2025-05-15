#pragma once

#include <unordered_map>
#include "functions.h"
#include "debug.h"
#include "parameters.h"
#include "tool.h"

namespace __Info {
    struct field_group_lock {
        int start_time;
        int duration;
        int disk_id;
        int tag;
        int size;
        int benefit_count;
        int profit;
    };
};

namespace Info {
    int normal_save_count = 0;
    int backup_save_count = 0;
}

namespace __DataCollector {
    std::vector<int> request_create_time(30000000, -1);
    std::vector<int> request_complete_time(30000000, -1);
    std::vector<int> TTL(200, 0);
    std::map<std::pair<int, int>, std::vector<std::vector<int>>> field_switch_record;
    std::map<std::pair<int, int>, std::vector<std::tuple<int, int, std::string>>> head_log;
    std::vector<std::pair<int, int>> request_log;
    std::vector<__Info::field_group_lock> field_group_lock_record;
}

class DataCollector {
public:

    static void field_group_lock(int start_time, int disk_id, int tag, int size, int benefit_count, int profit) {
        __DataCollector::field_group_lock_record.push_back({start_time, 
            GlobalVariable::epoch - start_time, disk_id, tag, size, benefit_count, profit});
    }
    static void field_group_lock_flush() {
        if constexpr (!submit) {
            static Debug field_group_lock_log("field_group_lock_log", true);
            field_group_lock_log.clear();
            for (__Info::field_group_lock info: __DataCollector::field_group_lock_record) {
                field_group_lock_log << info.start_time << " " << info.duration << " " << info.disk_id << " " <<
                    info.tag << " " << info.size << " " << info.benefit_count << " " << info.profit 
                    << "  | {start_time,duration,disk_id,tag,size,profit} density: " 
                    << (double) info.benefit_count / info.size  << " speed: " 
                    << (double) info.size / info.duration << newLine;
            }
        }
    }

    static void request_tag(int tag) {
        __DataCollector::request_log.push_back({GlobalVariable::epoch, tag});
    }
    static void request_tag_flush() {
        if constexpr (!submit) {
            static Debug request_tag_log("request_tag_log", true);
            request_tag_log.clear();
            std::vector<std::vector<int>> ans;
            for (std::pair<int, int>& info: __DataCollector::request_log) {
                while (info.first / 100 >= ans.size()) ans.push_back(std::vector<int> (GlobalVariable::M, 0));
                ans[info.first / 100][info.second - 1] ++;
            }
            for (int index = 0; index < ans.size(); index ++) {
                request_tag_log << ans[index] << "  | epoch: {" << index * 100 << "," << index * 100 + 99 << newLine;
            }

        }

    }

    static void head_move(int disk_id, int head_id, int pos, std::string ope) {
        std::pair<int, int> key = {disk_id, head_id};
        if (__DataCollector::head_log.find(key) == __DataCollector::head_log.end()) {
            __DataCollector::head_log.insert({key, {}});
        }
        __DataCollector::head_log[key].push_back({GlobalVariable::epoch, pos, Tool::opeSimplify(ope)});
    }
    static void head_move_flush() {
        if constexpr (!submit) {
            static Debug head_move_log("head_move_log", true);
            head_move_log.clear();

            for (auto& entry: __DataCollector::head_log) {
                for (std::tuple<int, int, std::string>& info: entry.second) {

                    head_move_log << entry.first.first << " " << entry.first.second << " " << std::get<0> (info) << " " 
                        << std::get<1> (info) << " " << std::get<2> (info)  << "  | " 
                        << "{disk_id, head_id, epoch, pos, ope}" << newLine;
                }
            }
        }
    }

    static void request_create(int req_id) {
        if constexpr (!submit) {
            __DataCollector::request_create_time[req_id] = GlobalVariable::epoch;
        }
    }
    static void request_complete(int req_id) {
        if constexpr (!submit) {
            __DataCollector::request_complete_time[req_id] = GlobalVariable::epoch;
        }
    }
    static void field_switch(int disk_id, int head_id, int tag, int read_size, double density) {
        if (__DataCollector::field_switch_record.find({disk_id, head_id}) == __DataCollector::field_switch_record.end()) {
            __DataCollector::field_switch_record.insert({{disk_id, head_id}, std::vector<std::vector<int>>()});
        }
        __DataCollector::field_switch_record[{disk_id, head_id}].push_back(
            {GlobalVariable::epoch, GlobalVariable::read_frequency[tag][(GlobalVariable::epoch - 1) / 1800 + 1], read_size, (int) (10000 * density)});
    }

    static void flush() {
        
        if constexpr (!submit) {
            data_collector.clear();
            
            static int req_id = 1;
            while (__DataCollector::request_complete_time[req_id] != -1) {
                __DataCollector::TTL[__DataCollector::request_complete_time[req_id] 
                    - __DataCollector::request_create_time[req_id]] ++;
                ++ req_id;
            }
            static int preFlush = 0;
            if (GlobalVariable::epoch - preFlush >= 1800) {
                data_collector << __DataCollector::TTL << newLine;
                preFlush = GlobalVariable::epoch;
            }

            data_collector << "域切换日志:" << newLine;
            for (auto& entry: __DataCollector::field_switch_record) {
                data_collector << "    磁盘id: " << entry.first.first << " 磁头id: " 
                    << entry.first.second << "   {时间,读请求频率,有效读取长度,请求密度}";
                for (std::vector<int> info: entry.second) {
                    data_collector << " {" << info[0] << "," << info[1]
                        << "," << info[2] << "," << info[3] << "}";
                }
                data_collector << newLine;
            }
            static Debug field_switch_data("field_switch_data", true);
            field_switch_data.clear();
            for (auto& entry: __DataCollector::field_switch_record) {

                for (auto p = entry.second.begin(); p != entry.second.end(); ++ p) {
                    std::vector<int>& info = *p;
                    if ((p + 1) != entry.second.end()) {
                        field_switch_data << info[1]
                            << " " << info[2] << " " << info[3] << " " << (*(p + 1))[0] - info[0];
                        field_switch_data << "   |" << "disk_id: " << entry.first.first << " head_id: " << entry.first.second
                            << " epoch: " << info[0];
                        field_switch_data << newLine;
                    }

                }
            }

            
        }
    }
    static void save_state_flush() {
        static Debug obj_save_state("obj_save_state", true);
        obj_save_state.clear();
        obj_save_state << "对象正常保存数: " << Info::normal_save_count << newLine;
        obj_save_state << "对象备份保存数: " << Info::backup_save_count << newLine;
        if (Info::normal_save_count + Info::backup_save_count != 0) {
            obj_save_state << "对象正常保存率: " 
            << 100.0 * (double) Info::normal_save_count / (Info::normal_save_count + Info::backup_save_count) 
            << "%" << newLine;
        }

    }

    


};