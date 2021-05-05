#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include <cmath>
#include <utility>
#include <vector>
#include <cstring>
using namespace std;

struct Line {
    int valid;
    unsigned long long tag;
    int dirty;
    int last_visit;
    unsigned char* block;
    int ifPrefetch;
    int visit_count;
};

struct SET {
    Line* line;
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
    string replacePolicy;
    
    CacheConfig() {}
    CacheConfig(int _size, int _associativity, int _set_num, int _write_through, int _write_allocate) {
        size = _size;
        associativity = _associativity;
        set_num = _set_num;
        write_through = _write_through;
        write_allocate = _write_allocate;
        block_size = size / (set_num * associativity);
        set_bit = log2(set_num);
        block_bit = log2(block_size);
        prefetchNum = 2;
    }
};

class Cache: public Storage {
public:
    Cache() {}
    ~Cache() {}

    // Sets & Gets
    void SetConfig(CacheConfig cc);
    void GetConfig(CacheConfig cc);
    void SetLower(Storage *ll) { lower_ = ll; }
    // Main access process
    void HandleRequest(uint64_t addr, int bytes, int read,
                       char *content, int &hit, int &totalCycle, int ifPrefetch, int ifWriteDirty);

    // Bypassing
    int BypassDecision(unsigned long long addr);
    // Partitioning
    void PartitionAlgorithm();
    // Replacement
    void HitUpdate(int setID, int lineID);
    std::pair<int, int> ReplaceDecision(unsigned long long addr);
    void ReplaceAlgorithm(uint64_t addr, int bytes, int read,
                          char* content, int &hit, int &totalCycle, int ifPrefetch);
    // Prefetching
    int PrefetchDecision(unsigned long long addr, int miss);
    void PrefetchAlgorithm(unsigned long long addr, int miss);
    inline int getSetID(unsigned long long addr);
    inline unsigned long long getTag(unsigned long long addr);
    inline int getBlockID(unsigned long long addr);
    int selectVictim(int setID);

    CacheConfig config_;
    Storage *lower_;
    DISALLOW_COPY_AND_ASSIGN(Cache);
    SET* sets;
    int total_count;
    
    vector<vector<unsigned long long> > LRU;
    void init();
};

#endif //CACHE_CACHE_H_ 
