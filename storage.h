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
    int access_time; // In nanoseconds
    int replace_num; // Evict old lines
    int fetch_num; // Fetch lower layer
    int prefetch_num; // Prefetch
    int bypass_num;
};

// Storage basic config
struct StorageLatency {
  int hit_latency; // In nanoseconds
  int bus_latency; // Added to each request
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
        stats_.bypass_num = 0;
    }
    void GetStats(StorageStats &ss) { ss = stats_; }
    void SetLatency(StorageLatency sl) { latency_ = sl; }
    void GetLatency(StorageLatency &sl) { sl = latency_; }

    // Main access process
    // [in]  addr: access address
    // [in]  bytes: target number of bytes
    // [in]  read: 0|1 for write|read
    // [i|o] content: in|out data
    // [out] hit: 0|1 for miss|hit
    // [out] time: total access time
    virtual void HandleRequest(uint64_t addr, int bytes, int read,
                             char *content, int &hit, int &totalCycle, int ifPrefetch, int ifWriteDirty) = 0;

    StorageStats stats_;
    StorageLatency latency_;
    void printStats(FILE* res, double &missRate) {
        fprintf(res, "access count: %d\n", stats_.access_counter);
        fprintf(res, "access time: %d\n", stats_.access_time);
        fprintf(res, "hit count: %d\n", stats_.access_counter - stats_.miss_num);
        fprintf(res, "miss count: %d\n", stats_.miss_num);
        missRate = (double)stats_.miss_num / stats_.access_counter;
        fprintf(res, "miss rate: %lf\n", missRate);
    }
};

#endif //CACHE_STORAGE_H_ 
