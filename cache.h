#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include "co_filter.h"
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
using namespace std;

struct Line {
    int valid;
    unsigned long long tag;
    int dirty;
    unsigned char* block;
    int visit_count;
};

struct SET {
    Line* line;
};

struct record {
    unsigned long long addr;
    int sz;
};

struct CacheConfig {
    int size;
    int associativity;
    int set_num; // Number of cache sets
    int write_through; // 0|1 for back|through
    int write_allocate; // 0|1 for no-alc|alc
    int block_size;
    int set_bit, block_bit;
    int prefetchNum;
    int if_prefetch;
    string replacePolicy;
    
    CacheConfig() {}
    CacheConfig(int _size, int _associativity, int _set_num, int _write_through, int _write_allocate, string _replace_policy, int _if_prefetch) {
        size = _size;
        associativity = _associativity;
        set_num = _set_num;
        write_through = _write_through;
        write_allocate = _write_allocate;
        block_size = size / (set_num * associativity);
        set_bit = log2(set_num);
        block_bit = log2(block_size);
        prefetchNum = 2;
        replacePolicy = _replace_policy;
        if_prefetch = _if_prefetch;
    }
};

class Cache: public Storage {
public:
    CoFilter snoop;
    int core;
    int layer;
    unordered_set<unsigned long long> tagSet;
    int validLines;
    unordered_map<unsigned long long, record> recordMap;
    Cache(int _core, Cache** p, int _layer) {
        core = _core;
        snoop = CoFilter(p);
        layer = _layer;
        validLines = 0;
    }
    ~Cache() {}

    // Sets & Gets
    void SetConfig(CacheConfig cc);
    void GetConfig(CacheConfig cc);
    void SetLower(Storage *ll) { lower_ = ll; }
    // Main access process
    void HandleRequest(uint64_t addr, int bytes, int read,
                       char *content, int ifPrefetch, int ifWriteDirty);


    // Replacement
    void HitUpdate(int setID, int lineID);
    std::pair<int, int> ReplaceDecision(unsigned long long addr);
    void ReplaceAlgorithm(uint64_t addr, int bytes, int read,
                          char* content, int ifPrefetch, int ifWriteDirty);
    // Prefetching
    int PrefetchDecision(unsigned long long addr, int miss);
    void PrefetchAlgorithm(unsigned long long addr, int miss);
    inline int getSetID(unsigned long long addr);
    inline unsigned long long getTag(unsigned long long addr);
    inline int getBlockID(unsigned long long addr);
    inline unsigned long long getTagAndSet(unsigned long long addr);
    void selectVictimOfLRU(int setID, int &res);
    void selectVictimOfLFU(int setID, int &res);
    void selectVictimOfFIFO(int setID, int &res);
    int selectVictim(int setID);
    
    int checkCapacityMiss();
    int checkConflictMiss(uint64_t addr);
    int checkFalseSharing(uint64_t addr, int bytes);

    CacheConfig config_;
    Storage *lower_;
    DISALLOW_COPY_AND_ASSIGN(Cache);
    SET* sets;
    int total_count;
    
    vector<vector<unsigned long long> > LRU;
    void init();
};

#endif //CACHE_CACHE_H_ 
