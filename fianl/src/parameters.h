#pragma once

#include <vector>
#include <map>
#include <array>
#include <queue>
#include <unordered_map>

#include "functions.h"




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
#define FRE_PER_SLICING (1800)
class HyperParameters {
public:
    //static constexpr double discountFactor = 0.99;
    static constexpr int head_num = 2;

};

double discard_rate = 0.01;

namespace GlobalInfo {

    int T, M, N, V, G, K1, K2;    
    std::vector<int> obj_tags;
    std::vector<std::vector<int>> delete_record;
    std::vector<std::vector<write_event_inform>> write_record;
    std::vector<std::vector<read_event_id>> read_record;
    std::unordered_map<int, std::vector<int>> obj_req_time;
    std::vector<int> obj_size;
    std::vector<int> obj_create_time;
    std::vector<int> obj_delete_time;
    std::unordered_map<int,int> real_tag;
    std::unordered_map<int, std::vector<int>> req_count;
    
}

namespace TagPredict {
    std::function<int(int)> subscript_convert = [](int time) -> int {
        return (time - 1) / 100;
    };
    std::unordered_map<int, std::vector<int>> tag_req_count;
    // 任意时刻每个标签的对象总大小
    std::unordered_map<int, FenwickTree*> tag_obj_holding;
    std::vector<int> predict;
    EasyOnlineTagSchedule* easyOnlineTagSchedule;
}


namespace GlobalVariable {
    // 全局变量：
    // T: 时间片数量（实际交互时总共 T + EXTRA_TIME 个时间片）
    // M: 对象标签数
    // N: 硬盘个数
    // V: 每个硬盘的存储单元个数
    // G: 每个磁头每个时间片最多消耗的令牌数
    // K: 表每次垃圾回收事件每个硬盘最多的交换存储单元的操作次数 (0 <= K <= 100)

    int epoch = 1;
    double score = 0;    
    std::vector<Disk*> disks;
    std::vector<CentralizedDisk*> cetralized_disks;
    // 代表请求是否完成(请求的对象被删除在对象删除事件中上报，请求读取成功，请求繁忙), 下标从1开始
    std::vector<bool> read_complete(30000000, false);
    
    struct __req_record {
        __req_record(int e, int r, int o) {
            epoch = e;
            req_id = r;
            obj_id = o;
        }
        int epoch;
        int req_id;
        int obj_id;
    };
    // 请求队列: {epoch, req_id, obj_id}
    std::queue<__req_record> req_queue;
    



    // 每个标签的对象总数, 下标从1开始
    std::vector<int> tag_count;

    std::unordered_map<int, Object*> objects;

    std::vector<FieldGroup*> all_field_group;



}








