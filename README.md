# CodeCraft 2025 — 分布式对象存储控制系统

> 2025 华为软件精英挑战赛 · 个人 solo  

## 一、赛题背景

本届赛题抽象自华为云真实业务场景：AI 训练需要频繁读取海量小文件并快速加载 Checkpoint，这些数据平时存放在容量大但访问慢的 **OBS** 中，训练时需提前搬运到高性能的 **SFS Turbo**。赛题要求实现一个分布式对象存储控制模块，在有限硬盘资源和 105 时间片读延迟约束下，处理海量对象读写请求。

## 二、整体设计

### 2.1 双磁盘系统

决赛采用**两轮交互**，两轮使用不同的磁盘实现：

| 阶段 | 磁盘类 | 标签策略 | 核心目标 |
|---|---|---|---|
| 第一轮 | `CentralizedDisk` | 在线增量预测 | 探索数据、收集请求模式 |
| 第二轮 | `Disk` | 加权平方误差全局预测 | 利用预测标签优化布局、冲刺得分 |

第一轮的 `CentralizedDisk` 是静态、简化的磁盘模型：每个磁头拥有固定连续区间，区间内按标签均分，写入时从标签起点连续写入。它的优点是逻辑简单、无域锁开销，适合快速收集数据。

第二轮的 `Disk` 是成熟的域感知系统：基于 `field` 域和 `FieldGroup` 域组，支持收益驱动的磁头调度、特征感知写入和精细的垃圾回收。

### 2.2 存储布局

每块磁盘划分为两个区域：

| 区域 | 占比 | 用途 |
|---|---|---|
| 主存储区 | 1/3 | 存放一份副本，**只从此区域读取** |
| 备份区 | 2/3 | 存放两份副本，不参与读取 |

三副本分别写入不同盘。主存储区进一步划分为多个**物理域**（`field`），每个域绑定一个标签，域内对象顺序存储。读取时磁头只需在域内有收益的区间内顺序扫描，读完后才 Jump 到新域。

### 2.3 写入布局

写入策略分两层：

**磁盘级**：基于「盘空间余量 + 热度均衡度」选择三块目标盘，避免热点硬盘压力。

**域级**：使用 `WriteTool::feature_storage` 按对象生命周期特征选择落盘位置。核心逻辑是：

```cpp
long long loss = llabs(feature_left  - obj_feature - 10)
               + llabs(feature_right - obj_feature - 10);
```

其中 `feature` 是对象的删除时间（`delete_time`），`-10` 是经验偏移量。含义是让**删除时间相近的对象聚集**，便于后续 GC 高效回收。

如果 `feature_storage` 找不到满足条件的连续空闲区间，则回退到 `separate_storage`（分散写入）。

### 2.4 标签预测

决赛中隐藏标签对象（`tag = 0`）需要根据读请求的时间分布反推真实标签。实现分两阶段：

**第一轮 · 在线预测器 `EasyOnlineTagSchedule`**

- 按 **100 时间片为一个时间桶**，将时间轴离散化
- 对每个隐藏标签对象维护各桶的请求数向量 `zero_tag_obj[obj_id][t]`
- 对每个已知标签，用 Fenwick 树维护存活对象数，在线估计平均每对象请求分布 `req_frequency[tag][t]`
- 每次 `getSchedule()` 时，只把 `older_time` 之后新产生的桶**增量累加**进累计平方误差，避免重算整条向量
- 每 1800 时间片输出一次预测：选累计平方误差最小的标签

**第二轮 · 全局预测 `ImproveGreedyTagSchedule`**

- 第一轮结束后，对所有隐藏标签对象做一次全局预测
- 统计标签分布时除以 `tag_obj_holding[tag]->query(t)`（Fenwick 树查询的**历史存活对象数**），把累计请求数归一化为“平均每对象请求数”
- 只在对象生命周期 `[create_time, delete_time)` 内比较请求分布的平方误差
- 选误差最小的标签，写回 `GlobalInfo::obj_tags`，供第二轮使用

代码中还实现了 `GreedyTagSchedule`（余弦相似度）和 `LogLikelihoodTagSchedule`（泊松对数似然）作为对比策略，但第二轮实际启用的是加权平方误差。

### 2.5 磁头调度

`Disk::step` 是每个时间片调度磁头的入口：

1. 如果磁头未绑定域，遍历所有域，用 `highestScoreChoice` 选得分最高的域：

```cpp
f_profit = f->getProfit() / (f->benefit_size() + token/5);
```

