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

pthread_mutex_t mutex;
pthread_cond_t cond;
int ready = 0;

forward_list<Page> freePages; // linked list of free pages
int size = 0; // to track size of free list
forward_list<Process> jobs; // linked list of jobs to run
forward_list<Process> done; // linked list of completed jobs

void printMap(ofstream &file, int map[]) {
    int len = total_time / 1000;
    for (int i = 0; i < len; i++) {
        if (i > 0 && i % 10 == 0) { file << "\n"; }
        if (map[i] == 0) { file << ".\t"; }
        else { file << map[i] << "\t"; }
    }
    file << "\n\n";
}

/* start routine which takes the algorithm type ('L' or 'M') as argument. */
void *start(void *algType) {
    int type = (long) algType;
    int timestamp = 0;
    int mem_map[total_time / 1000] = { 0 };
    ready++;

    // choosing output file to write
    ofstream file;
    if (type == 0) {
        file.open("lfu.txt", ofstream::app);
    }
    else {
        file.open("mfu.txt", ofstream::app);
    }

    while (!jobs.empty() && timestamp < total_time) {
        pthread_mutex_lock(&mutex);

        // checking all threads ready    
        if (ready == num_threads) { pthread_cond_broadcast(&cond); }
        else { pthread_cond_wait(&cond, &mutex); }

        // get next job in queue
        Process curr = jobs.front();
        jobs.pop_front();
        pthread_mutex_unlock(&mutex);
        if (timestamp < curr.getArrival()) { timestamp = curr.getArrival(); }
        pthread_mutex_lock(&mutex);
        file << setfill('0') << setw(2) << timestamp / 1000 << "s: Process "
             << curr.name << " arrived. Total size " << curr.getSize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP:" << endl;
        printMap(file, mem_map);
 
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
                f.setPage(i);
                flag = true;
            }
            curr.memPages.emplace_front(f);
            size--;
            curr.miss++;
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
            sleep(0.1);
        }
        // continue referencing pages, using free list as available
        // and alg where applicable
        int localTime = timestamp; // preserving start time
        while (timestamp < localTime + curr.getService() && timestamp < total_time) {
            if (timestamp % 1000 == 0) { mem_map[timestamp / 1000] = curr.name; }
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
                        f.setPage(i);
                        flag = true;
                    }
                    curr.memPages.emplace_front(f);
                    curr.miss++;
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
                    if (type == 1) { curr.memPages.reverse(); }
                    Page m = curr.memPages.front();
                    curr.memPages.pop_front();
                    pgNum = m.getPage();
                    procNum = curr.name;
                    m.setPage(i);
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
            sleep(0.1);
        }
        // print job stats
        pthread_mutex_lock(&mutex);
        file << setfill('0') << setw(2) << timestamp / 1000 << "s: Process "
             << curr.name << " completed. Total size " << curr.getSize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP:" << endl;
        printMap(file, mem_map);

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

    /*for (int i = 0; i < num_threads; i++) {
        int type = 1;
        mfu_output.open("mfu.txt", ofstream::app);
        mfu_output << "USING MFU.\n\n";
        mfu_output.close();
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

    return 0;
}
