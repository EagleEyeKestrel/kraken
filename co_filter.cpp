//
//  co_filter.cpp
//  
//
//  Created by ji luyang on 2021/5/20.
//

#include <stdio.h>
#include <unordered_map>
#include <vector>
#include "co_filter.h"
#include "cache.h"
#include "reader.h"
using namespace std;

void CoFilter::hitSpread(int core, int read, uint64_t addr, int ifPrefetch) {
    int setID = l1[core]->getSetID(addr);
    unsigned long long ts = l1[core]->getTagAndSet(addr);
    
    if (l1[core]->sets[setID].line[ts2line[ts]].valid == EXCLUSIVE) {
        if (!read) {
            l1[core]->sets[setID].line[ts2line[ts]].valid = MODIFIED;
        }
    }
    
    else if (l1[core]->sets[setID].line[ts2line[ts]].valid == SHARED) {
        if (!read) {
            l1[core]->sets[setID].line[ts2line[ts]].valid = MODIFIED;
            for (int i = 0; i < num_cores; i++) {
                if (i == core || !l1[i]->snoop.ts2line.count(ts)) continue;
                if (l1[i]->sets[setID].line[l1[i]->snoop.ts2line[ts]].valid != INVALID) {
                    l1[i]->sets[setID].line[l1[i]->snoop.ts2line[ts]].valid = INVALID;
                    l1[i]->validLines--;
                }
            }
        }
    }
    
    else {
        //pass
    }
}

void CoFilter::missSpread(int core, int read, uint64_t addr, int ifPrefetch) {
    int setID = l1[core]->getSetID(addr);
    unsigned long long ts = l1[core]->getTagAndSet(addr);
    if (read) {
        //  read miss, update other cores
        /*if (core == 1 && addr == 0x00007f17fa7896a0) {
            cout<<"miss\n";
        }*/
        vector<int> toBeUpdated;
        int modifiedFlag = 0;
        for (int i = 0; i < num_cores; i++) {
            if (i == core || !l1[i]->snoop.ts2line.count(ts)) continue;
            int nowflag = l1[i]->sets[setID].line[l1[i]->snoop.ts2line[ts]].valid;
            if (nowflag != INVALID) {
                toBeUpdated.push_back(i);
                if (nowflag == MODIFIED) modifiedFlag = 1;
            }
        }
        /*if (core == 1 && addr == 0x00007f17fa7896a0) {
            cout<<toBeUpdated.size()<<endl;
        }*/
        if (toBeUpdated.empty()) {
            l1[core]->sets[setID].line[ts2line[ts]].valid = EXCLUSIVE;
        } else {
            if (modifiedFlag) {
                int _core = toBeUpdated[0];
                l1[_core]->lower_->HandleRequest(addr, l1[_core]->config_.block_size, 0, (char*)l1[_core]->sets[setID].line[l1[_core]->snoop.ts2line[ts]].block, ifPrefetch, 1);
            }
            toBeUpdated.push_back(core);
            for (int i: toBeUpdated) {
                l1[i]->sets[setID].line[l1[i]->snoop.ts2line[ts]].valid = SHARED;
            }
            
        }
    } else {
        vector<int> toBeUpdated;
        for (int i = 0; i < num_cores; i++) {
            if (i == core || !l1[i]->snoop.ts2line.count(ts)) continue;
            int nowflag = l1[i]->sets[setID].line[l1[i]->snoop.ts2line[ts]].valid;
            if (nowflag != INVALID) {
                toBeUpdated.push_back(i);
            }
        }
        l1[core]->sets[setID].line[l1[core]->snoop.ts2line[ts]].valid = MODIFIED;
        for (int i: toBeUpdated) {
            l1[i]->sets[setID].line[l1[i]->snoop.ts2line[ts]].valid = INVALID;
            l1[i]->validLines--;
        }
        
    }
}
