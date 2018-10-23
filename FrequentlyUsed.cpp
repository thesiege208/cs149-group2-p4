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
forward_list<Page> free = forward_list(num_free); // linked list of free pages
int size = 0; // to track size of free list
forward_list<Process> jobs = forward_list(num_process); // linked list of jobs to run
forward_list<Process> done = forward_list(num_process); // linked list of completed jobs

/* least frequently used replacement algorithm. */
void lfu(Process p) {
    /*// reference more pages using free list as available
    int i; // tracking previous page # referenced for locality
    int localTime = timeStamp; // preserving start time
    while (timeStamp < localTime + p.getService()) {
        timeStamp += 100;
        i = locality(i, p.getSize()); // get next ref page #
        // check if page already loaded
        for (auto it = p.memPages.begin(); it != p.memPages.end(); ++it) {
            if (*it.getPage() == i) {
                p.hit++;
                *it.addCount();
                lfu_output << setfill('0') << setw(2) << timeStamp / 1000
                           << "s: Process " << p.name << " referenced page "
                           << i << ". Page in memory. No process or page evicted."
                           << endl;
             }
        } else if (!free.empty()) { // check if free pages available
            pthread_mutex_lock(&mutex);
            Page f = free.pop_front();
            f.setPage(i);
            p.memPages.insert_after(f);
            size--;
            p.miss++;
            pthread_mutex_unlock(&mutex);
            file << setfill('0') << setw(2) << timeStamp / 1000
                 << "s: Process " << p.name << " referenced page " << i
                 << ". Page not in memory. No process or page evicted." << endl;
        } else { // use alg to replace page
            p.memPages.sort(); // sort in ascending order by counters
            Page m = p.memPages.pop_front();
            int prev = m.getPage();
            m.setPage(i);
            p.memPages.insert_after(m);
            p.miss++;
            file << setfill('0') << setw(2) << timeStamp / 1000
                 << "s: Process " << p.name << " referenced page " << i
                 << ". Page not in memory. Page " << prev << " evicted." << endl;
        }
        sleep(0.1);
    }
    // add job to done list and exit
    done.insert_after(p);
    return;*/
}

/* most frequently used replacement algorithm. */
void mfu(Process p) {

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
    // checking all threads ready    
    if (ready == num_threads) { pthread_cond_broadcast(&cond); }
    else { pthread_cond_wait(&cond, &mutex); }
    // choosing output file to write
    ofstream file;
    if (algType == 'L') {
        file = lfu_output;
        file << "USING LFU.\n\n";
    }
    else {
        file = mfu_output;
        file << "USING MFU.\n\n";
    }

    while (!jobs.empty()) {
        pthread_mutex_lock(&mutex);
        // get next job in queue
        Process curr = jobs.pop_front();
        file << setfill('0') << setw(2) << timeStamp / 1000 << "s: Process "
             << curr.name << " arrived. Total size " << curr.getsize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP. MUST REPLACE!";

        pthread_mutex_unlock(&mutex);
        // reference page 0
        int i = 0; // tracking page # for locality
        Page f;
        if (size >= 4) {
            pthread_mutex_lock(&mutex);
            f = free.pop_front();
            f.setPage(i);
            curr.memPages.insert_after(f);
            size--;
            curr.miss++;
            pthread_mutex_unlock(&mutex);
            file << setfill('0') << setw(2) << timeStamp / 1000
                 << "s: Process " << curr.name << " referenced page " << i
                 << ". Page not in memory. No process or page evicted." << endl;
            sleep(0.1);
        }
        // continue referencing pages, using free list as available
        // and alg where applicable
        int localTime = timeStamp; // preserving start time
        while (timeStamp < localTime + curr.getService()) {
            i = locality(i, curr.getSize()); // get next ref page #
            // check if page already loaded
            for (auto it = curr.memPages.begin(); it != curr.memPages.end(); ++it) {
                if (*it.getPage() == i) {
                    curr.hit++;
                    *it.addCount();
                    lfu_output << setfill('0') << setw(2) << timeStamp / 1000
                               << "s: Process " << curr.name << " referenced page "
                               << i << ". Page in memory. No process or page evicted."
                               << endl;
                 }
            } else if (!free.empty()) { // check if free pages available
                pthread_mutex_lock(&mutex);
                f = free.pop_front();
                f.setPage(i);
                curr.memPages.insert_after(f);
                size--;
                curr.miss++;
                pthread_mutex_unlock(&mutex);
                file << setfill('0') << setw(2) << timeStamp / 1000
                     << "s: Process " << curr.name << " referenced page " << i
                     << ". Page not in memory. No process or page evicted." << endl;
            } else { // use alg to replace page
                curr.memPages.sort(); // sort in ascending order by counters
                if (algType == 'M') { curr.memPages.reverse(); }
                Page m = curr.memPages.pop_front();
                int prev = m.getPage();
                m.setPage(i);
                curr.memPages.insert_after(m);
                curr.miss++;
                file << setfill('0') << setw(2) << timeStamp / 1000
                     << "s: Process " << curr.name << " referenced page " << i
                     << ". Page not in memory. Page " << prev << " evicted." << endl;
            }
            timeStamp += 100;
            sleep(0.1);
        }
        // add job to done list and print job stats
        done.insert_after(curr);
        file << setfill('0') << setw(2) << timeStamp / 1000 << "s: Process "
             << curr.name << " completed. Total size " << curr.getsize()
             << "MB. Duration " << curr.getService() / 1000 << "s.\n"
             << "MEMORY MAP. MUST REPLACE!";
    }
    
    pthread_cancel(pthread_self());
    return NULL;
}
 
int main() {
    srand(time(NULL));

    pthread_t threads[num_threads];

    lfu_output.open("lfu.txt");
    mfu_output.open("mfu.txt");

    // gen 100 page free list, each 1MB.
    for (int j = 0; j < num_free; j++) {
        Page pg = page();
        free.emplace_front(pg);
        size++;
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

    // calc avgs, print to output file.
    
    for (int i = 0; i < numberOfSellers; i++) {
        pthread_join(threads[i], NULL);
    }

    lfu_output.close();
    mfu_output.close()

    return 0;
}
