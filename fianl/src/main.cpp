

#include "debug.h"
#include "io.h"
#include "wrong.h"
#include "disk.h"
#include "time_check.h"
#include "object.h"
#include "tool.h"
#include "info_view.h"
#include "data_collector.h"
#include "field_group.h"
#include "disk_tool.h"
#include "tag_schedule.h"
#include "centralized_disk.h"
#include "functions.h"
#include "field.hpp"
#include "real_tag.h"
#include "online_tag_schedule.h"


using namespace std;


unordered_set<int> obj_discard;
void obj_discard_compute() {
    std::priority_queue<
        std::pair<double,int>,
        std::vector<std::pair<double,int>>,
        std::greater<std::pair<double,int>>
    > pq;
    for (int obj_id = 1; obj_id < GlobalInfo::obj_tags.size(); ++ obj_id) {
        int create_time = GlobalInfo::obj_create_time[obj_id];
        int delete_time = GlobalInfo::obj_delete_time[obj_id];
        int req_count = GlobalInfo::obj_req_time[obj_id].size();
        pq.push({(double) req_count / (delete_time - create_time), obj_id});
    }
    int size = pq.size();
    int discard_count = discard_rate * size;
    for (int _ = 0; _ < discard_count; ++ _) {
        int obj_id = pq.top().second;
        obj_discard.insert(obj_id);
        pq.pop();
    }
}


// 对象删除事件处理函数
void delete_action() {
    vector<int> input = delete_action_input();
    GlobalInfo::delete_record[GlobalVariable::epoch] = input;
    for(int object_id : input) {
        EXCEPTION_CHECK(GlobalVariable::objects.find(object_id) == GlobalVariable::objects.end(), "要删除的对象不存在于objects中。");
        EXCEPTION_CHECK(GlobalInfo::obj_delete_time.size() <= object_id, "delete_action: obj_delete_time容量不足");
        GlobalInfo::obj_delete_time[object_id] = GlobalVariable::epoch;
        Object* obj = GlobalVariable::objects[object_id];
        GlobalVariable::objects.erase(object_id);
        if (obj -> tag != 0) {
            EXCEPTION_CHECK(obj -> tag <= 0 || obj -> tag > GlobalInfo::M, "write_action: 对象标签异常");
            TagPredict::tag_obj_holding[obj -> tag] -> update(GlobalVariable::epoch, -1);
        } 
        TagPredict::easyOnlineTagSchedule -> obj_delete(object_id, obj -> tag);
        GlobalVariable::tag_count[obj -> tag] -= obj -> size;
        delete obj;
    }
    delete_action_output();
}






