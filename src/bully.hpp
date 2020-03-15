#ifndef __BULLY_HPP
#define __BULLY_HPP
#include <list>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <condition_variable>
std::list<int> idList = {1, 2, 3, 4, 5, 6};
std::list<int> faultList;
std::list<int> recoverList; 

int Leader = -1; 
typedef enum{
    ELECTION,
    ALIVE,
    VICTORY
}msgType_t;

typedef struct {
    int id;
    int type;
}message;

std::condition_variable leader_cond_;
std::mutex mutex_;
#endif