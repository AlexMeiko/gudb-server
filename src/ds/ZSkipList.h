#pragma once

#include <string>

//TODO: 实现跳表

struct ZSkipListNode {
    double score;
    std::string value;
    ZSkipListNode *next;
};

class ZSkipList {};
