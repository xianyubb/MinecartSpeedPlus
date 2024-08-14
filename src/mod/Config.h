
#include <string>

#include <vector>


struct MoveMentData {
    // 档位名称
    std::string name;
    // 开启花费金额
    int money = 1000;

    int speed = 1;
};


struct Config {
    int                       version   = 2;
    std::string               ScoreName = "money";
    std::vector<MoveMentData> moveMentList;
};

extern Config config;
