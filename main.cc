#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unordered_map>
#include "cache.h"
#include "memory.h"

int main(int argc, const char* argv[]) {
    if(argc != 2) {
        printf("Wrong format. Please input command like ./main trace1.txt\n");
        exit(1);
    }
    int L1_size, L1_assoc, L1_set_num, L1_write_through, L1_write_allocate;
    int L2_size, L2_assoc, L2_set_num, L2_write_through, L2_write_allocate;
    FILE* config_file = fopen("config.txt", "r");
    fscanf(config_file, "L1 size: %d", &L1_size);
    fscanf(config_file, "L1 associativity: %d", &L1_assoc);
    fscanf(config_file, "L1 set num: %d", &L1_set_num);
    fscanf(config_file, "L1 write through: %d", &L1_write_through);
    fscanf(config_file, "L1 write allocate: %d", &L1_write_allocate);
    fscanf(config_file, "L2 size: %d", &L2_size);
    fscanf(config_file, "L2 associativity: %d", &L2_assoc);
    fscanf(config_file, "L2 set num: %d", &L2_set_num);
    fscanf(config_file, "L2 write through: %d", &L2_write_through);
    fscanf(config_file, "L2 write allocate: %d", &L2_write_allocate);
    CacheConfig l1_config(L1_size, L1_assoc, L1_set_num, L1_write_through, L1_write_allocate);
    CacheConfig l2_config(L2_size, L2_assoc, L2_set_num, L2_write_through, L2_write_allocate);
    fclose(config_file);
    
    Memory m;
    Cache l1, l2;
    l1.SetLower(&l2);
    l2.SetLower(&m);
    l1.SetConfig(l1_config);
    l2.SetConfig(l2_config);

    StorageStats s;
    s.access_time = 0;
    m.SetStats(s);
    l1.SetStats(s);
    l2.SetStats(s);

    StorageLatency ml;
    ml.bus_latency = 0;
    ml.hit_latency = 100;
    m.SetLatency(ml);

    StorageLatency ll;
    ll.bus_latency = 0;
    ll.hit_latency = 3;
    l1.SetLatency(ll);
    
    ll.bus_latency = 6;
    ll.hit_latency = 4;
    l2.SetLatency(ll);
    
    l1.init();
    l2.init();

    int hit = 0, totalCycle = 0, cnt = 0, iter;
    unsigned long long addr;
    //int sz;
    int sz = 1;
    char method[10];
    char content[64];
    int r = 0, w = 0;
    
    
    

    FILE* trace_file = fopen(argv[1], "r");
    if(!trace_file) {
        printf("Open file error! Couldn't find trace file.\n");
        exit(1);
    }
    
    //for (int i = 1; i <= 100; i++) {
        fseek(trace_file, 0, SEEK_SET);
    //while (fscanf(trace_file, "%s %llx\n", method, &addr) != EOF) {
    while (fscanf(trace_file, "%llx: %d, %s\n", &addr, &sz, method) != EOF) {
        cnt++;
        if (!strcmp(method, "r") || !strcmp(method, "w")) {
            int read = method[0] == 'r';
            unsigned long long nowMask = addr >> l1_config.block_bit, nowAddr = addr;
            int ccnt = 0;
            while (nowAddr < addr + sz) {
                l1.HandleRequest(nowAddr, read, 0, content, hit, totalCycle, 0, 0);
                nowAddr = ((nowAddr >> l1_config.block_bit) + 1) << l1_config.block_bit;
                ccnt++;
            }
            //if (l2.stats_.access_counter == l2.stats_.miss_num) cout<<l2.stats_.access_counter<<" "<<l2.stats_.miss_num<<endl;
            /*if (ccnt > 1) {
                printf("%016llx, %d\n", addr, sz);
            }
            FILE* mylog = fopen("mylog.txt", "a+");
            fprintf(mylog, "0x%016llx: hit: %d, miss: %d\n", addr, l1.stats_.access_counter - l1.stats_.miss_num, l1.stats_.miss_num);
            fclose(mylog);*/
        }
                
    }
    //fclose(trace_file);
        //if (!(i % 10 == 0 && i >= 50)) continue;
        //cout<<cnt<<endl;
    double miss1, miss2, tmp1, tmp2;
    FILE* res = fopen("res.txt", "a+");
    fprintf(res, "--------------------L1--------------------\n");
    l1.printStats(res, miss1);
    
    fprintf(res, "--------------------L2--------------------\n");
    l2.printStats(res, miss2);
    
    fprintf(res, "--------------------M--------------------\n");
    m.printStats(res, tmp1);
    fprintf(res, "\n");
    fprintf(res, "Total memory access count: %d\n", cnt);
    fprintf(res, "Total memory access time: %d cycles\n", totalCycle);
    double AMAT = 3 + miss1 * (10 + 100 * miss2);
    fprintf(res, "AMAT: %lf\n", AMAT);
    fclose(res);
    return 0;
}
