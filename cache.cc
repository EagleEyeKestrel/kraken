#include "cache.h"
#include "def.h"
#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <time.h>
#include <cstdlib>
using namespace std;

inline int Cache::getSetID(unsigned long long addr) {
    addr >>= config_.block_bit;
    return addr & ((1 << config_.set_bit) - 1);
}

inline unsigned long long Cache::getTag(unsigned long long addr) {
    return addr >> (config_.set_bit + config_.block_bit);
}

inline int Cache::getBlockID(unsigned long long addr) {
    int mask = (1 << config_.block_bit) - 1;
    return addr & mask;
}

void Cache::init() {
    if (replacePolicy == "LRU") {
        LRU = vector<vector<unsigned long long> >(config_.set_num, vector<unsigned long long>());
    } else if (replacePolicy == "LFU") {
        for (int i = 0; i < config_.set_num; i++) {
            for (int j = 0; j < config_.associativity; j++) {
                sets[i].line[j].visit_count = 0;
            }
        }
    }
    
}

void Cache::SetConfig(CacheConfig cc) {
    config_ = cc;
    total_count = 0;
    sets = new SET[config_.set_num];
    for(int i = 0; i < config_.set_num; i++) {
        sets[i].line = new Line[config_.associativity];
        memset(sets[i].line, 0, config_.associativity * sizeof(Line));
        for(int j = 0; j < config_.associativity; j++) {
            sets[i].line[j].block = new unsigned char[config_.block_size];
        }
    }
}

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &totalCycle, int ifPrefetch, int ifWriteDirty) {
    if(!ifPrefetch && !ifWriteDirty) {
        total_count++;
        stats_.access_counter++;
    }
    // Bypass?
    if (!BypassDecision(addr)) {
        PartitionAlgorithm();
        if(!ifPrefetch) {
            totalCycle += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += latency_.bus_latency + latency_.hit_latency;
        }
        pair<int, int> tmpRes = ReplaceDecision(addr);

        if(tmpRes.first) {
            if(!ifPrefetch && !ifWriteDirty) stats_.miss_num++;
            ReplaceAlgorithm(addr, bytes, read, content, hit, totalCycle, ifPrefetch);
        } else {
            // go here cout<<"2\n";
            int setID = getSetID(addr);
            int lineID = tmpRes.second;
            int blockID = getBlockID(addr);
            if(!ifPrefetch) {
                hit++;
            }
            
            if(read == 1) {
                memcpy(content, sets[setID].line[lineID].block + blockID, bytes);
            }
            if(read == 0) {
                memcpy(sets[setID].line[lineID].block + blockID, content, bytes);
                if(!config_.write_through) {
                    sets[setID].line[lineID].dirty = 1;
                } else {
                    lower_->HandleRequest(addr, bytes, read, content, hit, totalCycle, ifPrefetch, 0);// dirty add L2 hit?
                }
            }
        }
        
        if(!ifPrefetch && PrefetchDecision(addr, tmpRes.first)) {
            stats_.prefetch_num++;
            PrefetchAlgorithm(addr, tmpRes.first);
        }
        return;
    }
    if(!ifPrefetch) {
        // don't add hit latency
        stats_.access_time += latency_.bus_latency;
        stats_.bypass_num++;
        totalCycle += latency_.bus_latency;
    }
}

int Cache::BypassDecision(unsigned long long addr) {
    /*if(config_.size == 32768) return 0;
    int setID = getSetID(addr);
    int blockID = getBlockID(addr);
    unsigned long long tag = getTag(addr);
    for(int j = 0; j < config_.associativity; j++) {
        if(!sets[setID].line[j].valid) return 0;
    }
    return !bypassTable.count(tag);*/
    return 0;
}

void Cache::PartitionAlgorithm() {
}

void Cache::HitUpdate(int setID, int lineID) {
    if (replacePolicy == "LRU") {
        for (int k = 0; k < LRU[setID].size(); k++) {
            if (LRU[setID][k] == lineID) {
                LRU[setID].erase(LRU[setID].begin() + k);
                LRU[setID].insert(LRU[setID].begin(), lineID);
                return;
            }
        }
    }
    if (replacePolicy == "LFU") {
        sets[setID].line[lineID].visit_count++;
        return;
    }
}

