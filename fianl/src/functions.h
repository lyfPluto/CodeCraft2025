/**
 * 包含类和函数的声明
*/
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <set>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <functional>
#include <numeric>

#include "fenwick_tree.h"
using namespace std;


class Disk;
class CentralizedDisk;
class field;
class Object;
class FieldGroup;
class OnlineTagSchedule;
class EasyOnlineTagSchedule;

struct write_event_inform;
struct read_event_id;


namespace InfoView {
    std::string field_view(field* f);
    std::string tag_dis_view(std::unordered_map<int, std::vector<std::tuple<field*, field*, field*>>>& diskByLabel);
}



