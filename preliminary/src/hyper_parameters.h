#pragma once

#include <vector>
#include <map>

#define FRE_PER_SLICING (1800)
class HyperParameters {
public:
    //static constexpr double discountFactor = 0.99;
    

};

// 全局变量：
// T: 时间片数量（实际交互时总共 T + EXTRA_TIME 个时间片）
// M: 对象标签数
// N: 硬盘个数
// V: 每个硬盘的存储单元个数
// G: 每个磁头每个时间片最多消耗的令牌数
int T, M, N, V, G;
int epoch = 1;
double score = 0;





