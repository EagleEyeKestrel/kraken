#include "cache.h"
#include "def.h"
#include "co_filter.h"
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

inline unsigned long long Cache::getTagAndSet(unsigned long long addr) {
    return addr >> config_.block_bit;
}

inline int Cache::getBlockID(unsigned long long addr) {
    int mask = (1 << config_.block_bit) - 1;
    return addr & mask;
}

void Cache::init() {
    if (config_.replacePolicy == "LRU") {
        LRU = vector<vector<unsigned long long> >(config_.set_num, vector<unsigned long long>());
    } else if (config_.replacePolicy == "LFU" || config_.replacePolicy == "FIFO") {
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

int Cache::checkCapacityMiss() {
    return validLines == config_.set_num * config_.associativity;
}

int Cache::checkConflictMiss(uint64_t addr) {
    int fullFlag = 1;
    int setID = getSetID(addr);
    unsigned long long tag = getTag(addr);
    for (int j = 0; j < config_.associativity; j++) {
        if (sets[setID].line[j].valid == INVALID) {
            fullFlag = 0;
            break;
        }
    }
    return fullFlag;
}

int Cache::checkFalseSharing(uint64_t addr, int bytes) {
    int setID = getSetID(addr);
    unsigned long long tag = getTag(addr);
    unsigned long long ts = getTagAndSet(addr);
    int lineID = -1;
    for (int j = 0; j < config_.associativity; j++) {
        if (sets[setID].line[j].valid == INVALID && sets[setID].line[j].tag == tag) {
            lineID = j;
            break;
        }
    }
    if (lineID == -1) return 0;
    //printf("%016llx %d\n", addr, bytes);
    record tmp = recordMap[ts];
    unsigned long long l1 = addr, r1 = addr + bytes - 1;
    unsigned long long l2 = tmp.addr, r2 = tmp.addr + tmp.sz - 1;
    return r1 < l2 || r2 < l1;
}

//content is write content or read buffer
void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int ifPrefetch, int ifWriteDirty) {
    unsigned long long ts = getTagAndSet(addr);
    if(!ifPrefetch && !ifWriteDirty) {
        total_count++;
        stats_.access_counter++;
    }
    pair<int, int> tmpRes = ReplaceDecision(addr);
        if(tmpRes.first) {

            //cout<<layer<<" "<<ifPrefetch<<" "<<"miss\n";

            if(!ifPrefetch && !ifWriteDirty) {
                stats_.miss_num++;
                if (tagSet.count(ts)) {
                    if (checkFalseSharing(addr, bytes)) {
                        stats_.false_sharing++;
                    } else if (checkCapacityMiss()) {
                        stats_.capacity_miss++;
                    } else if (checkConflictMiss(addr)){
                        stats_.conflict_miss++;
                    }
                } else {
                    stats_.compulsory_miss++;
                }
            }
            ReplaceAlgorithm(addr, bytes, read, content, ifPrefetch, ifWriteDirty);
        } else {
            //cout<<layer<<" "<<ifPrefetch<<" "<<"hit\n";
            if (layer == 1) snoop.hitSpread(core, read, addr, bytes, ifPrefetch);
            int setID = getSetID(addr);
            int lineID = tmpRes.second;
            int blockID = getBlockID(addr);
            if(read == 1) {
                memcpy(content, sets[setID].line[lineID].block + blockID, bytes);
            }
            if(read == 0) {
                //cout<<setID<<" "<<lineID<<" "<<blockID<<" "<<bytes<<endl;
                memcpy(sets[setID].line[lineID].block + blockID, content, bytes);
                sets[setID].line[lineID].dirty = 1;
            }
            
        }
    if (!tagSet.count(ts)) {
        tagSet.insert(ts);
    }
    if(!ifPrefetch && PrefetchDecision(addr, tmpRes.first)) {
        stats_.prefetch_num++;
        PrefetchAlgorithm(addr, tmpRes.first);
    }
    //if (addr == 0x00007f17fad83514) cout<<"prefetch done\n";
    return;
}

void Cache::HitUpdate(int setID, int lineID) {
    if (config_.replacePolicy == "LRU") {
        for (int k = 0; k < LRU[setID].size(); k++) {
            if (LRU[setID][k] == lineID) {
                LRU[setID].erase(LRU[setID].begin() + k);
                LRU[setID].insert(LRU[setID].begin(), lineID);
                return;
            }
        }
    }
    if (config_.replacePolicy == "LFU") {
        sets[setID].line[lineID].visit_count++;
        return;
    }
    if (config_.replacePolicy == "FIFO") {
        return;
    }
}

pair<int, int> Cache::ReplaceDecision(unsigned long long addr) {
    //  return { ifReplace, lineIndex }
    unsigned long long tmpTag = getTag(addr);
    int setID = getSetID(addr);
    int res2 = -1;
    for(int j = 0; j < config_.associativity; j++) {
        if(sets[setID].line[j].tag == tmpTag && sets[setID].line[j].valid != INVALID) {
            res2 = j;
            HitUpdate(setID, j);
            return make_pair(0, j);
        }
    }
    
    return make_pair(1, -1);
}

void Cache::selectVictimOfLRU(int setID, int &res) {
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
            int id = -1;
            for (int k = 0; k < config_.associativity; k++) {
                if (LRU[setID][k] == res) {
                    id = k;
                    break;
                }
            }
            LRU[setID].erase(LRU[setID].begin() + id);
            LRU[setID].insert(LRU[setID].begin(), res);
            /*res = LRU[setID].back();
            unsigned long long outTag = sets[setID].line[res].tag;
            LRU[setID].pop_back();
            LRU[setID].insert(LRU[setID].begin(), res);
            stats_.replace_num++;*/
        }
    }
    
}

