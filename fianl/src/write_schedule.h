#include "functions.h"
#include "debug.h"
#include "wrong.h"


class WriteSchedule {
public:
    virtual std::vector<int> getSchedule(std::vector<std::pair<int, int>> objs, std::vector<std::pair<int, int>> disk_block);

};


class AWriteSchedule: public WriteSchedule {
public:
    std::vector<int> getSchedule(std::vector<std::pair<int, int>> objs, std::vector<std::pair<int, int>> disk_block) override {
        int n = objs.size();
        int m = disk_block.size();


        
    }    




};









