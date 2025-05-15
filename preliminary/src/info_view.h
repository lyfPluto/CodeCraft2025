#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

#include "disk.h"


namespace InfoView {
    std::string field_view(field* f) {
        if (f == nullptr) return "(NULL)";
        std::string ans = "";
        ans += "(";
        ans += "tag:" + std::to_string(f -> tag);
        ans += ",start:" + std::to_string(f -> get_start());
        ans += ",size:" + std::to_string(f -> get_size());
        ans += ",remain_empty:" + std::to_string(f -> get_remain_empty());
        ans += ",disk_id:" + std::to_string(f -> get_disk() -> id);
        ans += ",profit:" + std::to_string(f -> getProfit());
        ans += "  [";
        for (int index: f -> benefit_index) ans += std::to_string(index + f ->benefit_index_delta) + ",";
        ans += "]";
        ans += ")";
        return ans;
    }

    std::string tag_dis_view(unordered_map<int, vector<tuple<field*, field*, field*>>>& diskByLabel) {
        std::string ans = "";
        for (auto& entry: diskByLabel) {
            ans += "tag:" + std::to_string(entry.first) + "\n";
            for (tuple<field*, field*, field*>& fs: entry.second) {
                ans += "    [" + field_view(get<0>(fs)) + ",  " + field_view(get<1>(fs)) + ",  " + field_view(get<2>(fs)) + "]\n";
            }
        }
        return ans;
    }


}
