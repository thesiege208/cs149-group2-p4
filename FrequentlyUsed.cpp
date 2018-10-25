/* SJSU Fall 2018
 * CS149 Operating Systems
 * Sijing Xie (013829478), Group 2
 * Project 4
 *
 * Running LFU/MFU page allocation sims.
 */
 
#include "common.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <string>
#include <forward_list>
#include <pthread.h>
#include <unistd.h>
 
using namespace std;

ofstream lfu_output;
ofstream mfu_output;

const int num_process = 150;
const int num_free = 100;
const int total_time = 60000; // in ms
const int num_threads = 25; // max # running jobs at any given time

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

int timestamp = 0; // in ms
forward_list<Page> freePages = forward_list<Page>(num_free); // linked list of free pages
int size = 0; // to track size of free list
forward_list<Process> jobs = forward_list<Process>(num_process); // linked list of jobs to run
forward_list<Process> done = forward_list<Process>(num_process); // linked list of completed jobs
int mem_map[total_time / 1000] = { 0 };

void printMap(ofstream &file) {
    int len = total_time / 1000;
    for (int i = 0; i < len; i++) {
        if (i % 10 == 0) { file << "\n"; }
        if (mem_map[i] == 0) { file << ". "; }
        else { file << mem_map[i] << " "; }
    }
    file << "\n";
}

/* start routine which takes the algorithm type ('L' or 'M') as argument. */
void *start(void *algType) {
    char *type = static_cast<char*>(algType);
    ready++;
    // gen 150 random processes in job list + names. sort by arrival.
    for (int i = 1; i <= num_process; i++) {
        Process p = Process();
        p.name = i;
        jobs.emplace_front(p);
    }
    jobs.sort();
    // checking all threads ready    
    if (ready == num_threads) { pthread_cond_broadcast(&cond); }
    else { pthread_cond_wait(&cond, &mutex); }
    // choosing output file to write
    ofstream file;
    if (*type == 'L') {
        file.open("lfu.txt", ofstream::app);
        file << "USING LFU.\n\n";
    }
    else {
        file.open("mfu.txt", ofstream::app);
        file << "USING MFU.\n\n";
    }

    while (!jobs.empty() && timestamp < total_time) {
        pthread_mutex_lock(&mutex);
        // get next job in queue
        Process curr = jobs.front();
        jobs.pop_front();
        if (timestamp < curr.getArrival()) { timestamp = curr.getArrival(); }
        file << setfill('0') << setw(2) << timestamp / 1000 << "s: Process "
             << curr.name << " arrived. Total size " << curr.getSize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP:\n";
        printMap(file);

        pthread_mutex_unlock(&mutex);
        // reference page 0
        int i = 0; // tracking page # for locality
        Page f;
        int pgNum, procNum; // preserve old data for printing
        bool flag = false; // check for eviction
        if (size >= 4) {
            pthread_mutex_lock(&mutex);
            f = freePages.front();
            freePages.pop_front();
            if (f.getProcess() != -1) { // if page has old data
                procNum = f.getProcess();
                f.setProcess(curr.name);
                pgNum = f.getPage();
                f.setPage(i);
                flag = true;
            }
            curr.memPages.emplace_front(f);
            size--;
            curr.miss++;
            pthread_mutex_unlock(&mutex);
            file << setfill('0') << setw(2) << timestamp / 1000
                 << "s: Process " << curr.name << " referenced page " << i
                 << ". Page not in memory. ";
            if (flag) {
                file << "Process " << procNum << ", page " << pgNum << " evicted."
                     << endl;
            } else {
                file << "No page evicted." << endl;
            }
            flag = false; // reset
            sleep(0.1);
        }
        // continue referencing pages, using free list as available
        // and alg where applicable
        int localTime = timestamp; // preserving start time
        while (timestamp < localTime + curr.getService() && timestamp < total_time) {
            bool ref = false; // if page has been referenced
            i = locality(i, curr.getSize()); // get next ref page #
            // check if page already loaded
            for (auto it = curr.memPages.begin(); it != curr.memPages.end(); ++it) {
                if (it->getPage() == i) {
                    curr.hit++;
                    it->addCount();
                    file << setfill('0') << setw(2) << timestamp / 1000
                               << "s: Process " << curr.name << " referenced page "
                               << i << ". Page in memory. No page evicted."
                               << endl;
                    ref = true;
                    break;
                 }
            }
            if (!ref) {
                if (!freePages.empty()) { // check if free pages available
                    pthread_mutex_lock(&mutex);
                    f = freePages.front();
                    freePages.pop_front();
                    if (f.getProcess() != -1) { // if page has old data
                        procNum = f.getProcess();
                        f.setProcess(curr.name);
                        pgNum = f.getPage();
                        f.setPage(i);
                        flag = true;
                    }
                    curr.memPages.emplace_front(f);
                    size--;
                    curr.miss++;
                    pthread_mutex_unlock(&mutex);
                    file << setfill('0') << setw(2) << timestamp / 1000
                         << "s: Process " << curr.name << " referenced page " << i
                         << ". Page not in memory. ";
                    if (flag) {
                        file << "Process " << procNum << ", page " << pgNum
                             << " evicted." << endl;
                    } else {
                        file << "No page evicted." << endl;
                    }
                    flag = false; // reset
                } else { // use alg to replace page
                    curr.memPages.sort(); // sort in ascending order by counters
                    if (*type == 'M') { curr.memPages.reverse(); }
                    Page m = curr.memPages.front();
                    pgNum = m.getPage();
                    procNum = curr.name;
                    m.setPage(i);
                    curr.miss++;
                    file << setfill('0') << setw(2) << timestamp / 1000
                         << "s: Process " << curr.name << " referenced page " << i
                         << ". Page not in memory. Process " << procNum << ", page "
                         << pgNum << " evicted." << endl;
                }
            }
            timestamp += 100;
            if (timestamp % 1000 == 0) { mem_map[timestamp / 1000] = curr.name; }
            sleep(0.1);
        }
        // add job to done list and print job stats
        done.emplace_front(curr);
        // return pages to free list but don't clear
        for (auto it = curr.memPages.begin(); it != curr.memPages.end();) {
            f = *it;
            ++it;
            curr.memPages.pop_front();
            freePages.emplace_front(f);
        }
        file << setfill('0') << setw(2) << timestamp / 1000 << "s: Process "
             << curr.name << " completed. Total size " << curr.getSize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP:\n";
        printMap(file);
    }
    
    pthread_cancel(pthread_self());
    file.close();
    return NULL;
}
 
