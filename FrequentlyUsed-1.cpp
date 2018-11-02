/* SJSU Fall 2018
 * CS149 Operating Systems
 * Sijing Xie (013829478), Group 2
 * Project 4
 *
 * Running LFU/MFU page replacement sims.
 */
 
#include "common.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <string>
#include <map>
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

pthread_mutex_t mutex;
pthread_cond_t cond;
int ready = 0;

forward_list<Page> freePages; // linked list of free pages
int size = 0; // to track size of free list
forward_list<Process> jobs; // linked list of jobs to run
forward_list<Process> done; // linked list of completed jobs
map<int, int> memory; // tracking use of free pages; key-value = process-#pgs

/* prints memory map to file. */
void printMap(ofstream &file) {
    int i = 0;
    for (auto it = memory.begin(); it != memory.end(); ++it) {
        if (i % 10 == 0) { file << "\n"; }
        int j = 0;
        if (it->second == 0) { file << ".\t"; }
        else {
            while (j < it->second) {
                file << it->first << "\t";
                i++;
            }
        }
    }
    for (int i = memory.size(); i < 100; i++) {
        if (i % 10 == 0) { file << "\n"; }
        file << ".\t";
    }
    file << "\n\n";
}

/* start routine which takes the algorithm type (0 or 1) as argument. */
void *start(void *algType) {
    int type = (long) algType;
    int timestamp = 0;
    ready++;
    pthread_mutex_lock(&mutex);
    auto iter = memory.begin(); // memory map iterator
    pthread_mutex_unlock(&mutex);

    // choosing output file to write
    ofstream file;
    if (type == 0) {
        file.open("lfu.txt", ofstream::app);
    }
    else if (type == 1) {
        file.open("mfu.txt", ofstream::app);
    }

    while (!jobs.empty() && timestamp < total_time) {
        pthread_mutex_lock(&mutex);

        // checking all threads ready    
        if (ready != num_threads) { pthread_cond_wait(&cond, &mutex); }
        else { pthread_cond_broadcast(&cond); }

        // get next job in queue
        Process curr = jobs.front();
        jobs.pop_front();
        if (timestamp < curr.getArrival()) { timestamp = curr.getArrival(); }
        file << setfill('0') << setw(2) << timestamp / 1000 << "s: Process "
             << curr.name << " arrived. Total size " << curr.getSize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP:" << endl;
        printMap(file);
        memory[curr.name] = 0; // add to map, 0 refs
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
            if (f.getProcess() != -1 && f.getPage() != -1) { // if page has old data
                procNum = f.getProcess();
                f.setProcess(curr.name);
                pgNum = f.getPage();
                flag = true;
            }
            f.setPage(i);
            curr.memPages.emplace_front(f);
            size--;
            curr.miss++;

            // add to memory map
            iter = memory.find(curr.name);
            iter->second++;
            iter = memory.begin();

            file << setfill('0') << setw(2) << timestamp / 1000
                 << "s: Process " << curr.name << " referenced page " << i
                 << ". Page not in memory. ";
            if (flag) {
                file << "Process " << procNum << ", page " << pgNum << " evicted."
                     << endl;
            } else {
                file << "No page evicted." << endl;
            }
            pthread_mutex_unlock(&mutex);
            flag = false; // reset
            // sleep(100);
        }
        // continue referencing pages, using free list as available
        // and alg where applicable
        int localTime = timestamp; // preserving start time
        while (timestamp < localTime + curr.getService() && timestamp < total_time) {
            timestamp += 100;
            bool ref = false; // if page has been referenced
            i = locality(i, curr.getSize()); // get next ref page #
            // check if page already loaded
            for (auto it = curr.memPages.begin(); it != curr.memPages.end(); ++it) {
                if (it->getPage() == i) {
                    curr.hit++;
                    it->addCount();
                    pthread_mutex_lock(&mutex);
                    file << setfill('0') << setw(2) << timestamp / 1000
                               << "s: Process " << curr.name << " referenced page "
                               << i << ". Page in memory. No page evicted."
                               << endl;
                    pthread_mutex_unlock(&mutex);
                    ref = true;
                    break;
                 }
            }
            if (!ref) {
                pthread_mutex_lock(&mutex);
                if (!freePages.empty()) { // check if free pages available
                    f = freePages.front();
                    freePages.pop_front();
                    size--;
                    if (f.getProcess() != -1) { // if page has old data
                        procNum = f.getProcess();
                        f.setProcess(curr.name);
                        pgNum = f.getPage();
                        flag = true;
                    }
                    f.setPage(i);
                    curr.memPages.emplace_front(f);
                    curr.miss++;

                    // add to memory map
                    iter = memory.find(curr.name);
                    iter->second++;
                    iter = memory.begin();

                    file << setfill('0') << setw(2) << timestamp / 1000
                         << "s: Process " << curr.name << " referenced page " << i
                         << ". Page not in memory. ";
                    if (flag) {
                        file << "Process " << procNum << ", page " << pgNum
                             << " evicted." << endl;
                    } else {
                        file << "No page evicted." << endl;
                    }
                    pthread_mutex_unlock(&mutex);
                    flag = false; // reset
                } else if (!curr.memPages.empty()) { // use alg to replace page
                    curr.memPages.sort(); // sort in ascending order by counters
                    if (type == 1) { curr.memPages.reverse(); } // reverse if mfu
                    Page m = curr.memPages.front();
                    curr.memPages.pop_front();
                    pgNum = m.getPage();
                    procNum = curr.name;
                    m.setPage(i);
                    m.resetCount();
                    curr.memPages.emplace_front(m);
                    curr.miss++;
                    pthread_mutex_lock(&mutex);
                    file << setfill('0') << setw(2) << timestamp / 1000
                         << "s: Process " << curr.name << " referenced page " << i
                         << ". Page not in memory. Process " << procNum << ", page "
                         << pgNum << " evicted." << endl;
                    pthread_mutex_unlock(&mutex);
                } else { // no available free pages or already loaded pages
                    continue;
                }
            }
            // sleep(100); // stalled threads + returned to main/join, for some reason
        }
        // print job stats
        pthread_mutex_lock(&mutex);
        file << setfill('0') << setw(2) << timestamp / 1000 << "s: Process "
             << curr.name << " completed. Total size " << curr.getSize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP:" << endl;
        printMap(file);

        // add job to done list
        done.emplace_front(curr);

        // return pages to free list but don't clear
        for (auto it = curr.memPages.begin(); it != curr.memPages.end();) {
            f = *it;
            ++it;
            curr.memPages.pop_front();
            freePages.emplace_front(f);
            size++;
        }
        memory.erase(curr.name);
        pthread_mutex_unlock(&mutex);
    }
    file.close();
    cout << "RIGHT BEFORE CANCEL" << endl;
    pthread_cancel(pthread_self());
    return NULL;
}
 
