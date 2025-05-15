#pragma once
#include <cstdio>
#include <vector>
#include <cassert>
#include <cstdarg>
#include <map>
#include <cctype>
#include "debug.h"
#include "hyper_parameters.h"
#include "wrong.h"


std::map<int, std::vector<int>> delete_frequency, write_frequency, read_frequency;

void pretreatment() {
    // 读取全局参数：时间片数量 T、对象标签数 M、硬盘个数 N、每块硬盘存储单元数 V 以及每个磁头每个时间片可用令牌数 G
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    /**
     * T: 时间片数量，总共有 T+105 个时间片
     * M: 对象标签数
     * N: 硬盘个数
     * V: 每个硬盘的存储单元个数
     * G: 每个磁头每个时间片最多消耗的令牌数
     */

    for (int tag = 1; tag <= M; ++ tag) {
        read_frequency[tag] = std::vector<int> ();
    }
    
    // 以下三个循环分别读取全局预处理阶段中的3组数据：
    // 第一组：各对象标签下删除操作的频率统计数据（但示例代码中仅用 %*d 读入并忽略）

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int input;
            scanf("%d", &input);
            delete_frequency[i].push_back(input);
        }
    }
    // 第二组：各对象标签下写入操作的频率统计数据
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int input;
            scanf("%d", &input);
            write_frequency[i].push_back(input);
        }
    }
    // 第三组：各对象标签下读取操作的频率统计数据

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int input;
            scanf("%d", &input);
            read_frequency[i].push_back(input);
        }
    }

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

void delete_action_output(std::vector<int> ans) {
    printf("%d\n", ans.size());
    for(int num: ans) printf("%d\n", num);
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
/*

struct read_event_id { 
    int req_id;
    int obj_id;
};

// 大缓冲区大小（例如1MB），可根据实际需求调整
const int BUF_SIZE = 1 << 20;  
char buf[BUF_SIZE];
int buf_pos = 0, buf_len = 0;

// 从缓冲区中快速读入字符
inline char fast_getchar() {
    return getchar();
    debug << "start fast_getchar" << newLine;
    if (buf_pos == buf_len) {
        buf_len = fread(buf, 1, BUF_SIZE, stdin);
        debug << "buf_len: " << buf_len << newLine;
        if (buf_len == 0) {

            return EOF;}
        buf_pos = 0;
    }
    debug << "end fast_getchar" << newLine;
    return buf[buf_pos++];
}

// 快速读入int类型数据
inline int fast_readint() {
    int x = 0, sign = 1;
    char c = getchar();

    // 跳过非数字字符
    while (!isdigit(c) && c != '-' && c != EOF) c = getchar();

    if (c == '-') {
        sign = -1;
        c = getchar();
    }

    while (isdigit(c)) {
        x = x * 10 + c - '0';
        c = getchar();
    }
    return x * sign;
}

std::vector<read_event_id> read_action_input() {
    int n_read = fast_readint();  // 快速读入事件数量
    std::vector<read_event_id> ans(n_read);

    for(int i = 0; i < n_read; i++) {
        ans[i].req_id = fast_readint();
        ans[i].obj_id = fast_readint();
    }

    return ans; // NRVO避免额外拷贝
}
*/








class read_action_output {
public:

    read_action_output(int _N): move(_N, ""), all_read_complete() {}
    void jump(int disk_id, int jump_id) {
        if (!move[disk_id].empty()) {
            throw Exception("同时对一个磁盘调用jump和pass。");
        };
        move[disk_id] = std::string("j") + " " + std::to_string(jump_id + 1);
    }
    void pass(int disk_id) {
        if (move[disk_id].find('j') != std::string::npos) {
            throw Exception("同时对一个磁盘调用jump和pass。");
        }
        move[disk_id] += "p";

    }
    void read(int disk_id) {
        if (move[disk_id].find('j') != std::string::npos) {
            throw Exception("同时对一个磁盘调用jump和pass。");
        }
        move[disk_id] += "r";
    }
    void read_complete(int req_id) { // 读完成事件
        all_read_complete.push_back(req_id);
    }
    void output() {
        for(std::string out: move) {
            if (out.find('j') != std::string::npos) printf((out + "\n").c_str());
            else printf((out + "#" + "\n").c_str());
        }
        printf("%d\n", all_read_complete.size());
        for(int num: all_read_complete) {
            printf("%d\n", num);
        }
        fflush(stdout);
    };
private:
    std::vector<std::string> move;
    std::vector<int> all_read_complete;
};

read_action_output read_out(0);
void read_print() {
    read_out.output();
    read_out = read_action_output(N);
}