pair<int, int> Cache::ReplaceDecision(unsigned long long addr) {
    //  return { ifReplace, lineIndex }
    unsigned long long tmpTag = getTag(addr);
    int setID = getSetID(addr);
    int res2 = -1;
    for(int j = 0; j < config_.associativity; j++) {
        if(sets[setID].line[j].tag == tmpTag && sets[setID].line[j].valid) {
            res2 = j;
            HitUpdate(setID, j);
            return make_pair(0, j);
        }
    }
    
    return make_pair(1, -1);
}

void Cache::selectVictimOfLRU(int setID, int& res) {
    if(res == -1) {
        int victimLine = LRU[setID].back();
        unsigned long long outTag = sets[setID].line[victimLine].tag;
        LRU[setID].pop_back();
        LRU[setID].insert(LRU[setID].begin(), victimLine);
        res = victimLine;
        stats_.replace_num++;
    } else {
        if(LRU[setID].size() < config_.associativity) {
            LRU[setID].insert(LRU[setID].begin(), res);
        } else {
            res = LRU[setID].back();
            unsigned long long outTag = sets[setID].line[res].tag;
            LRU[setID].pop_back();
            LRU[setID].insert(LRU[setID].begin(), res);
            stats_.replace_num++;
        }
    }
    
}

void Cache::selectVictimOfLFU(int setID, int& res) {
    if (res == -1) {
        int min_counter = -1, min_way = 0;
        
    }
}

int Cache::selectVictim(int setID) {
    int res = -1;
    for(int j = 0; j < config_.associativity; j++) {
        if(!sets[setID].line[j].valid) {
            res = j;
            break;
        }
    }
    /*if(res == -1) {
        int victimLine = LRU[setID].back();
        unsigned long long outTag = sets[setID].line[victimLine].tag;
        LRU[setID].pop_back();
        LRU[setID].insert(LRU[setID].begin(), victimLine);
        res = victimLine;
        stats_.replace_num++;
    } else {
        if(LRU[setID].size() < config_.associativity) {
            LRU[setID].insert(LRU[setID].begin(), res);
        } else {
            res = LRU[setID].back();
            unsigned long long outTag = sets[setID].line[res].tag;
            LRU[setID].pop_back();
            LRU[setID].insert(LRU[setID].begin(), res);
            stats_.replace_num++;
        }
    }*/
    return res;
}

void Cache::ReplaceAlgorithm(uint64_t addr, int bytes, int read,
                            char *content, int &hit, int &totalCycle, int ifPrefetch){
    if(!read && !config_.write_allocate) {
        lower_->HandleRequest(addr, bytes, read, content, hit, totalCycle, ifPrefetch, 0);
        return ;
    }
    
    unsigned long long tmpTag = getTag(addr);
    int setID = getSetID(addr);
    int blockID = getBlockID(addr);
    int lineID = selectVictim(setID);
    
    if(sets[setID].line[lineID].dirty) {
        unsigned long long dirtyTag = sets[setID].line[lineID].tag;
        unsigned long long dirtyAddr = (dirtyTag << (config_.set_bit + config_.block_bit)) + (setID << config_.block_bit);
        lower_->HandleRequest(dirtyAddr, config_.block_size, 0, (char*)sets[setID].line[lineID].block, hit, totalCycle, ifPrefetch, 1);
    }
    
    unsigned long long nowAddr = addr - blockID;
    lower_->HandleRequest(nowAddr, config_.block_size, 1, (char*)sets[setID].line[lineID].block, hit, totalCycle, ifPrefetch, 0);
    stats_.fetch_num++;
    // TODO: prefetch flag
    sets[setID].line[lineID].valid = 1;
    sets[setID].line[lineID].tag = tmpTag;
    
    if(!read) {
        for(int i = 0; i < bytes; i++) {
            sets[setID].line[lineID].block[blockID + i] = content[i];
        }
        sets[setID].line[lineID].dirty = 1;
    } else {
        for(int i = 0; i < bytes; i++) {
            content[i] = sets[setID].line[lineID].block[blockID + i];
        }
        sets[setID].line[lineID].dirty = 0;
    }
}

int Cache::PrefetchDecision(unsigned long long addr, int miss) {
    return miss;
    //return 0;
}

void Cache::PrefetchAlgorithm(unsigned long long addr, int miss) {
    char prefetchContent[64];
    int prefetchHit = 0, prefetchCycle = 0;
    for(int i = 1; i < config_.prefetchNum; i++) {
        HandleRequest(addr + 64 * i, 0, 2, prefetchContent, prefetchHit, prefetchCycle, 1, 0);
    }
}