void Cache::selectVictimOfLFU(int setID, int &res) {
    if (res == -1) {
        int min_counter = 0;
        for (int j = 0; j < config_.associativity; j++) {
            if (j == 0 || sets[setID].line[j].visit_count < min_counter) {
                res = j;
                min_counter = sets[setID].line[j].visit_count;
            }
        }
    }
    sets[setID].line[res].visit_count = 1;
}

void Cache::selectVictimOfFIFO(int setID, int &res) {
    if (res == -1) {
        int min_counter = 0;
        for (int j = 0; j < config_.associativity; j++) {
            if (j == 0 || sets[setID].line[j].visit_count < min_counter) {
                res = j;
                min_counter = sets[setID].line[j].visit_count;
            }
        }
    }
    sets[setID].line[res].visit_count = stats_.access_counter;
}

int Cache::selectVictim(int setID) {
    int res = -1;
    for(int j = 0; j < config_.associativity; j++) {
        if(sets[setID].line[j].valid == INVALID) {
            res = j;
            break;
        }
    }
    if (config_.replacePolicy == "LRU") selectVictimOfLRU(setID, res);
    if (config_.replacePolicy == "LFU") selectVictimOfLFU(setID, res);
    if (config_.replacePolicy == "FIFO") selectVictimOfFIFO(setID, res);
    return res;
}

void Cache::ReplaceAlgorithm(uint64_t addr, int bytes, int read,
                            char *content, int ifPrefetch, int ifWriteDirty){
    
    unsigned long long tmpTag = getTag(addr);

    int setID = getSetID(addr);
    int blockID = getBlockID(addr);
    int lineID = selectVictim(setID);
    /*if (core == 1 && addr == 0x00007f17fa7896a0 && stats_.access_counter == 653) {
        cout<<"here\n";
    }*/
    if (sets[setID].line[lineID].dirty) {
        //printf("write dirty\n");
        unsigned long long dirtyTag = sets[setID].line[lineID].tag;
        unsigned long long dirtyAddr = (dirtyTag << (config_.set_bit + config_.block_bit)) + (setID << config_.block_bit);
        lower_->HandleRequest(dirtyAddr, config_.block_size, 0, (char*)sets[setID].line[lineID].block, ifPrefetch, 1);
    }
    
    if (!ifPrefetch && !ifWriteDirty && sets[setID].line[lineID].valid == INVALID) {
        validLines++;
    }
    if (layer == 1 && sets[setID].line[lineID].valid != INVALID) {
        //printf("snoop out invalid\n");
        unsigned long long outTS = (sets[setID].line[lineID].tag << config_.set_bit) + setID;
        snoop.ts2line.erase(outTS);
    }
    //if (addr == 0x00007f17fad83554) cout<<"update snoop map\n";
    if (layer == 1) {
        snoop.ts2line[getTagAndSet(addr)] = lineID;
        /*if (core == 1 && addr == 0x00007f17fa7896a0 && stats_.access_counter == 653) {
            cout<<core<<" "<<read<<" "<<addr<<" "<<ifPrefetch<<endl;
        }*/
        snoop.missSpread(core, read, addr, bytes, ifPrefetch);
    }
    unsigned long long nowAddr = addr - blockID;
    //miss了取line 先要进行filter工作，再handlerequest
    //if (addr == 0x00007f17fad83554) printf("missSpread done\n");

    lower_->HandleRequest(nowAddr, bytes, 1, (char*)sets[setID].line[lineID].block, ifPrefetch, 0);
    //if (addr == 0x00007f17fad83554) printf("mdss\n");
    stats_.fetch_num++;
    sets[setID].line[lineID].tag = tmpTag;
    if (layer == 2) sets[setID].line[lineID].valid = EXCLUSIVE;

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
    //if (addr == 0x00007f17fad83554) printf("replace finished\n");
    
}

int Cache::PrefetchDecision(unsigned long long addr, int miss) {
    return config_.if_prefetch ? miss : 0;
}

void Cache::PrefetchAlgorithm(unsigned long long addr, int miss) {
    char prefetchContent[config_.block_size];
    for(int i = 1; i < config_.prefetchNum; i++) {
        HandleRequest(((addr + config_.block_size * i) >> config_.block_bit) << config_.block_bit, config_.block_size, 1, prefetchContent, 1, 0);
    }
}