// 对象写入事件处理函数
void write_action() {
    std::vector<write_event_inform> input = write_action_input();
    GlobalInfo::write_record[GlobalVariable::epoch] = input;
    for (write_event_inform& info: input) {
        EXCEPTION_CHECK(GlobalVariable::objects.find(info.id) != GlobalVariable::objects.end(), "插入的对象已存在于objects中");
        Object* obj = new Object(info.id, info.tag, info.size);
        GlobalVariable::tag_count[obj -> tag] += obj -> size;
        while (GlobalInfo::obj_tags.size() <= obj -> id) {
            GlobalInfo::obj_tags.push_back(0);
        }
        while (TagPredict::predict.size() <= obj -> id) {
            TagPredict::predict.push_back(0);
        }
        while (GlobalInfo::obj_size.size() <= obj -> id) {
            GlobalInfo::obj_size.push_back(0);
        }
        while (GlobalInfo::obj_create_time.size() <= obj -> id) {
            GlobalInfo::obj_create_time.push_back(0);
        }
        while (GlobalInfo::obj_delete_time.size() <= obj -> id) {
            GlobalInfo::obj_delete_time.push_back(GlobalInfo::T + EXTRA_TIME);
        }
        GlobalInfo::obj_tags[obj -> id] = obj -> tag;
        TagPredict::predict[obj -> id] = obj -> tag;
        GlobalInfo::obj_size[obj -> id] = obj -> size;
        GlobalInfo::obj_create_time[obj -> id] = GlobalVariable::epoch;
        GlobalVariable::objects.emplace(info.id, obj);
        if (obj -> tag != 0) {
            EXCEPTION_CHECK(obj -> tag <= 0 || obj -> tag > GlobalInfo::M, "write_action: 对象标签异常");
            TagPredict::tag_obj_holding[obj -> tag] -> update(GlobalVariable::epoch, 1);
        } 
        TagPredict::easyOnlineTagSchedule -> obj_create(obj -> id, obj -> tag);
        write_output.id = info.id;
        bool sussess_flag = false;
        int obj_tag = obj -> tag;
        if (obj_tag == 0) {
            obj_tag = rand() % 16 + 1;
        }
        int head_choice = rand() % HyperParameters::head_num;
        int disk_start = rand() % GlobalInfo::N;
        for (int delta = 0; delta < GlobalInfo::N; ++ delta) {
            int disk_id = (disk_start + delta) % GlobalInfo::N;
            try {
                GlobalVariable::cetralized_disks[disk_id] -> save(obj);
                obj -> addDisk(GlobalVariable::cetralized_disks[disk_id]);

                sussess_flag = true;
            } catch (const OutOfDiskSpace_Exception& e) {
                continue;
            }
            if (sussess_flag) break;
        }
        if (!sussess_flag){
            result << newLine << "epoch" << GlobalVariable::epoch << "  " << obj ->tag << "  " << obj -> size;         
            throw Exception("无法找到足够空间的域存储对象呀。");
        }

        int backup_count = 0;
        disk_start = rand() % GlobalInfo::N;
        for (int delta = 0; delta < GlobalInfo::N; ++ delta) {
            int disk_id = (disk_start + delta) % GlobalInfo::N;
            try {
                GlobalVariable::cetralized_disks[disk_id] -> save_backup(obj);
                obj -> addDisk(GlobalVariable::cetralized_disks[disk_id]);
                
                backup_count ++;
                sussess_flag = true;
            } catch (const OutOfDiskSpace_Exception& e) {
                continue;
            }
            if (backup_count >= 2) break;
        }

        if (backup_count < 2) {
            throw Exception("无法找到足够的备份空间存储对象");
        }
        write_print();

    }

}


std::map<int, std::vector<int>> tag_backup;
void tag_backup_compute() {
    tag_backup.clear();
    std::unordered_map<int, std::vector<int>> req_count;
    std::function<int(int)> subscript_convert = [&](int time) -> int {
        return (time - 1) / 100;
    };
    int size = 1 + subscript_convert(GlobalInfo::T + EXTRA_TIME);
    for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
        req_count[tag] = std::vector<int> (size, 0);
    }
    for (int obj_id = 1; obj_id <= GlobalInfo::obj_tags.size(); ++ obj_id) {
        int obj_tag = GlobalInfo::obj_tags[obj_id];
        if (obj_tag == 0) continue;
        for (const int& req_time: GlobalInfo::obj_req_time[obj_id]) {
            req_count[obj_tag][subscript_convert(req_time)] ++;
        }
    }


    int tag_num = GlobalInfo::M;
    vector<int> temp;
    for (int tag = 1; tag <= tag_num; ++ tag) temp.push_back(tag);
    for (int tag = 1; tag <= tag_num; ++ tag) tag_backup[tag] = temp;
    for (int tag = 1; tag <= tag_num; ++ tag) {
        auto& vec = tag_backup[tag];
        Tool::shuffleVector(vec);
        vec[0] = tag;

        sort(vec.begin(), vec.end(), [&](int a, int b) {
            return Tool::cosine_similarity(req_count[tag], req_count[a])
                > Tool::cosine_similarity(req_count[tag], req_count[b]);
        });

    }
    debug << "tag_backup_compute:" << newLine;
    for (int tag = 1; tag <= tag_num; ++ tag){
        debug << "tag:" << tag << "  " << tag_backup[tag] << newLine;
    }
}

