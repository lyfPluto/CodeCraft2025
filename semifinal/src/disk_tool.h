#pragma once
#include "functions.h"
#include "debug.h"
#include "wrong.h"
#include "parameters.h"
struct field_info {
    field_info() = delete;
    field_info(int _tag,int _disk_id, int _start_pos, int _size) {
        tag = _tag;
        disk_id = _disk_id;
        start_pos = _start_pos;
        size = _size;
    }
    int tag;
    int disk_id;
    int start_pos;
    int size;
};
std::ostream& operator<<(std::ostream& os, const field_info& fi) {
    os << "{" << fi.tag << "," << fi.disk_id << "," << fi.start_pos << "," << fi.size << "}";
    return os;
}


class DiskTool {
public:






    // 磁盘区域划分
    static std::pair<std::vector<std::vector<field_info>>, std::vector<field_info>> field_distribution(
        int disk_num, int disk_size, int tag_num) {
        debug << "start field_distribution" << newLine;
        std::vector<std::vector<field_info>> field_group;
        std::vector<field_info> backup_field;
        
        std::vector<std::pair<int, int>> disk_candidate;
        for (int disk_id = 0; disk_id < disk_num; ++ disk_id) {
            disk_candidate.push_back({0, disk_id});
        }
        std::vector<int> tag_storage(tag_num, 0);
        for (int i = 1; i <= tag_num; i ++) {
            int storage = 0;
            //int max_storage = 0;
            auto& wf = GlobalVariable::write_frequency[i];
            auto& df = GlobalVariable::delete_frequency[i];
            int len = wf.size();
            for (int t = 0; t < len; t ++) {
                storage += wf[t];
                storage -= df[t];
                //max_storage = std::max(max_storage, storage);
                //tag_storage[i - 1] = max(max_storage, tag_storage[i - 1]);
                tag_storage[i - 1] += storage;
            } 
        }
        long long sum = 0;
        for (int num: tag_storage) sum += num;
        double discount = (((double)GlobalVariable::N * GlobalVariable::V) / (3 * sum));
        discount *= 1.05;
        for (int& num: tag_storage) num *= discount;
        int seg_count = GlobalVariable::N * 8;
        int min_len = 0, max_len = 1000000;
        while (min_len < max_len - 1) {
            int mid_len = (min_len + max_len) / 2;
            int seg_need = 0;
            for (int i = 0; i < GlobalVariable::M; i ++) {
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




/*
        std::vector<std::pair<int, int>> field_para; // {大小, 标签}
        for (int i = 0; i < tag_num; i ++) {
            int seg_tag = (tag_storage[i] - 1) / seg_len + 1;
            for (int j = 0; j < seg_tag; j ++) {
                field_para.push_back({tag_storage[i] / seg_tag, i + 1});
            }
        }
        std::vector<int> size;
        std::vector<int> weight;
        for (std::pair<int, int>& para: field_para) {
            int len = para.first;
            int tag = para.second;
            size.push_back(len);
            int w = 0;
            for (int rf: GlobalVariable::read_frequency[tag]) w += rf;
            weight.push_back(w / ((tag_storage[tag] - 1) / seg_len + 1));
        }
        //static std::vector<std::vector<int>> BackpackItemDistribution(int N, int M, 
        //    int capacity, std::vector<int> size, std::vector<int> weight)
        std::vector<std::vector<int>> dis_ans = DiskTool::BackpackItemDistribution(GlobalVariable::N, 
            field_para.size(), 1.4 * GlobalVariable::V / 3, size, weight);
        std::vector<int> disk_start(GlobalVariable::N, 0);
        for (int disk_id = 0; disk_id < GlobalVariable::N; ++ disk_id) {
            int next_disk_id = (disk_id + 1) % GlobalVariable::N;
            for (int _: dis_ans[disk_id]) {
                int field_size = field_para[_].first;
                int field_tag = field_para[_].second;
                field_info f1 = {field_tag, disk_id, disk_start[disk_id], field_size};
                field_info f2 = {field_tag, next_disk_id, disk_start[next_disk_id], field_size};
                field_group.push_back({f1, f2});
                disk_start[disk_id] += field_size;
                disk_start[next_disk_id] += field_size;
            }
        }
        for (int j = 0; j < disk_num; j ++) {
            if (disk_start[j] < disk_size) {
                int start = (disk_start[j] + disk_size) % disk_size;
                //field* f = disk_candidate[j].second -> add_field(-1, start, GlobalVariable::V - disk_candidate[j].first);
                field_info f = {-1, j, start, disk_size - start};
                backup_field.push_back(f);
            }
        }

        return {field_group, backup_field};
        throw Exception("...");
*/


        for (int i = 0; i < tag_num; i ++) {
            std::vector<int> distribution(disk_num, 0);
            int seg_tag = (tag_storage[i] - 1) / seg_len + 1;
            sort(disk_candidate.begin(), disk_candidate.end(),
            [](const std::pair<int,int>& a, const std::pair<int, int>& b) {
                return a.first < b.first;  // 比较 pair.first
            });
            if (seg_tag > disk_num) seg_tag = disk_num;
            int disk_index = 0;
            std::vector<std::pair<int, int>> dis;
            for (int j = 0; j < seg_tag; j ++) {
                sort(disk_candidate.begin(), disk_candidate.end(),
                [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                    return a.first < b.first;  // 比较 pair.first
                });
                disk_index = rand() % 3;
                int disk_index1 = rand() % 3;
                int disk_index2 = disk_index1 + 1;
                {
                    disk_index2 = -1;
                    for (int candicate_id = 0; candicate_id < GlobalVariable::N; ++ candicate_id) {
                        if (disk_candidate[candicate_id].second == (disk_candidate[disk_index1].second + 1) % GlobalVariable::N) {
                            disk_index2 = candicate_id;
                        }
                    }
                    EXCEPTION_CHECK(disk_index2 == -1, "DiskTool::field_distribution: disk_index2异常");
                }

                int start1 = (disk_candidate[disk_index1].first + disk_size) % disk_size;
                int start2 = (disk_candidate[disk_index2].first + disk_size) % disk_size;
                field_info f1 = {i + 1, disk_candidate[disk_index1].second, start1, tag_storage[i] / seg_tag};
                field_info f2 = {i + 1, disk_candidate[disk_index2].second, start2, tag_storage[i] / seg_tag};
                dis.push_back({disk_candidate[disk_index1].second, disk_candidate[disk_index2].second});
                field_group.push_back({f1, f2});
                disk_candidate[disk_index1].first += tag_storage[i] / seg_tag;
                disk_candidate[disk_index2].first += tag_storage[i] / seg_tag;
                disk_index += 2;
            }
            debug << "标签: " << i + 1 << "  域分配: " << dis << newLine;
        }
        for (int j = 0; j < disk_num; j ++) {
            if (disk_candidate[j].first < disk_size) {
                int start = (disk_candidate[j].first + disk_size) % disk_size;
                //field* f = disk_candidate[j].second -> add_field(-1, start, GlobalVariable::V - disk_candidate[j].first);
                field_info f = {-1, disk_candidate[j].second, start, disk_size - disk_candidate[j].first};
                backup_field.push_back(f);
            }
        }
        debug << "end field_distribution" << newLine;
        debug << "field_group: " << field_group << newLine;
        return {field_group, backup_field};
    }

    // 磁盘碎片整理
    static std::vector<std::pair<int, int>> defragmentation(int n, std::function<bool(int)> state, int &remain) {
        std::vector<std::pair<int, int>> swaps;  // 存储交换操作的记录
    
        int first_empty = 0;  // 初始时第一个空闲位置
        int last_object = n - 1;  // 初始时最后一个存储对象的位置
    
        // 寻找第一个空闲位置
        while (first_empty < n && state(first_empty)) {
            first_empty++;
        }
    
        // 寻找最后一个存储对象的位置
        while (last_object >= 0 && !state(last_object)) {
            last_object--;
        }
    
        // 执行碎片整理操作，直到 remain 为 0 或磁盘整理完成
        while (remain > 0 && first_empty < last_object) {
            // 如果第一个空闲位置小于最后一个存储对象位置，则进行交换
            if (state(last_object)) {
                // 记录交换操作
                swaps.push_back({first_empty, last_object});
    
                // 执行交换操作
                // 注意这里只是模拟了交换，具体实现时可以根据需要改变磁盘的状态
                // 例如：state[first_empty] = true; state[last_object] = false;
    
                // 更新剩余交换次数
                remain--;
    
                // 更新空闲位置和存储对象的位置
                first_empty++;
                while (first_empty < n && state(first_empty)) {
                    first_empty++;
                }
    
                last_object--;
                while (last_object >= 0 && !state(last_object)) {
                    last_object--;
                }
            }
        }
    
        return swaps;  // 返回所有的交换记录
    }


    static std::vector<std::vector<int>> BackpackItemDistribution(int N, int M, 
        int capacity, std::vector<int> size, std::vector<int> weight) {
        
        EXCEPTION_CHECK(M != size.size() || M != weight.size(), "DiskTool::BackpackItemDistribution: 数组长度异常");
        std::vector<int> indices(M);
        iota(indices.begin(), indices.end(), 0);
        sort(indices.begin(), indices.end(), [&](int a, int b) {
            if (size[a] != size[b]) return size[a] > size[b];
            return weight[a] > weight[b];
        });
    
        std::vector<std::vector<int>> result(N);
        std::vector<int> current_size(N, 0);
        std::vector<int> current_weight(N, 0);
        for (int idx : indices) {
            int s = size[idx];
            int w = weight[idx];
            int selected = -1;
            int min_weight = std::numeric_limits<int>::max();
            int max_remain = -1;
    
            std::vector<int> iterate_seq(N);
            iota(iterate_seq.begin(), iterate_seq.end(), 0);
            Tool::shuffleVector(iterate_seq);
            // 寻找可容纳当前物品且总重量最小的背包
            //for (int i = 0; i < N; ++i) {
            for (int i: iterate_seq) {
                if (current_size[i] + s > capacity) continue; // 跳过容量不足的背包
                // 优先选择总重量更小的背包
                if (current_weight[i] < min_weight) {
                    min_weight = current_weight[i];
                    selected = i;
                    max_remain = capacity - current_size[i];
                } 
                // 若总重量相同，则选择剩余容量更大的背包
                else if (current_weight[i] == min_weight) {
                    int remain = capacity - current_size[i];
                    if (remain > max_remain) {
                        selected = i;
                        max_remain = remain;
                    }
                }
            }
    
            // 若没有找到可用的背包，抛出异常
            if (selected == -1) {
                throw InsufficientBackpackCapacity();
            }
    
            // 将物品放入选中的背包并更新状态
            result[selected].push_back(idx);
            current_size[selected] += s;
            current_weight[selected] += w;
        }
    
        return result;
    }

    static std::vector<std::vector<int>> BackpackItemDistribution(int N, int M, int T, int capacity, 
        const std::vector<int>& size, 
        const std::vector<std::vector<int>>& weight);


};


std::vector<std::vector<int>> DiskTool::BackpackItemDistribution(int N, int M, int T, int capacity, 
                        const std::vector<int>& size, 
                        const std::vector<std::vector<int>>& weight) {
    // 1. Check capacity for each item
    for (int i = 0; i < M; ++i) {
        if (size[i] > capacity) {
            throw InsufficientBackpackCapacity();  // 单个物品超过背包容量
        }
    }
    // If total sizes exceed total capacity, no valid packing (optional check)
    long long totalSize = 0;
    for (int s : size) totalSize += s;
    if (totalSize > 1LL * capacity * N) {
        // 总物品大小超过总容量，无解（这里也可抛异常或处理）
        throw InsufficientBackpackCapacity();
    }

    // 2. Compute lower bound (LB) for maximum load
    int LB = 0;
    // Lower bound at least the max single-item weight at any time
    int maxSingleWeight = 0;
    for (int i = 0; i < M; ++i) {
        for (int t = 0; t < T; ++t) {
            if (weight[t][i] > maxSingleWeight) {
                maxSingleWeight = weight[t][i];
            }
        }
    }
    LB = maxSingleWeight;
    // Also consider average distribution per time
    for (int t = 0; t < T; ++t) {
        long long sumW = 0;
        for (int i = 0; i < M; ++i) {
            sumW += weight[t][i];
        }
        int avg = (int)((sumW + N - 1) / N);  // ceil division
        if (avg > LB) LB = avg;
    }

    // 3. Greedy initial assignment to get an upper bound (UB)
    int UB = 0;
    // Order items by a weight metric (peak weight)
    std::vector<int> order(M);
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) {
        // Compare by max weight (or sum of weights as tiebreaker)
        int maxWa = 0, maxWb = 0;
        long long sumWa = 0, sumWb = 0;
        for (int t = 0; t < T; ++t) {
            if (weight[t][a] > maxWa) maxWa = weight[t][a];
            if (weight[t][b] > maxWb) maxWb = weight[t][b];
            sumWa += weight[t][a];
            sumWb += weight[t][b];
        }
        if (maxWa != maxWb) return maxWa > maxWb;
        // If peak weights equal, compare total weight as secondary
        return sumWa > sumWb;
    });
    // Greedy assignment
    std::vector<int> binSizeUsed(N, 0);
    std::vector<std::vector<int>> binWeightUsed(N, std::vector<int>(T, 0));
    // Initially no bin used
    int binsUsed = 0;
    for (int idx = 0; idx < M; ++idx) {
        int item = order[idx];
        // choose best bin for this item
        int bestBin = -1;
        int bestNewMax = std::numeric_limits<int>::max();
        for (int j = 0; j < N; ++j) {
            if (j >= binsUsed && binsUsed == N) {
                // no more bins available
                continue;
            }
            // Check capacity
            if (j < binsUsed) {
                if (binSizeUsed[j] + size[item] > capacity) continue;
            } else {
                // j == binsUsed (a new bin)
                if (size[item] > capacity) continue; // (shouldn't happen due to earlier check)
            }
            // Calculate new max load if item put in bin j
            int newMax = 0;
            for (int t = 0; t < T; ++t) {
                int load = binWeightUsed[j][t] + weight[t][item];
                if (load > newMax) newMax = load;
            }
            // Also consider other bins' current max load
            // The overall max after placing would be max(newMax, current global max)
            int globalMaxBefore = UB;
            if (idx == 0) {
                globalMaxBefore = 0;
            } else {
                // Compute current global max load among bins
                globalMaxBefore = 0;
                for (int bj = 0; bj < binsUsed; ++bj) {
                    for (int t = 0; t < T; ++t) {
                        if (binWeightUsed[bj][t] > globalMaxBefore) {
                            globalMaxBefore = binWeightUsed[bj][t];
                        }
                    }
                }
            }
            int newGlobalMax = newMax;
            if (globalMaxBefore > newGlobalMax) newGlobalMax = globalMaxBefore;
            if (newGlobalMax < bestNewMax) {
                bestNewMax = newGlobalMax;
                bestBin = j;
            }
        }
        if (bestBin == -1) {
            // This should not happen unless binsUsed==N and item doesn't fit any (capacity issue)
            throw InsufficientBackpackCapacity();
        }
        // Place item in bestBin
        if (bestBin == binsUsed) {
            // use a new bin
            binsUsed++;
        }
        binSizeUsed[bestBin] += size[item];
        for (int t = 0; t < T; ++t) {
            binWeightUsed[bestBin][t] += weight[t][item];
        }
        // update UB as the current global max load
        int currentMax = 0;
        for (int bj = 0; bj < binsUsed; ++bj) {
            for (int t = 0; t < T; ++t) {
                if (binWeightUsed[bj][t] > currentMax) {
                    currentMax = binWeightUsed[bj][t];
                }
            }
        }
        if (currentMax > UB) {
            UB = currentMax;
        }
    }
    if (UB < LB) UB = LB;  // ensure UB at least LB

