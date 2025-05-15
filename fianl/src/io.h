#pragma once
#include <cstdio>
#include <vector>
#include <cassert>
#include <cstdarg>
#include <map>
#include <array>

#include <cctype>
#include "debug.h"
#include "parameters.h"
#include "wrong.h"




void pretreatment() {

    // 读取全局参数：时间片数量 T、对象标签数 M、硬盘个数 N、每块硬盘存储单元数 V 以及每个磁头每个时间片可用令牌数 G
    scanf("%d%d%d%d%d%d%d", &GlobalInfo::T, &GlobalInfo::M, &GlobalInfo::N, 
        &GlobalInfo::V, &GlobalInfo::G, &GlobalInfo::K1, &GlobalInfo::K2);
    /**
     * T: 时间片数量，总共有 T+105 个时间片
     * M: 对象标签数
     * N: 硬盘个数
     * V: 每个硬盘的存储单元个数
     * G: 每个磁头每个时间片最多消耗的令牌数
     * K: 每次垃圾回收事件每个硬盘最多的交换存储单元的操作次数
     */

    // 完成全局预处理阶段后输出 "OK" 表示初始化成功
    printf("OK\n");
    fflush(stdout);

}


// ------------------------- 时间片对齐事件 -------------------------

// 时间片对齐事件：接收输入中的时间片同步信号，并原样输出，用于和判题器同步
void timestamp_action()
{
    int timestamp;
    // %*s 用于跳过输入的字符串（比如“TIMESTAMP”），然后读取一个整数
    scanf("%*s%d", &timestamp);
    // 输出时间片对齐信息，格式必须与题目要求一致
    printf("TIMESTAMP %d\n", timestamp);
    IO << "时间片对齐: " << timestamp << newLine;
    fflush(stdout);
    
}


std::vector<int> delete_action_input() {
    int len;
    scanf("%d", &len);
    std::vector<int> ans(len, 0);
    for(int i = 0; i < len; i ++) scanf("%d", &ans[i]);
    if (!ans.empty()) {
        IO << "------------------对象删除 ";
        for (int id : ans) IO << " " << id;
        IO << newLine;        
    }

    return ans;
}


std::vector<int> request_delete;
void delete_action_output() {
    printf("%d\n", request_delete.size());
    for(int num: request_delete) printf("%d\n", num);
    request_delete.clear();
    fflush(stdout);
}


struct write_event_inform {
    int id; // 对象编号
    int size; // 对象大小
    int tag; // 对象标签
};

std::vector<write_event_inform> write_action_input() {
    int len;
    scanf("%d", &len);
    std::vector<write_event_inform> ans(len);
    for(int i = 0; i < len; i ++) {
        scanf("%d%d%d", &ans[i].id, &ans[i].size, &ans[i].tag);
    }
    return ans;

}

struct write_event_pos {
    int id; // 对象编号
    std::vector<int> disk_id; // 硬盘编号
    std::vector<std::vector<int>> disk_pos;
};
write_event_pos write_output;
void write_action_output(write_event_pos ans) {
    printf("%d\n", ans.id);
    IO << "写入阶段:" << newLine;
    for(int i = 0; i < ans.disk_id.size(); i ++) {
        IO << "存储区域选择: " << ans.disk_id[i] + 1;
        printf("%d", ans.disk_id[i] + 1);
        for(int num: ans.disk_pos[i]) {
            printf(" %d", num + 1);
            IO << " " << num + 1;
        }
        printf("\n");
        IO << newLine;
    }
    fflush(stdout);
}
void write_print() {
    write_action_output(write_output);
    write_output = write_event_pos();
};

struct read_event_id { // 对象读取事件交互中的输入
    int req_id;
    int obj_id;
};
class ReadActionInput {
public:
    static int remain;
    static read_event_id get() {
        if (remain == 0) {
            throw Exception("当前时间片获取读请求次数过多，大于读请求总数。");
        }
        remain --;
        read_event_id ans;
        scanf("%d%d", &ans.req_id, &ans.obj_id);
        return ans;
    }
    static int size() {
        if (remain != 0) {
            throw Exception("还有未接收的读请求。");
        }
        
        int n_read;
        scanf("%d", &n_read);
        remain = n_read;
        return n_read;
    }
};
int ReadActionInput::remain = 0;






class read_action_output {
public:

    read_action_output(int _N) : head_ope(_N, std::array<std::string, 2>{{"", ""}}) {}
    void jump(int disk_id, int jump_id, int head_id) {
        if (!head_ope[disk_id][head_id].empty()) {
            throw Exception("同时对一个磁盘调用jump和pass。");
        };
        head_ope[disk_id][head_id] = std::string("j") + " " + std::to_string(jump_id + 1);
    }
    void pass(int disk_id, int head_id) {
        if (head_ope[disk_id][head_id].find('j') != std::string::npos) {
            throw Exception("同时对一个磁盘调用jump和pass。");
        }
        head_ope[disk_id][head_id] += "p";

    }
    void read(int disk_id, int head_id) {
        if (head_ope[disk_id][head_id].find('j') != std::string::npos) {
            throw Exception("同时对一个磁盘调用jump和pass。");
        }
        head_ope[disk_id][head_id] += "r";
    }





    void read_complete(int req_id) { // 读完成事件
        throw Exception("调用已废弃的函数: read_action_output::read_complete");
        //all_read_complete.push_back(req_id);
    }
    void output() {
        for(std::array<std::string, 2> out: head_ope) {
            for (std::string s: out) {
                if (s.find('j') != std::string::npos) printf((s + "\n").c_str());
                else printf((s + "#" + "\n").c_str());
            }
        }
        /*
        printf("%d\n", all_read_complete.size());
        for(int num: all_read_complete) {
            printf("%d\n", num);
        }*/
        fflush(stdout);
    };
private:
    std::vector<std::array<std::string, 2>> head_ope;
    //std::vector<int> all_read_complete;
};


std::vector<int> read_complete;
std::vector<int> read_busy;
read_action_output read_out(0);
void read_print() {
    read_out.output();
    read_out = read_action_output(GlobalInfo::N);
    printf("%d\n", read_complete.size());
    for(int num: read_complete) {
        printf("%d\n", num);
    }
    read_complete.clear();
    printf("%d\n", read_busy.size());
    for(int num: read_busy) {
        printf("%d\n", num);
    }
    read_busy.clear();
    fflush(stdout);
};


void recycle_input() {
    scanf("%*s %*s");
};

std::map<int, std::vector<std::pair<int, int>>> recycle_record;
void recycle_print() {
    //scanf("%*s %*s");
    printf("GARBAGE COLLECTION\n");
    for (int disk_id = 0; disk_id < GlobalInfo::N; disk_id ++) {
        if (recycle_record.find(disk_id) == recycle_record.end()) {
            printf("0\n");
        } else {
            std::vector<std::pair<int, int>>& opes = recycle_record[disk_id];
            printf("%ld\n", opes.size());
            for (std::pair<int, int> ope: opes) {
                printf("%d %d\n", ope.first + 1, ope.second + 1);
            }
        }
    }
    fflush(stdout);
    recycle_record.clear();
}


void extra_info() {
    int n_incre;
    scanf("%d", &n_incre);
    for (int _ = 0; _ < n_incre; ++ _) {
        std::pair<int, int> info;
        scanf("%d%d", &info.first, &info.second);
        GlobalInfo::obj_tags[info.first] = info.second;
    }
}




