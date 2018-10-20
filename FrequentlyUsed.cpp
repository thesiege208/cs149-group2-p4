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
#include <pthread.h>
#include <unistd.h>
 
using namespace std;

const int num_process = 150;
const int num_free = 100;
const int total_time = 60000; // in ms
const int num_threads = 25; // max # running jobs at any given time

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

int timestamp = 0; // in ms
forward_list<Page> free = forward_list(num_free); // linked list of free pages
forward_list<Process> jobs = forward_list(num_process); // linked list of jobs to run

/* least frequently used replacement algorithm. */
void lfu() {
}

/* most frequently used replacement algorithm. */
void mfu() {
}

/* start routine which takes the algorithm type ('L' or 'M') as argument. */
void *start(void *algType) {
    ready++;
    // gen 150 random processes in job list + names. sort by arrival.
    for (int i = 0; i < num_process; i++) {
        Process p = process();
        p.name = i;
        jobs.emplace_front(p);
    }
    jobs.sort();
    
    if (ready == num_threads) { pthread_cond_broadcast(&cond); }
    else { pthread_cond_wait(&cond, &mutex); }

    pthread_mutex_lock(&mutex);
    // if algType == 'L' then lfu()
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    // if algType == 'M' them mfu()
    pthread_mutex_unlock(&mutex);
    
    pthread_cancel(pthread_self());
    return NULL;
}
 
int main() {
    srand(time(NULL));

    pthread_t threads[num_threads];

    // gen 100 page free list, each 1MB.
    for (int j = 0; j < num_free; j++) {
        Page pg = page();
        free.emplace_front(pg);
    }
    
    // run lfu 5x
    for (int cnt = 0; cnt < 5; cnt++) {
        for (int i = 0; i < num_threads; i++) {
            char type = 'L';
            pthread_create(&threads[i], NULL, start,  reinterpret_cast<void*>(type));
        }
    }

    // run mfu 5x
    for (int cnt = 0; cnt < 5; c++) {
        for (int i = 0; i < num_threads; i++) {
            char type = 'M';
            pthread_create(&threads[i], NULL, start,  reinterpret_cast<void*>(type));
        }
    }

    // 3. work through job list, each job needs at least 4 pages from free list. each job has header (Process obj) + list (object var) of its pages in mem.
    // 4. for each memory reference, collect timestamp in s, process name, page ref'd, if page in mem, which process/page # evicted if nec. track hit/miss ratio of pages ref'd.
    // 5. run each alg 5x, calc avgs, print to output file.
    
    for (int i = 0; i < numberOfSellers; i++) {
        pthread_join(threads[i], NULL);
    }

    exit(0);
}