int main() {
    srand(time(NULL));

    pthread_t threads[num_threads];
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // gen 100 page free list, each 1MB
    for (int j = 0; j < num_free; j++) {
        Page pg = Page();
        freePages.emplace_front(pg);
        size++;
    }
    
    // gen 150 random processes in job list + names. sort by arrival.
    for (int i = 1; i <= num_process; i++) {
        Process p = Process();
        p.name = i;
        jobs.emplace_front(p);
    }
    jobs.sort();

    // comment out lfu section when running mfu
    lfu_output.open("lfu.txt", ofstream::app);
    lfu_output << "USING LFU.\n\n";
    lfu_output.close();
    for (int i = 0; i < num_threads; i++) {
        int type = 0;
        pthread_create(&threads[i], NULL, start,  reinterpret_cast<void*>(type));
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // calc stats for 1 run
    int hits = 0, misses = 0, complete = 0;
    for (auto it = done.begin(); it != done.end(); ++it) {
        Process p = *it;
        hits += p.hit;
        misses += p.miss;
        complete++;
    }
    lfu_output.open("lfu.txt", ofstream::app);
    lfu_output << "STATS:\n\tHITS = " << hits << ", MISSES = " << misses << "\n\n";
    lfu_output.close();
    // end lfu comment here

    // comment out mfu section when running lfu
    /*mfu_output.open("mfu.txt", ofstream::app);
    mfu_output << "USING MFU.\n\n";
    mfu_output.close();
    for (int i = 0; i < num_threads; i++) {
        int type = 1;
        pthread_create(&threads[i], NULL, start,  reinterpret_cast<void*>(type));
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // calc stats for 1 run
    int hits = 0, misses = 0, complete = 0;
    for (auto it = done.begin(); it != done.end(); ++it) {
        Process p = *it;
        hits += p.hit;
        misses += p.miss;
        complete++;
    }
    mfu_output.open("mfu.txt", ofstream::app);
    mfu_output << "STATS:\n\tHITS = " << hits << ", MISSES = " << misses << "\n\n";
    mfu_output.close();*/
    // end mfu comment here

    return 0;
}
