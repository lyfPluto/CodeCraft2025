#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <set>

#include "debug.h"
#include "io.h"
#include "wrong.h"
#include "disk.h"
#include "time_check.h"
#include "object.h"
#include "tool.h"
#include "info_view.h"
using namespace std;

// 定义一些常量：
// MAX_DISK_NUM：硬盘最大数量（题目规定上限10块，数组下标从1开始所以多1）
// MAX_DISK_SIZE：每块硬盘的最大存储单元数（16384个，加1用于下标1起计）
// MAX_REQUEST_NUM：读取请求的最大数量（30000000个，加1）
// MAX_OBJECT_NUM：对象的最大数量（100000个，加1）
// REP_NUM：每个对象需要存储的副本数，题目要求3个副本
// FRE_PER_SLICING：每个时间片段包含的时间单位数，1800
// EXTRA_TIME：额外的时间片数量（105个时间片）
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define EXTRA_TIME (105)



template<typename T>
T& getRandomElement(std::vector<T>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector is empty.");
    }
    // 建议使用静态变量来保持随机数引擎的状态（防止每次调用都重新种子）
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, vec.size() - 1);
    int randomIndex = dis(gen);
    return vec[randomIndex];
}











unordered_map<int, Object*> objects;

// 对象删除事件处理函数
void delete_action() {
    vector<int> input = delete_action_input();
    std::vector<int> ans;
    for(int object_id : input) {
        EXCEPTION_CHECK(objects.find(object_id) == objects.end(), "要删除的对象不存在于objects中。");
        Object* obj = objects[object_id];
        objects.erase(object_id);
        for(int undone_req: obj -> get_undone_request()) ans.push_back(undone_req);
        delete obj;
    }
    delete_action_output(ans);
}