FieldGroup* discard_field = new FieldGroup({nullptr, nullptr, nullptr}, 0);
unordered_map<int, vector<FieldGroup*>> diskByLabel;
void secondry_write_action() {
    std::vector<write_event_inform> input = GlobalInfo::write_record[GlobalVariable::epoch];
    for (write_event_inform& info: input) {
        EXCEPTION_CHECK(GlobalVariable::objects.find(info.id) != GlobalVariable::objects.end(), "插入的对象已存在于objects中");
        info.tag = GlobalInfo::obj_tags[info.id];

        Object* obj = new Object(info.id, info.tag, info.size);
        GlobalVariable::tag_count[obj -> tag] += obj -> size;
        GlobalVariable::objects.emplace(info.id, obj);
        write_output.id = info.id;
        bool sussess_flag = false;
        int obj_tag = obj -> tag;
        if (obj_tag == 0) {
            obj_tag = rand() % 16 + 1;
        }
        if (obj_discard.find(info.id) == obj_discard.end()) 
        for (int tag: tag_backup[obj_tag]) {
            vector<FieldGroup*>& field_candidate = diskByLabel[tag];
            std::vector<double> scores;
            for (FieldGroup* fs: field_candidate) {
                scores.push_back(-(*fs)[0] -> get_disk() -> get_profit_sum() - (*fs)[1] -> get_disk() -> get_profit_sum());
            }
            for (FieldGroup* fs: field_candidate) {
                try {
                    fs -> save(obj);
                    write_print();
                    sussess_flag = true;
                    if (tag == obj -> tag) {
                        ++ Info::normal_save_count;
                    } else {
                        ++ Info::backup_save_count;
                    }
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
        else {
            discard_field -> save(obj);
            obj -> set_request_permit(false);
            write_print();
            sussess_flag = true;
        }



        if (!sussess_flag){
            result << newLine << "epoch" << GlobalVariable::epoch << "  " << obj ->tag << "  " << obj -> size;         
            throw Exception("无法找到足够空间的域存储对象呀。");
        }
    }

}

// 对象读取事件处理函数
void read_action() {
    int request_num = ReadActionInput::size();
    std::vector<read_event_id> input;
    for (int i = 0; i < request_num; i ++) {
        read_event_id read_in = ReadActionInput::get();
        input.push_back(read_in);
        EXCEPTION_CHECK(GlobalVariable::objects.find(read_in.obj_id) == GlobalVariable::objects.end(), 
            "要读取的对象不在objects中。");
        GlobalInfo::obj_req_time[read_in.obj_id].push_back(GlobalVariable::epoch);
        
        Object* obj = GlobalVariable::objects[read_in.obj_id];
        TagPredict::easyOnlineTagSchedule -> add_request(read_in.obj_id, obj -> tag);
        DataCollector::request_tag(obj -> tag);
        GlobalVariable::req_queue.push({GlobalVariable::epoch, read_in.req_id, read_in.obj_id});
        while (GlobalInfo::req_count[obj -> tag].size() <= (GlobalVariable::epoch - 1) / 100) {
            GlobalInfo::req_count[obj -> tag].push_back(0);
        }
        GlobalInfo::req_count[obj -> tag][(GlobalVariable::epoch - 1) / 100] ++;
        obj -> add_request(read_in.req_id);
    }
    GlobalInfo::read_record[GlobalVariable::epoch] = input;
    vector<double> temp_profit;
    for(CentralizedDisk* d: GlobalVariable::cetralized_disks) {
        TimeCheck::start("step");
        for (int head_id = 0; head_id < HyperParameters::head_num; head_id ++) {
            d -> step(head_id);
        }
        TimeCheck::end("step");
    }

    

    
    while (!GlobalVariable::req_queue.empty()) {
        auto req_front = GlobalVariable::req_queue.front();
        if (GlobalVariable::epoch - req_front.epoch >= 105) {
            if (!GlobalVariable::read_complete[req_front.req_id]) {
                Object* obj = GlobalVariable::objects[GlobalVariable::req_queue.front().obj_id];
                read_busy.push_back(req_front.req_id);
                GlobalVariable::read_complete[req_front.req_id] = true;
                DataCollector::request_complete(req_front.req_id);
            }
            GlobalVariable::req_queue.pop();
        } else {
            break;
        }
    }

    read_print();
}

// 对象读取事件处理函数
void secondry_read_action() {
    std::vector<read_event_id> input = GlobalInfo::read_record[GlobalVariable::epoch];
    for (int i = 0; i < input.size(); i ++) {
        read_event_id read_in = input[i];
        EXCEPTION_CHECK(GlobalVariable::objects.find(read_in.obj_id) == GlobalVariable::objects.end(), "要读取的对象不在objects中。");

        Object* obj = GlobalVariable::objects[read_in.obj_id];
        DataCollector::request_tag(obj -> tag);
        GlobalVariable::req_queue.push({GlobalVariable::epoch, read_in.req_id, read_in.obj_id});
        //GlobalVariable::__req_by_obj.push(read_in.obj_id);
        obj -> add_request(read_in.req_id);
    }
    GlobalInfo::read_record[GlobalVariable::epoch] = input;
    vector<double> temp_profit;
    for(Disk* d: GlobalVariable::disks) {
        TimeCheck::start("step");
        double priProfit = d -> profit_amount;
        for (int head_id = 0; head_id < HyperParameters::head_num; head_id ++) {
            d -> step(head_id);
        }
        temp_profit.push_back(d -> profit_amount - priProfit);
        TimeCheck::end("step");
    }

    
    if (GlobalVariable::epoch % 1800 == 0) {
        for (double p: temp_profit) {
            perform << p << "      ";
        }     
        perform << newLine;
    }
    
    while (!GlobalVariable::req_queue.empty()) {
        auto req_front = GlobalVariable::req_queue.front();
        if (GlobalVariable::epoch - req_front.epoch >= 105) {
            if (!GlobalVariable::read_complete[req_front.req_id]) {
                Object* obj = GlobalVariable::objects[GlobalVariable::req_queue.front().obj_id];
                read_busy.push_back(req_front.req_id);
                GlobalVariable::read_complete[req_front.req_id] = true;
                DataCollector::request_complete(req_front.req_id);
            }
            GlobalVariable::req_queue.pop();
        } else {
            break;
        }
    }

    read_print();
}

void recycle_action() {
    for (CentralizedDisk* disk: GlobalVariable::cetralized_disks) disk -> recycle();
    recycle_input();
    recycle_print();
}

void secondry_recycle_action() {
    for (Disk* disk: GlobalVariable::disks) disk -> recycle();
    recycle_print();
}


// 对象删除事件处理函数
void secondry_delete_action() {
    vector<int> input = GlobalInfo::delete_record[GlobalVariable::epoch];
    for(int object_id : input) {
        EXCEPTION_CHECK(GlobalVariable::objects.find(object_id) == GlobalVariable::objects.end(), "要删除的对象不存在于objects中。");
        Object* obj = GlobalVariable::objects[object_id];
        GlobalVariable::objects.erase(object_id);
        GlobalVariable::tag_count[obj -> tag] -= obj -> size;
        delete obj;
    }
    delete_action_output();
}









void field_distribution(vector<Disk*>& disk_list) {
    std::pair<std::vector<std::vector<field_info>>, std::vector<field_info>> field_dis = 
        DiskTool::field_distribution(GlobalInfo::N, GlobalInfo::V, GlobalInfo::M);
    std::vector<std::vector<field_info>> field_group = field_dis.first;
    std::vector<field_info> backup_field = field_dis.second;
    debug << "{disk_id, tag, start_pos, size}" << newLine;
    for (int field_index = 0; field_index < field_group.size(); ++ field_index) {
        debug << "域id: " << field_index << newLine;
        
        for (field_info fi: field_group[field_index]) {
            debug << Tab << fi.disk_id << " " << fi.tag << " " << fi.start_pos << " " << fi.size << newLine;
            break;
        }
    }
    debug << "备份域: " << newLine;
    debug << "{disk_id, tag, start_pos, size}" << newLine;
    for (field_info fi: backup_field) {
        debug << Tab << fi.disk_id << " " << fi.tag << " " << fi.start_pos << " " << fi.size << newLine;
    }

    for (std::vector<field_info>& fg: field_group) {
        field_info fi1 = fg[0];
        field* f1 = GlobalVariable::disks[fi1.disk_id] -> add_field(fi1.tag, fi1.start_pos, fi1.size, FieldType::mainField);
        field_info fi2 = fg[1];
        field* f2 = GlobalVariable::disks[fi2.disk_id] -> add_field(fi2.tag, fi2.start_pos, fi2.size, FieldType::assistField);
        diskByLabel[fi1.tag].push_back(new FieldGroup({f1, f2, nullptr}, fi1.tag));
    }

    for (field_info& fi: backup_field) {
        field* f = GlobalVariable::disks[fi.disk_id] -> add_field(fi.tag, fi.start_pos, fi.size, FieldType::backupField);
        FieldGroup::add_backup_field(f);
    }
}
void secondry_field_distribution(vector<Disk*>& disk_list) {
    std::pair<std::vector<std::vector<field_info>>, std::vector<field_info>> field_dis = 
        DiskTool::secondry_field_distribution(GlobalInfo::N, GlobalInfo::V, GlobalInfo::M);
    std::vector<std::vector<field_info>> field_group = field_dis.first;
    std::vector<field_info> backup_field = field_dis.second;
    debug << "{disk_id, tag, start_pos, size}" << newLine;
    for (int field_index = 0; field_index < field_group.size(); ++ field_index) {
        debug << "域id: " << field_index << newLine;
        
        for (field_info fi: field_group[field_index]) {
            debug << Tab << fi.disk_id << " " << fi.tag << " " << fi.start_pos << " " << fi.size << newLine;
            break;
        }
    }
    debug << "备份域: " << newLine;
    debug << "{disk_id, tag, start_pos, size}" << newLine;
    for (field_info fi: backup_field) {
        debug << Tab << fi.disk_id << " " << fi.tag << " " << fi.start_pos << " " << fi.size << newLine;
    }

    for (std::vector<field_info>& fg: field_group) {
        field_info fi1 = fg[0];
        field* f1 = GlobalVariable::disks[fi1.disk_id] -> add_field(fi1.tag, fi1.start_pos, fi1.size, FieldType::mainField);
        field_info fi2 = fg[1];
        field* f2 = GlobalVariable::disks[fi2.disk_id] -> add_field(fi2.tag, fi2.start_pos, fi2.size, FieldType::assistField);
        diskByLabel[fi1.tag].push_back(new FieldGroup({f1, f2, nullptr}, fi1.tag));
    }

    for (field_info& fi: backup_field) {
        field* f = GlobalVariable::disks[fi.disk_id] -> add_field(fi.tag, fi.start_pos, fi.size, FieldType::backupField);
        FieldGroup::add_backup_field(f);
    }
}

void tag_predict_complete() {
    //TagSchedule* ts = new GreedyTagSchedule();
    //TagSchedule* ts = new LogLikelihoodTagSchedule();
    /*
    TagSchedule* ts = new ImproveGreedyTagSchedule();
    std::vector<std::pair<int, int>> schedule = ts -> getSchedule(GlobalInfo::obj_tags.size() - 1,
        [&](int obj_id) -> int {
            return GlobalInfo::obj_tags[obj_id];
        }, 
        [&](int obj_id) -> std::vector<int> {
            return GlobalInfo::obj_req_time[obj_id];
        },
        [&](int obj_id) -> int {
            return GlobalInfo::obj_create_time[obj_id];
        },
        [&](int obj_id) -> int {
            return std::min(GlobalVariable::epoch, GlobalInfo::obj_delete_time[obj_id]);
        }
    );*/

    std::vector<std::pair<int, int>> schedule = TagPredict::easyOnlineTagSchedule -> getSchedule();
    for (const std::pair<int, int>& tag_pre: schedule) {
        TagPredict::predict[tag_pre.first] = tag_pre.second;
    }

    if constexpr (!submit) {
        int predict_correct = 0;
        int predict_wrong = 0;
        for (const std::pair<int, int>& tag_pre: schedule) {
            int obj_id = tag_pre.first;
            int excepted = tag_pre.second;
            int actual = GlobalInfo::real_tag[obj_id];
            EXCEPTION_CHECK(actual <= 0 || actual > GlobalInfo::M, "对象标签异常");
            //debug << "actual: " << actual<<" " << obj_id<< newLine;
            if (excepted == actual) {
                ++ predict_correct;
            } else {
                ++ predict_wrong;
            }
        }
        result << "标签预测结果: " << newLine;
        result << Tab << "predict_correct: " << predict_correct << newLine;
        result << Tab << "predict_wrong: " << predict_wrong << newLine;
        result << Tab << "准确率: " << ((double) predict_correct) / (predict_correct + predict_wrong) << newLine;
    }

}

void tag_predict() {
    //TagSchedule* ts = new GreedyTagSchedule();
    //TagSchedule* ts = new LogLikelihoodTagSchedule();
    TagSchedule* ts = new ImproveGreedyTagSchedule();
    std::vector<std::pair<int, int>> schedule = ts -> getSchedule(GlobalInfo::obj_tags.size() - 1,
        [&](int obj_id) -> int {
            return GlobalInfo::obj_tags[obj_id];
        }, 
        [&](int obj_id) -> std::vector<int> {
            return GlobalInfo::obj_req_time[obj_id];
        },
        [&](int obj_id) -> int {
            return GlobalInfo::obj_create_time[obj_id];
        },
        [&](int obj_id) -> int {
            return GlobalInfo::obj_delete_time[obj_id];
        }
    );
    for (const std::pair<int, int>& tag_pre: schedule) {
        EXCEPTION_CHECK(GlobalInfo::obj_tags[tag_pre.first] != 0, "tag_predict: 对象标签异常");
        GlobalInfo::obj_tags[tag_pre.first] = tag_pre.second;
    }
    if constexpr (!submit) {
        int predict_correct = 0;
        int predict_wrong = 0;
        for (const std::pair<int, int>& tag_pre: schedule) {
            int obj_id = tag_pre.first;
            int excepted = tag_pre.second;
            int actual = GlobalInfo::real_tag[obj_id];
            EXCEPTION_CHECK(actual <= 0 || actual > GlobalInfo::M, "对象标签异常");
            if (excepted == actual) {
                ++ predict_correct;
            } else {
                ++ predict_wrong;
            }
        }
        result << "标签预测结果: " << newLine;
        result << Tab << "predict_correct: " << predict_correct << newLine;
        result << Tab << "predict_wrong: " << predict_wrong << newLine;
        result << Tab << "准确率: " << ((double) predict_correct) / (predict_correct + predict_wrong) << newLine;
    }

}

// ------------------------- 资源清理 -------------------------
void clean() {
    diskByLabel.clear();
    FieldGroup::backup_field_clear();
    for (auto& entry: GlobalVariable::objects) {
        //delete entry.second;
    }    
    read_out = read_action_output(GlobalInfo::N);
    for (Disk* disk: GlobalVariable::disks) {
        //delete disk;
    }

    GlobalVariable::disks.clear();
    for(int i = 0; i < GlobalInfo::N; i ++) GlobalVariable::disks.emplace_back(new Disk(
        i, GlobalInfo::V, GlobalInfo::G, GlobalInfo::K2));
    std::fill(GlobalVariable::read_complete.begin(), GlobalVariable::read_complete.end(), false);
    GlobalVariable::req_queue = std::queue<GlobalVariable::__req_record>();
    
    //GlobalVariable::__req_by_obj = std::queue<int>();
    GlobalVariable::objects.clear();
    GlobalVariable::all_field_group.clear();
    GlobalVariable::score = 0;
    secondry_field_distribution(GlobalVariable::disks);
    tag_backup_compute();
    
    
}

// ------------------------- 主函数 -------------------------
int main() {
    srand(3);
    TimeCheck::start("main");
    GlobalVariable::epoch = 1;
    TimeCheck::start("pretreatment");
    pretreatment();
    TimeCheck::end("pretreatment");
    GlobalVariable::tag_count = vector<int> (GlobalInfo::M + 1, 0);
    GlobalInfo::delete_record = vector<vector<int>> (GlobalInfo::T + EXTRA_TIME + 1);
    GlobalInfo::write_record = vector<std::vector<write_event_inform>> (GlobalInfo::T + EXTRA_TIME + 1);
    GlobalInfo::read_record = vector<std::vector<read_event_id>> (GlobalInfo::T + EXTRA_TIME + 1);
    TagPredict::easyOnlineTagSchedule = new EasyOnlineTagSchedule();
    GlobalInfo::real_tag = loadRealTag();
    for (int tag = 1; tag <= GlobalInfo::M; ++ tag) {
        TagPredict::tag_req_count[tag] = vector<int> (1 + TagPredict::subscript_convert(GlobalInfo::T + EXTRA_TIME), 0);
        TagPredict::tag_obj_holding[tag] = new FenwickTree(1 + GlobalInfo::T + EXTRA_TIME);
        GlobalInfo::req_count[tag] = std::vector<int> ();
    }

    read_out = read_action_output(GlobalInfo::N);

    for(int i = 0; i < GlobalInfo::N; i ++) GlobalVariable::cetralized_disks.emplace_back(new CentralizedDisk(
        i, GlobalInfo::V, GlobalInfo::G, GlobalInfo::K1));

    //field_distribution(GlobalVariable::disks);
    //tag_backup_compute();
    TimeCheck::flush();
    //debug << InfoView::tag_dis_view(diskByLabel) << newLine;     
    // 主循环：对每个时间片（共 T+EXTRA_TIME 个时间片）依次处理各交互事件
    

    for (; GlobalVariable::epoch <= GlobalInfo::T + EXTRA_TIME; GlobalVariable::epoch ++) {
        
        debugFlush();
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

        for (Object* obj: profit_update) {
            obj -> profit_flush();
        }
        profit_update.clear();
        //field_group_adjust();
        if ((GlobalVariable::epoch >= 60000) && (GlobalVariable::epoch % 10 == 0)) {
            //backup_compute();
        }
        if (GlobalVariable::epoch % 1800 == 0) {
            tag_predict_complete();
            //request_permit_adjust();
            TimeCheck::start("recycle_action");
            recycle_action();
            TimeCheck::end("recycle_action");
            TimeCheck::flush();

            //DataCollector::flush();
            //DataCollector::head_move_flush();
            //DataCollector::request_tag_flush();
            //DataCollector::field_group_lock_flush();
            //DataCollector::save_state_flush();

            for (Disk* d: GlobalVariable::disks) {
                d -> busy_adjust();
            }
        }
    }

    tag_predict();
    extra_info();
    //obj_discard_compute();

    clean();

    
    profit_update.clear();
    for (GlobalVariable::epoch = 1; GlobalVariable::epoch <= GlobalInfo::T + EXTRA_TIME; GlobalVariable::epoch ++) {
        debugFlush();
        TimeCheck::start("timestamp_action");
        timestamp_action(); // 处理时间片对齐事件
        TimeCheck::end("timestamp_action");
        TimeCheck::start("delete_action");
        secondry_delete_action();    // 处理对象删除事件
        TimeCheck::end("delete_action");
        TimeCheck::start("write_action");
        secondry_write_action();     // 处理对象写入事件
        TimeCheck::end("write_action");
        TimeCheck::start("read_action");
        secondry_read_action();      // 处理对象读取事件
        TimeCheck::end("read_action");
        for (Object* obj: profit_update) {
            obj -> profit_flush();
        }

        profit_update.clear();
        //field_group_adjust();
        if ((GlobalVariable::epoch >= 60000) && (GlobalVariable::epoch % 10 == 0)) {
            //backup_compute();
        }
        if (GlobalVariable::epoch % 1800 == 0) {
            //request_permit_adjust();
            TimeCheck::start("recycle_action");
            secondry_recycle_action();
            TimeCheck::end("recycle_action");
            TimeCheck::flush();

            //DataCollector::flush();
            //DataCollector::head_move_flush();
            //DataCollector::request_tag_flush();
            //DataCollector::field_group_lock_flush();
            //DataCollector::save_state_flush();

            for (Disk* d: GlobalVariable::disks) {
                EXCEPTION_CHECK(d == nullptr, "for (Disk* d: GlobalVariable::disks): d为空")
                d -> busy_adjust();
            }
        }
    }
    TimeCheck::end("main");
    clean();
    result << "score" << GlobalVariable::score << newLine;
    TimeCheck::clean();
    return 0;
}