    // 4. Binary search for minimal feasible max load
    std::vector<std::vector<int>> bestAssign; // store best assignment found
    int lo = LB, hi = UB;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        // Check if feasible with max load = mid
        // Use DFS/backtracking to try to assign all items within mid
        // Sort items by difficulty (peak weight) to improve pruning
        std::vector<int> order2(M);
        iota(order2.begin(), order2.end(), 0);
        sort(order2.begin(), order2.end(), [&](int a, int b) {
            // sort by max weight or size to prioritize heavy items
            int maxWa = 0, maxWb = 0;
            for (int t = 0; t < T; ++t) {
                if (weight[t][a] > maxWa) maxWa = weight[t][a];
                if (weight[t][b] > maxWb) maxWb = weight[t][b];
            }
            if (maxWa != maxWb) return maxWa > maxWb;
            return size[a] > size[b];
        });
        // Reset bin usage arrays
        std::vector<int> binSize(N, 0);
        std::vector<std::vector<int>> binWeight(N, std::vector<int>(T, 0));
        bool feasible = false;
        // DFS lambda
        std::function<bool(int, int)> dfs = [&](int idx, int used) {
            if (idx == M) {
                // all items placed successfully
                return true;
            }
            int item = order2[idx];
            // Try placing item into existing bins
            for (int j = 0; j < used; ++j) {
                // capacity constraint
                if (binSize[j] + size[item] > capacity) continue;
                // weight constraint for all time periods
                bool ok = true;
                for (int t = 0; t < T; ++t) {
                    if (binWeight[j][t] + weight[t][item] > mid) {
                        ok = false;
                        break;
                    }
                }
                if (!ok) continue;
                // Place item in bin j
                binSize[j] += size[item];
                for (int t = 0; t < T; ++t) {
                    binWeight[j][t] += weight[t][item];
                }
                if (dfs(idx + 1, used)) {
                    return true;
                }
                // backtrack
                binSize[j] -= size[item];
                for (int t = 0; t < T; ++t) {
                    binWeight[j][t] -= weight[t][item];
                }
            }
            // Try placing item in a new bin (if available)
            if (used < N) {
                // Place in bin 'used' (next new bin)
                binSize[used] = size[item];
                for (int t = 0; t < T; ++t) {
                    binWeight[used][t] = weight[t][item];
                }
                // Check weight constraint on new bin (should hold since it was empty)
                bool ok_new = true;
                for (int t = 0; t < T; ++t) {
                    if (binWeight[used][t] > mid) { ok_new = false; break; }
                }
                if (ok_new) {
                    if (dfs(idx + 1, used + 1)) {
                        return true;
                    }
                }
                // backtrack new bin (reset)
                for (int t = 0; t < T; ++t) {
                    binWeight[used][t] = 0;
                }
                binSize[used] = 0;
            }
            return false;
        };
        feasible = dfs(0, 0);
        if (feasible) {
            // mid is achievable, record as potential best and try lower
            hi = mid;
        } else {
            // mid not achievable, increase it
            lo = mid + 1;
        }
    }
    // lo (or hi) is now the minimum achievable max load
    int optimalMax = lo;
    // 5. Reconstruct assignment for optimalMax
    // We perform DFS similar to above, but now we build the assignment result.
    std::vector<std::vector<int>> result(N);
    std::vector<int> binSizeFinal(N, 0);
    std::vector<std::vector<int>> binWeightFinal(N, std::vector<int>(T, 0));
    // Sort items in the same order as during feasibility check for consistency
    std::vector<int> orderFinal(M);
    iota(orderFinal.begin(), orderFinal.end(), 0);
    sort(orderFinal.begin(), orderFinal.end(), [&](int a, int b) {
        int maxWa = 0, maxWb = 0;
        for (int t = 0; t < T; ++t) {
            if (weight[t][a] > maxWa) maxWa = weight[t][a];
            if (weight[t][b] > maxWb) maxWb = weight[t][b];
        }
        if (maxWa != maxWb) return maxWa > maxWb;
        return size[a] > size[b];
    });
    std::function<bool(int, int)> dfs_assign = [&](int idx, int used) {
        if (idx == M) {
            return true;
        }
        int item = orderFinal[idx];
        for (int j = 0; j < used; ++j) {
            if (binSizeFinal[j] + size[item] > capacity) continue;
            bool ok = true;
            for (int t = 0; t < T; ++t) {
                if (binWeightFinal[j][t] + weight[t][item] > optimalMax) {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;
            // assign item to bin j
            binSizeFinal[j] += size[item];
            for (int t = 0; t < T; ++t) {
                binWeightFinal[j][t] += weight[t][item];
            }
            result[j].push_back(item);
            if (dfs_assign(idx + 1, used)) {
                return true;
            }
            // backtrack
            result[j].pop_back();
            binSizeFinal[j] -= size[item];
            for (int t = 0; t < T; ++t) {
                binWeightFinal[j][t] -= weight[t][item];
            }
        }
        if (used < N) {
            // use new bin
            if (size[item] <= capacity) {
                bool ok_new = true;
                for (int t = 0; t < T; ++t) {
                    if (weight[t][item] > optimalMax) { ok_new = false; break; }
                }
                if (!ok_new) {
                    // If a single item exceeds optimalMax in some dimension, it's impossible (shouldn't happen due to LB)
                } else {
                    binSizeFinal[used] = size[item];
                    for (int t = 0; t < T; ++t) {
                        binWeightFinal[used][t] = weight[t][item];
                    }
                    result[used].push_back(item);
                    if (dfs_assign(idx + 1, used + 1)) {
                        return true;
                    }
                    // backtrack
                    result[used].pop_back();
                    for (int t = 0; t < T; ++t) {
                        binWeightFinal[used][t] = 0;
                    }
                    binSizeFinal[used] = 0;
                }
            }
        }
        return false;
    };
    dfs_assign(0, 0);
    return result;
}