int main() {
    srand(time(NULL));

    pthread_t threads[num_threads];

    lfu_output.open("lfu.txt");
    mfu_output.open("mfu.txt");

    // gen 100 page free list, each 1MB.
    for (int j = 0; j < num_free; j++) {
        Page pg = Page();
        freePages.emplace_front(pg);
        size++;
    }
    
    // run lfu 5x
    int lfu_hits = 0, lfu_misses = 0;
    for (int cnt = 0; cnt < 5; cnt++) {
        for (int i = 0; i < num_threads; i++) {
            char type = 'L';
            pthread_create(&threads[i], NULL, start,  reinterpret_cast<void*>(type));
        }

        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        // calc stats for 1 run
        int hits = 0, misses = 0;
        for (auto it = done.begin(); it != done.end(); ++it) {
            Process p = *it;
            hits += p.hit;
            misses += p.miss;
        }
        lfu_hits += hits;
        lfu_misses += misses;
        ofstream lfu_output;
        lfu_output.open("lfu.txt", ofstream::app);
        lfu_output << "STATS:\n\tHITS = " << hits << ", MISSES = " << misses << "\n\n";
        lfu_output.close();
    }
    lfu_hits /= 5;
    lfu_misses /= 5;
    ofstream lfu_output;
    lfu_output.open("lfu.txt", ofstream::app);
    lfu_output << "STATS AVERAGED OVER 5 RUNS:\n\tAVG HITS = " << lfu_hits << ", AVG MISSES = " << lfu_misses << endl;
    lfu_output.close();

    // run mfu 5x
    int mfu_hits = 0, mfu_misses = 0;
    for (int cnt = 0; cnt < 5; cnt++) {
        for (int i = 0; i < num_threads; i++) {
            char type = 'M';
            pthread_create(&threads[i], NULL, start,  reinterpret_cast<void*>(type));
        }

        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        // calc stats for 1 run
        int hits = 0, misses = 0;
        for (auto it = done.begin(); it != done.end(); ++it) {
            Process p = *it;
            hits += p.hit;
            misses += p.miss;
        }
        mfu_hits += hits;
        mfu_misses += misses;
        ofstream mfu_output;
        mfu_output.open("mfu.txt", ofstream::app);
        mfu_output << "STATS:\n\tHITS = " << hits << ", MISSES = " << misses << "\n\n";
        mfu_output.close();
    }
    mfu_hits /= 5;
    mfu_misses /= 5;
    ofstream mfu_output;
    mfu_output.open("mfu.txt", ofstream::app);
    mfu_output << "STATS AVERAGED OVER 5 RUNS:\n\tAVG HITS = " << mfu_hits << ", AVG MISSES = " << mfu_misses << endl;
    mfu_output.close();

    return 0;
}
