//
//  co_filter.h
//  
//
//  Created by ji luyang on 2021/5/18.
//

#ifndef co_filter_h
#define co_filter_h

#define INVALID 0
#define EXCLUSIVE 1
#define MODIFIED 2
#define SHARED 3

#include <unordered_map>
#include <vector>

using namespace std;
class Cache;
class CoFilter {
public:
    unordered_map<unsigned long long, int> ts2line;
    Cache** l1;
    CoFilter(){}
    CoFilter(Cache** c){
        l1 = c;
        ts2line.clear();
    }
    void hitSpread(int core, int read, uint64_t addr, int ifPrefetch);
    void missSpread(int core, int read, uint64_t addr, int ifPrefetch);
    
};

#endif /* co_filter_h */
