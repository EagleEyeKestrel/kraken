//
//  reader.h
//  
//
//  Created by ji luyang on 2021/5/18.
//

#ifndef reader_h
#define reader_h

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>


extern int L1_size, L1_assoc, L1_set_num, L1_write_through, L1_write_allocate;
extern int L2_size, L2_assoc, L2_set_num, L2_write_through, L2_write_allocate;
extern std::string replace_policy;
extern int num_cores;



#endif /* reader_h */