vector<field*> backup_field;
void obj_save(Object* obj, tuple<field*, field*, field*> fields) {

    // 使用 lambda 表达式作为自定义比较函数进行排序
    std::sort(backup_field.begin(), backup_field.end(), 
        [](const field* a, const field* b) {
            return a->get_remain_empty() > b->get_remain_empty();  // 从大到小排序
            //return a -> get_disk() -> remainEmpty() > b -> get_disk() -> remainEmpty();
        });
    int backup_index = 0;
    unordered_set<int> disk_choice;
    vector<field*> field_choice;

    if (get<0>(fields) == nullptr) {
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
        if (disk_choice.find(get<0>(fields) -> get_disk() -> id) != disk_choice.end()) {
            throw OutOfDiskSpace_Exception();
        }
        if (get<0>(fields) -> get_remain_empty() < obj -> size) {
            throw OutOfDiskSpace_Exception();
        }
        field_choice.push_back(get<0>(fields));
        disk_choice.insert(get<0>(fields) -> get_disk() -> id);
    }
    if (get<1>(fields) == nullptr) {
        
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
        if (disk_choice.find(get<1>(fields) -> get_disk() -> id) != disk_choice.end()) {
            throw OutOfDiskSpace_Exception();
        }
        if (get<1>(fields) -> get_remain_empty() < obj -> size) {
            throw OutOfDiskSpace_Exception();
        }
        field_choice.push_back(get<1>(fields));
        disk_choice.insert(get<1>(fields) -> get_disk() -> id);
    }
    if (get<2>(fields) == nullptr) {
        
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
        if (disk_choice.find(get<2>(fields) -> get_disk()-> id) != disk_choice.end()) {
            throw OutOfDiskSpace_Exception();
        }
        if (get<2>(fields) -> get_remain_empty() < obj -> size) {
            throw OutOfDiskSpace_Exception();
        }
        field_choice.push_back(get<2>(fields));
        disk_choice.insert(get<2>(fields) -> get_disk() -> id);
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


unordered_map<int, vector<tuple<field*, field*, field*>>> diskByLabel;
// 对象写入事件处理函数
void write_action() {

    std::vector<write_event_inform> input = write_action_input();

    for (write_event_inform& info: input) {

        EXCEPTION_CHECK(objects.find(info.id) != objects.end(), "插入的对象已存在于objects中");
        Object* obj = new Object(info.id, info.tag, info.size);
        objects.emplace(info.id, obj);
        write_output.id = info.id;
        bool sussess_flag = false;

        for (int _tag = 0; _tag < M; _tag ++) {
            int tag = (_tag + obj -> tag - 1) % M + 1;
            vector<tuple<field*, field*, field*>>& field_candidate = diskByLabel[tag];
            Tool::shuffleVector(field_candidate);
            /*
            vector<double> choice_scores;
            for (tuple<field*, field*, field*>& fs: field_candidate) {
                double score = 10000000;
                if (get<0>(fs)) score = min(score, (double) get<0>(fs) -> get_disk() -> remainEmpty());
                if (get<1>(fs)) score = min(score, (double) get<1>(fs) -> get_disk() -> remainEmpty());
                if (get<2>(fs)) score = min(score, (double) get<2>(fs) -> get_disk() -> remainEmpty());
                //if (get<0>(fs)) score = min(score, (double) get<0>(fs) -> get_remain_empty());
                //if (get<1>(fs)) score = min(score, (double) get<1>(fs) -> get_remain_empty());
                //if (get<2>(fs)) score = min(score, (double) get<2>(fs) -> get_remain_empty());
                choice_scores.push_back(score);
            }
            tuple<field*, field*, field*>& choice = Tool::highestScoreChoice(field_candidate, choice_scores);
            try {
                obj_save(obj, choice);
                write_print();
                sussess_flag = true;
                break;
            } catch (const OutOfDiskSpace_Exception& e) {
                if (tag == obj -> tag) {
                    Warning << "当前标签的域空间不足，使用其他标签的域代理存储。";
                }
                Warning << "磁盘容量不足。" << newLine;
                continue;
            }
            if (sussess_flag) break;*/
            for (tuple<field*, field*, field*>& fs: field_candidate) {
                try {
                    obj_save(obj, fs);
                    write_print();
                    sussess_flag = true;
                    break;
                } catch (const OutOfDiskSpace_Exception& e) {
                    if (tag == obj -> tag) {
                        Warning << "当前标签的域空间不足，使用其他标签的域代理存储。";
                    }
                    Warning << "磁盘容量不足。" << newLine;
                    continue;
                }
            }
            if (sussess_flag) break;
        }

        if (!sussess_flag){
            result << newLine << "epoch" << epoch << "  " << obj ->tag << "  " << obj -> size;
            debug << InfoView::tag_dis_view(diskByLabel) << newLine;            
            throw Exception("无法找到足够空间的域存储对象呀。");
        }
    }

}


// 对象读取事件处理函数
void read_action() {
    TimeCheck::start("read_action_input");
    TimeCheck::end("read_action_input");
    int request_num = ReadActionInput::size();
    for (int i = 0; i < request_num; i ++) {

        read_event_id read_in = ReadActionInput::get();
        TimeCheck::start("read_action_Exception");
        EXCEPTION_CHECK(objects.find(read_in.obj_id) == objects.end(), "要读取的对象不在objects中。");

        TimeCheck::end("read_action_Exception");
        Object* obj = objects[read_in.obj_id];

        TimeCheck::start("add_request");
        obj -> add_request(read_in.req_id);
        TimeCheck::end("add_request");
    }
    vector<double> temp_profit;
    for(Disk* d: disks) {
        TimeCheck::start("step");
        double priProfit = d -> profit_amount;
        d -> step();
        temp_profit.push_back(d -> profit_amount - priProfit);
        TimeCheck::end("step");
    }
    if (epoch % 100 == 0) {
        for (double p: temp_profit) {
            perform << p << "      ";
        }     
        perform << newLine;
    }

    
    read_print();
}


// ------------------------- 资源清理 -------------------------
void clean() {

    TimeCheck::clean();

}





void field_distribution(vector<Disk*>& disk_list) {
    debug << "start dis" << newLine; 
    vector<pair<int, Disk*>> disk_candidate;
    for (Disk* d: disk_list) {
        disk_candidate.push_back({0, d});
    }
    vector<int> tag_storage(M, 0);
    for (int i = 1; i <= M; i ++) {
        debug << "i: " << i << newLine;
        int storage = 0;
        int max_storage = 0;
        auto& wf = write_frequency[i];
        auto& df = delete_frequency[i];
        int len = wf.size();
        for (int t = 0; t < len; t ++) {
            storage += wf[t];
            storage -= df[t];
            max_storage = max(max_storage, storage);
            //tag_storage[i - 1] = max(max_storage, tag_storage[i - 1]);
            tag_storage[i - 1] += storage;
        } 
        
    }
    long long sum = 0;
    for (int num: tag_storage) sum += num;
    double discount = (((double)N * V) / (3 * sum));
    discount *= 1.05;
    for (int& num: tag_storage) num *= discount;
    debug << "tag_storage: ";
    for (int& num: tag_storage) debug << num << " ";
    debug << newLine;
    int seg_count = N * 4;
    debug << "seg_count: " << seg_count << newLine;
    int min_len = 0, max_len = 1000000;
    while (min_len < max_len - 1) {
        int mid_len = (min_len + max_len) / 2;
        int seg_need = 0;
        for (int i = 0; i < M; i ++) {
            seg_need += (tag_storage[i] - 1)/ mid_len + 1;
        }
        if (seg_need <= seg_count) {
            max_len = mid_len;
        }
        else {
            min_len = mid_len;
        }
    }
    int seg_len = max_len;
    debug << "seg_len: " << seg_len << newLine;
    for (int i = 0; i < M; i ++) {
        diskByLabel.insert({i + 1, vector<tuple<field*, field*, field*>>()});
        vector<int> distribution(N, 0);
        int seg_tag = (tag_storage[i] - 1) / seg_len + 1;
        sort(disk_candidate.begin(), disk_candidate.end(),
        [](const pair<int, Disk*>& a, const pair<int, Disk*>& b) {
            return a.first < b.first;  // 比较 pair.first
        });
        if (seg_tag > N) seg_tag = N;
        int disk_index = 0;
        for (int j = 0; j < seg_tag; j ++) {
            if (disk_index >= N - 1) {
                disk_index = 0;
                sort(disk_candidate.begin(), disk_candidate.end(),
                [](const pair<int, Disk*>& a, const pair<int, Disk*>& b) {
                    return a.first < b.first;  // 比较 pair.first
                });
            }
            if (V - disk_candidate[disk_index + 1].first < tag_storage[i] / seg_tag) {
                disk_index = 0;
                sort(disk_candidate.begin(), disk_candidate.end(),
                [](const pair<int, Disk*>& a, const pair<int, Disk*>& b) {
                    return a.first < b.first;  // 比较 pair.first
                });
            }
            //EXCEPTION_CHECK(N < 2 * seg_tag, "分片数过多，磁盘数不足。");
            int start1 = (disk_candidate[disk_index].first + V) % V;
            int start2 = (disk_candidate[disk_index + 1].first + V) % V;
            field* f1 = disk_candidate[disk_index].second -> add_field(i + 1, start1, tag_storage[i] / seg_tag);
            field* f2 = disk_candidate[disk_index + 1].second -> add_field(i + 1, start2, tag_storage[i] / seg_tag);
            diskByLabel[i + 1].push_back({f1, f2, nullptr});
            bool* owner_flag = new bool(false);
            f1 -> owner_flag = owner_flag;
            f2 -> owner_flag = owner_flag;
            disk_candidate[disk_index].first += tag_storage[i] / seg_tag;
            disk_candidate[disk_index + 1].first += tag_storage[i] / seg_tag;
            disk_index += 2;
        }
    }
    for (int j = 0; j < N; j ++) {
        if (disk_candidate[j].first < V) {
            int start = (disk_candidate[j].first + V) % V;
            field* f = disk_candidate[j].second -> add_field(-1, start, V - disk_candidate[j].first);
            backup_field.push_back(f);
        }
        
    }

    debug << "end dis" << newLine; 

}


// ------------------------- 主函数 -------------------------
int main() {

    TimeCheck::start("test");
    TimeCheck::start("main");
    epoch = 1;
    TimeCheck::start("pretreatment");
    pretreatment();
    TimeCheck::end("pretreatment");
    read_out = read_action_output(N);
    for(int i = 0; i < N; i ++) disks.emplace_back(new Disk(i, V, G));

    field_distribution(disks);
    TimeCheck::end("test");
    TimeCheck::flush();

    debug << InfoView::tag_dis_view(diskByLabel) << newLine;     
    // 主循环：对每个时间片（共 T+EXTRA_TIME 个时间片）依次处理各交互事件
    

    for (; epoch <= T + EXTRA_TIME; epoch ++) {

    
        TimeCheck::start("timestamp_action");
        timestamp_action(); // 处理时间片对齐事件
        TimeCheck::end("timestamp_action");

        TimeCheck::start("delete_action");
        delete_action();    // 处理对象删除事件
        TimeCheck::end("delete_action");
        
        TimeCheck::start("write_action");
        write_action();     // 处理对象写入事件
        TimeCheck::end("write_action");

        TimeCheck::start("read_action");
        read_action();      // 处理对象读取事件
        TimeCheck::end("read_action");


        if constexpr (true) { // 刷新对象的profit
            for (Object* obj: profit_update) {
                obj -> profit_flush();
            }
            profit_update.clear();
        }


        if (epoch % 1800 == 0) {
            TimeCheck::flush();
        }

        if ((!submit) && epoch == 10011) {
            for (Disk* disk: disks) {
                diskLog << disk -> debug_state();
            }
            diskLog << "\n\n\n\n\n\n\n\n\n";
        }
        if ((!submit) && epoch == 30000) {
            for (Disk* disk: disks) {
                diskLog << disk -> debug_state();

            }
            diskLog << "\n\n\n\n\n\n\n\n\n";
        }

    }
    TimeCheck::end("main");

    clean();
    result << "score" << score << newLine;
    
    return 0;
}