其中 `token/5` 是**寻道惩罚**，避免磁头频繁跨域 Jump。已被其他磁头锁定的域、备份域返回 `NaN` 被跳过。

2. 选中域后，调用 `forward_step`，由 `OriginHeadSchedule` 生成动作序列。

`OriginHeadSchedule` 的核心是**查询距离表**：

```cpp
case 1:  query_dis = 1;  break;
case 64: query_dis = 3;  break;
case 52: query_dis = 5;  break;
case 42: query_dis = 6;  break;
case 34: query_dis = 8;  break;
case 28: query_dis = 9;  break;
case 23: query_dis = 10; break;
case 19: query_dis = 10; break;
case 16: query_dis = 10; break;
```

`preCost` 是上次动作消耗的令牌数。连续 Read 时按 `max(16, ceil(preCost * 0.8))` 递减，`query_dis` 随之增大——**连续读时愿意跨过空位去读远处的收益点，被打断时收缩回 1，专注当前位置**。


### 2.6 垃圾回收

每 **1800 时间片**执行一次 GC。`Disk::recycle` 按「收益密度 = 有收益单元数 / 域总单元数」排序，优先整理高收益密度域，每个域调用 `DiskTool::defragmentation` 做双指针压缩：

- `first_empty` 从左找第一个空位
- `last_object` 从右找最后一个非空位
- 交换两者，直到 `first_empty >= last_object` 或配额用完

`CentralizedDisk::recycle` 则随机选一个磁头区间，提取区间内所有位置的标签（已知用真实标签，隐藏用 `TagPredict::predict`），调用 `RecycleTool::improve_recycle` 生成交换对，按「填空位 → 互惠配对 → 兜底搬运」三阶段整理。

### 2.7 过载保护

`Disk::busy_adjust` 只在 `epoch ∈ (20000, 75000)` 之间生效。按标签请求热度排序，将总分 1/200 以下的域关闭请求接收（`set_request_permit(false)`），让低热度请求尽早被拒绝为繁忙，减少扣分。

## 三、项目结构

```
CodeCraft2025/
├── preliminary/              # 初赛作品
│   ├── src/                  # 核心源码
│   ├── interactor            # 官方评测交互器
│   ├── run.py                # 评测脚本
│   └── docs/                 # 赛题任务书
├── semifinal/                # 复赛作品
├── fianl/                    # 决赛作品
│   ├── src/
│   │   ├── main.cpp              # 主控流程：两轮交互入口
│   │   ├── disk_base.h           # 物理层：单元↔对象映射、磁头位置、读写原语
│   │   ├── disk.h                # 域感知磁盘：field 管理、调度、GC、繁忙控制
│   │   ├── centralized_disk.h    # 第一轮静态磁盘：磁头区间 + 标签均分
│   │   ├── field.h               # 域抽象：标签、收益、benefit_index、锁
│   │   ├── field_group.h         # 域组：跨盘同标签域绑定
│   │   ├── disk_tool.h           # 域划分、碎片整理、背包分配
│   │   ├── write_tool.h          # 写入策略：continuous / separate / feature
│   │   ├── point_schedule.h      # 磁头调度：OriginHeadSchedule / BFS
│   │   ├── tag_schedule.h        # 离线标签预测：三种策略
│   │   ├── online_tag_schedule.h # 在线标签预测器
│   │   ├── object.h              # 对象与磁盘绑定、收益反馈
│   │   ├── object_base.h         # 读请求状态机
│   │   ├── fenwick_tree.h        # 树状数组（标签存活对象数）
│   │   ├── segment_tree.h        # 线段树
│   │   ├── tool.h                # 通用算法工具
│   │   └── ...
│   ├── interactor            # 官方评测交互器
│   ├── run.py                # 评测脚本
│   └── docs/                 # 赛题任务书
└── images/                   # 各阶段排名截图
```

## 四、快速开始

### 环境要求

- C++ 编译器：支持 C++17（GCC 9+ / Clang 10+）
- CMake 3.15+
- Python 3.8+

### 编译

```bash
cd fianl/src
mkdir build && cd build
cmake ..
make
```

### 运行评测

```bash
cd fianl
python run.py <interactor路径> <数据文件路径> <可执行文件路径>
```

`run.py` 通过管道连接官方 interactor 与选手程序，自动完成一轮完整评测并生成 `result.txt`。

## 五、各阶段成绩

| 阶段 | 排名 |
|---|---|
| 初赛 | 第 5 名 |
| 复赛 | 第 3 名 |
| 决赛 | 第 22 名 |

排名截图见 `images/` 目录。
