#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <cstring>
#include "cache.h"
#include "memory.h"
#include "reader.h"

int L1_size, L1_assoc, L1_set_num, L1_write_through, L1_write_allocate;
int L2_size, L2_assoc, L2_set_num, L2_write_through, L2_write_allocate;
std::string replace_policy;
int if_prefetch;
int num_cores;

void readConfig() {
    std::fstream config_file;
    config_file.open("./config.txt");
    if (!config_file.is_open()) {
        printf("Open file error! Couldn't find config file.\n");
        exit(1);
    }
    std::string s1, s2, s3;
    config_file >> s1 >> s2 >> L1_size;
    config_file >> s1 >> s2 >> L1_assoc;
    config_file >> s1 >> s2 >> s3 >> L1_set_num;
    config_file >> s1 >> s2 >> s3 >> L1_write_through;
    config_file >> s1 >> s2 >> s3 >> L1_write_allocate;
    config_file >> s1 >> s2 >> L2_size;
    config_file >> s1 >> s2 >> L2_assoc;
    config_file >> s1 >> s2 >> s3 >> L2_set_num;
    config_file >> s1 >> s2 >> s3 >> L2_write_through;
    config_file >> s1 >> s2 >> s3 >> L2_write_allocate;
    config_file >> s1 >> s2 >> replace_policy;
    config_file >> s1 >> s2 >> if_prefetch;
    config_file >> s1 >> s2 >> num_cores;
    config_file.close();
}

int main(int argc, const char* argv[]) {
    if(argc != 2) {
        printf("Wrong format. Please input command like ./main trace1.txt\n");
        exit(1);
    }
    readConfig();
    CacheConfig l1_config(L1_size, L1_assoc, L1_set_num, L1_write_through, L1_write_allocate, replace_policy, if_prefetch);
    CacheConfig l2_config(L2_size, L2_assoc, L2_set_num, L2_write_through, L2_write_allocate, replace_policy, if_prefetch);

    Memory m;
    Cache* l1[num_cores];
    Cache* l2[1];
    for (int i = 0; i < num_cores; i++) {
        l1[i] = new Cache(i, l1, 1);
    }
    l2[0] = new Cache(0, l2, 2);
    for (int i = 0; i < num_cores; i++) {
        l1[i]->SetLower(l2[0]);
        l1[i]->SetConfig(l1_config);
    }
    l2[0]->SetLower(&m);
    l2[0]->SetConfig(l2_config);

    StorageStats s;
    for (int i = 0; i < num_cores; i++) {
        l1[i]->SetStats(s);
    }
    l2[0]->SetStats(s);
    m.SetStats(s);

    
    for (int i = 0; i < num_cores; i++) {
        l1[i]->init();
    }
    l2[0]->init();

    int cnt = 0, iter;
    unsigned long long addr;
    int sz = 1, _core;
    int content_size = L1_size / (L1_assoc * L1_set_num);
    //cout<<content_size<<endl;
    char method[10];
    char content[content_size];
    int r = 0, w = 0;
    

    FILE* trace_file = fopen(argv[1], "r");
    if(!trace_file) {
        printf("Open file error! Couldn't find trace file.\n");
        exit(1);
    }
    
    //for (int i = 1; i <= 100; i++) {
    fseek(trace_file, 0, SEEK_SET);
    //while (fscanf(trace_file, "%s %llx\n", method, &addr) != EOF) {
    while (fscanf(trace_file, "%llx: %d, %d, %s\n", &addr, &_core, &sz, method) != EOF) {
        cnt++;
        //printf("%d: %016llx, %d, %d\n", cnt, addr, _core, sz);
        if (!strcmp(method, "r") || !strcmp(method, "w")) {
            int read = method[0] == 'r';
            unsigned long long nowMask = addr >> l1_config.block_bit, nowAddr = addr;
            while (nowAddr < addr + sz) {
                int _bytes = (l1_config.block_size) - (nowAddr & (l1[_core]->config_.block_size - 1));
                _bytes = min(_bytes, sz);
                //printf("%d: %016llx, %d\n", cnt, nowAddr, _bytes);
                l1[_core]->HandleRequest(nowAddr, _bytes, read, content, 0, 0);
                nowAddr = ((nowAddr >> l1_config.block_bit) + 1) << l1_config.block_bit;
            }
        }
        /*FILE* log = fopen("mylog.txt", "a+");
        fprintf(log, "core%d 0x%016llx: hit: %d, miss: %d\n", _core, addr, l1[_core]->stats_.access_counter - l1[_core]->stats_.miss_num, l1[_core]->stats_.miss_num);
        fclose(log);*/
                
    }
    double miss1, miss2, tmp1, tmp2;
    FILE* res = fopen("res.txt", "a+");
    for (int i = 0; i < num_cores; i++) {
        fprintf(res, "-----------------L1 core[%d]-----------------\n", i);
        l1[i]->printStats(res, miss1);
    }
    
    fprintf(res, "--------------------L2--------------------\n");
    l2[0]->printStats(res, miss2);
    
    fprintf(res, "--------------------M--------------------\n");
    m.printStats(res, tmp1);
    fprintf(res, "\n");
    fprintf(res, "Total memory access count: %d\n", cnt);
    fclose(res);
    return 0;
}
