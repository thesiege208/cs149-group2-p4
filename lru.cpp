/* SJSU Fall 2018
 * CS149 Operating Systems
 * Project 4
 * Group 2
 *
 * LRU Algorithm
 * Base implemenation credits to Sijing
 */

#include "common1.hpp"	
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <string>
#include <list>
#include <pthread.h>
#include <unistd.h>
#include <algorithm>

using namespace std;

const int total_time = 60000; 				// msec

const int num_threads = 20;					// any # >=20 of threads
const int num_process = 150;					// total processes
const int num_page = 100;					// page cache size
int free_pages = 100;					// unused pages

int counter = 0;					// thread counter

ofstream output;

pthread_mutex_t smutex = PTHREAD_MUTEX_INITIALIZER; 	// start mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;	// process mutex
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;	// page mutex
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;	// output mutex
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;	// done list mutex
pthread_cond_t scond = PTHREAD_COND_INITIALIZER;	// start condition
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;	// page condition

list<Page> page_list;					// page list
list<Process> jobs;					// job queue
list<Process> done;					// finished jobs

list<int> memMap;					// memory map

void pMap(ofstream &file) {
	file << "[ ";
	for (auto const& i : memMap) { file << i << " "; }
	file << "]" << endl;
}

void *paging(void *i) {
	int timer;					// process timer
	int timeStamp = 0;				// time of each printed event
	counter++;					// increment thread counter
	output.open("output.txt", ofstream::app);

	// broadcast condition when all threads ready
	pthread_mutex_lock(&smutex);			// lock starting mutex
	if (counter == num_threads) { pthread_cond_broadcast(&scond); }
	else { pthread_cond_wait(&scond, &smutex); }	// else wait for starting condition
	pthread_mutex_unlock(&smutex);			// unlock starting mutex

	output.close();
	output.open("output.txt", ofstream::app);
	
	// while there are jobs, and time not expired
	while (!jobs.empty() && timeStamp < total_time) {
		// process lock/unlock
		pthread_mutex_lock(&mutex1);		// lock process mutex
		Process proc = jobs.front();		// grab process
		jobs.pop_front();
		timer = proc.getService();

		if (timeStamp < proc.getArrival()) { timeStamp = proc.getArrival(); }
		output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
			<< proc.name << " arrived. Size: " << proc.getSize()
			<< "MB. Duration: " << proc.getService()/1000 << "s.\n"
			<< "Memory Map: \n";
		pMap(output);
		pthread_mutex_unlock(&mutex1);		// unlock process mutex
		
		// check for free pages
		pthread_mutex_lock(&mutex2);		// lock page mutex	
		if (free_pages < 4) { pthread_cond_wait(&cond2, &mutex2); } // *** REMEMBER TO BROADCAST ***
		pthread_mutex_unlock(&mutex2);		// unlock page mutex	
		
		int c = 0;				// iteration count;		
		int i = 0;				// first page iteration at 0
		int procNum, pgNum;			// to save old data
		bool flag = false;			// eviction flag
		Page p;		

		int localTime = timeStamp;		// save starting time
		while (timeStamp < localTime + proc.getService() && timeStamp < total_time) {
			if (timer <= 0) { break; }		// stop process if service time is out
			bool ref = false;			// if page is reference

			if (c > 0){				// after the first iteration, do this
				i = locality(i, proc.getSize());	// get next page
				timer -= 100;				// decrement time
				timeStamp += 100;
				for (auto it = proc.memPages.begin(); it != proc.memPages.end(); it++) {
					if (it->getPage() == i) {	// check if next page is referenced in the process
						proc.hit++;
						it->addCount();
						pthread_mutex_lock(&mutex3);
						output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
							<< proc.name << " referenced page " << i << ". Page in memory. No Eviction." << endl;
						pthread_mutex_unlock(&mutex3);
						ref = true;		// set flag to true
						break;
					}
				}
				sleep(0.1); 
			} 
			
			// main LRU algorithm part
			if (!ref) {					// if next page not referenced in current process
				// page lock/unlock
				pthread_mutex_lock(&mutex2);		// lock page mutex
				p = page_list.back();			// grab next page from back
				page_list.pop_back();
				memMap.pop_back();
				if (p.getProcess() != -1) { 
					procNum = p.getProcess();	// save old process name
					pgNum = p.getPage();		// and page number
					flag = true; 
				} // set flag true if page has old data

				p.setProcess(proc.name);		// set new process name
				p.setPage(i);				// and page number
				int k = proc.name;

				proc.memPages.push_front(p);		// add current page to current process
				if (free_pages > 0) { free_pages--; }	// decrement unused pages
				proc.miss++;
				page_list.push_front(p);		// move page to front of page_list
				memMap.push_front(k);
				output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
					<< proc.name << " referenced page " << i << ". Page not in memory. ";

				if (flag) { output << "Process " << procNum << ": Page " << pgNum << " evicted." << endl; }
				else { output << "No eviction." << endl; }
				pthread_mutex_unlock(&mutex2);		// unlock page mutex
				flag = false;
			} 
			// add
			c++;
		} // end inner while loop

		//for (auto it = proc.memPages.begin(); it != proc.memPages.end();) { free_pages++; }
		
		pthread_mutex_lock(&mutex2);
		free_pages += proc.memPages.size();
		if (free_pages >= 4) { pthread_cond_broadcast(&cond2); }
		pthread_mutex_unlock(&mutex2);

		pthread_mutex_lock(&mutex3);
		output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
				<< proc.name << " Completed. Size: " << proc.getSize()
				<< "MB. Duration: " << proc.getService()/1000 << "s.\n"
				<< "Memory Map: \n";
		//printMap(output, mem_map);
		pMap(output);
		pthread_mutex_unlock(&mutex3);
		
		pthread_mutex_lock(&mutex4);
		done.push_front(proc);
		pthread_mutex_unlock(&mutex4);
	} // end outer while loop
	output.close();	
	pthread_cancel(pthread_self());
}

int main() {
	int hit = 0, miss = 0; // total hits and misses
	int i = 0;
	
	srand(time(NULL));
	pthread_t threads[num_threads];
	output.open("output.txt");
	output.close();

	// generate 100 pages, create memory map
	for (i = 0; i < num_page; i++) {
		Page page = Page();
		page_list.push_front(page);
		memMap.push_front(0);
	}
	pMap(output);
	// generate 150 processes, sort by arrival
	for (i = 0; i <= num_process; i++) {
		Process p = Process();
		p.name = i+1;
		jobs.push_front(p);
	}
	jobs.sort();	

	// create threads
	for (i = 0; i < num_threads; i++) {
		pthread_create(&threads[i], NULL, paging, reinterpret_cast<void*>(i));
	}
	// join threads
	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	// calculate stats
	for (auto it = done.begin(); it != done.end(); it++) {
		Process p = *it;
		hit += p.hit;
		miss += p.miss;
	}
	
	output.open("output.txt", ofstream::app);
	output << "Hits: " << hit << "\tMiss: " << miss << endl;
	output.close();	
	
	return 0;
}



