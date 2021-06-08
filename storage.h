#ifndef CACHE_STORAGE_H_
#define CACHE_STORAGE_H_

#include <stdint.h>
#include <stdio.h>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

// Storage access stats
struct StorageStats {
    int access_counter;
    int miss_num;
    int compulsory_miss, capacity_miss, conflict_miss;
    int false_sharing;
    int replace_num; // Evict old lines
    int fetch_num; // Fetch lower layer
    int prefetch_num; // Prefetch
    int invalidations;
};


class Storage {
public:
    Storage() {}
    ~Storage() {}

    // Sets & Gets
    void SetStats(StorageStats ss) {
        stats_ = ss;
        stats_.access_counter = 0;
        stats_.miss_num = 0;
        stats_.replace_num = 0;
        stats_.fetch_num = 0;
        stats_.prefetch_num = 0;
        stats_.compulsory_miss = 0;
        stats_.capacity_miss = 0;
        stats_.conflict_miss = 0;
        stats_.false_sharing = 0;
        stats_.invalidations = 0;
    }
    void GetStats(StorageStats &ss) { ss = stats_; }

    // Main access process
    // [in]  addr: access address
    // [in]  bytes: target number of bytes
    // [in]  read: 0|1 for write|read
    // [i|o] content: in|out data
    // [out] hit: 0|1 for miss|hit
    // [out] time: total access time
    virtual void HandleRequest(uint64_t addr, int bytes, int read,
                             char *content, int ifPrefetch, int ifWriteDirty) = 0;

    StorageStats stats_;
    void printStats(FILE* res, double &missRate) {
        fprintf(res, "access count: %d\n", stats_.access_counter);
        fprintf(res, "hit count: %d\n", stats_.access_counter - stats_.miss_num);
        fprintf(res, "miss count: %d\n", stats_.miss_num);
        fprintf(res, "compulsory miss count: %d\n", stats_.compulsory_miss);
        fprintf(res, "capacity miss count: %d\n", stats_.capacity_miss);
        fprintf(res, "conflict miss count: %d\n", stats_.conflict_miss);
        fprintf(res, "false sharing count: %d\n", stats_.false_sharing);
        fprintf(res, "invalidations count: %d\n", stats_.invalidations);
        fprintf(res, "miss rate: %lf\n", (double)stats_.miss_num / stats_.access_counter);
    }
};

#endif //CACHE_STORAGE_H_ 
