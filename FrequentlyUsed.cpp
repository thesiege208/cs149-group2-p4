/* SJSU Fall 2018
 * CS149 Operating Systems
 * Sijing Xie (013829478), Group 2
 * Project 4
 *
 * Running LFU/MFU page allocation sims.
 */
 
#include "common.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <string>
#include <forward_list>
 
using namespace std;

const int num_process = 150;
const int total_time = 60000; // in ms

int timestamp = 0; // in ms

/* least frequently used replacement algorithm. */
void lfu() {
}

/* most frequently used replacement algorithm. */
void mfu() {
}
 
int main() {
    srand(time(NULL));
    
    // 1. gen 150 random processes in job list + names. sort by arrival.
    // 2. gen 100 page free list, each 1MB.
    // 3. work through job list, each job needs at least 4 pages from free list. each job has header + list (? each process gets own mini linked list?) of its pages in mem.
    // 4. for each memory reference, collect timestamp in s, process name, page ref'd, if page in mem, which process/page # evicted if nec. track hit/miss ratio of pages ref'd.
    // 5. run each alg 5x, calc avgs, print to output file.
    
    /* subj to change, if tracking vars for both algs at once is too messy.
    for (int i = 0; i < 5; i++) {
        lfu();
        mfu();
    }
    // calc stats, avgs
    */

    exit(0);
}
