#ifndef INCLUDES_H
#define INCLUDES

#include <iostream>
#include "parser.h"
#include "fat32.h"
#include <stdlib.h>
#include <vector>
#include <time.h> 
#include <chrono>
#include <ctime> 
#include <cmath>
#define MAX_LEN 255
#include "fat32dependent.h"
#include "attribute.h"

// Month vector that are used in file attributes.
std::vector<std::string> months = {"January",
                                   "February",
                                   "March",
                                   "April", 
                                   "May" , 
                                   "June" , 
                                   "July", 
                                   "August", 
                                   "September", 
                                   "October", 
                                   "November" , 
                                   "December"};

BPB_struct bootSector;
FatFile83 fileEntryToMoved;


#endif