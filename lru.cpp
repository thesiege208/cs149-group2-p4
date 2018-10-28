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
#include <bits/stdc++.h>				// for unordered map

using namespace std;

const int total_time = 60000; 				// msec
int timeStamp = 0;					// time of each printed event

int num_threads = 20;					// any # >=20 of threads
int num_process = 150;					// total processes
int num_page = 100;					// page cache size
int free_pages = 100;					// unused pages

int counter = 0;					// thread counter
int mem_map[total_time / 1000] = {0};

ofstream output;

pthread_mutex_t smutex = PTHREAD_MUTEX_INITIALIZER; 	// start mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;	// process grab mutex
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;	// page grab mutex
pthread_cond_t scond = PTHREAD_COND_INITIALIZER;	// start condition
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;	// page grab condition

list<Page> page_list;					// page list
list<Process> jobs;					// job queue
list<Process> done;					// finished jobs
unordered_map<int, list<Page>> pmap;			// page map, <proc.name, proc.memPages>


void printMap(ofstream &file) {
	int len = 100; total_time/1000;
	for (int i = 0; i < len; i++) {
		if (i % 10 == 0) { file << "\n"; }
		if (mem_map[i] == 0) { file << ". "; }
		else { file << mem_map[i] << " "; }
	}
	file << "\n";
}

void *paging(void *i) {
	int timer;					// process timer
	counter++;					// increment thread counter
	//printf("DEBUG 1: %i\n", counter);
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
		output 	<< "debug A: " << timeStamp << endl;
		// process lock/unlock
		pthread_mutex_lock(&mutex1);		// lock process mutex
		output 	<< "debug A1: mutex1 lock" << endl;		
		Process proc = jobs.front();		// grab process
		jobs.pop_front();
		int nm = proc.name;
		timer = proc.getService();
		if (timeStamp < proc.getArrival()) { timeStamp = proc.getArrival(); }
		output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
			<< proc.name << " arrived. Size: " << proc.getSize()
			<< "MB. Duration: " << proc.getService()/1000 << "s.\n"
			<< "Memory Map: \n";
		//printMap(output);
		pthread_mutex_unlock(&mutex1);		// unlock process mutex
		output 	<< "debug A2: mutex1 unlock" << endl;
		
		// check for free pages
		pthread_mutex_lock(&mutex2);		// lock page mutex
		output 	<< "debug B1: mutex2 lock" << endl;		
		if (free_pages < 4) { 				 		// *** REMEMBER TO BROADCAST ***
			output 	<< "debug B2: " << endl;
			pthread_cond_wait(&cond2, &mutex2); 
		}
		pthread_mutex_unlock(&mutex2);		// unlock page mutex
		output 	<< "debug B3: mutex2 unlock" << endl;

		if (timeStamp >= total_time) { 
			output 	<< "debug C: " << timeStamp << endl;
			break; 
		} // multi-threading check	
		
		int c = 0;				// iteration count;		
		int i = 0;				// first page iteration at 0
		int procNum, pgNum;			// to save old data
		bool flag = false;			// eviction flag
		Page p;					
		list<Page> pg;

		int localTime = timeStamp;		// save starting time
		output 	<< "debug Start: " << endl;
		while (timeStamp < localTime + proc.getService() && timeStamp < total_time) {
			output 	<< "debug D: " << timeStamp << endl;
			if (timer == 0) { 
				output << "debug E: " << endl; 
				break; 
			}		// stop process if service time is out
			bool ref = false;			// if page is reference

			if (c > 0){				// after the first iteration, do this
				output 	<< "debug F: " << endl;				
				i = locality(i, proc.getSize());	// get next page
				pg = pmap[nm];
				if (proc.memPages != pg) { 
					proc.memPages = pg;
					output 	<< "debug G: " << endl; 
				}
				for (auto it = proc.memPages.begin(); it != proc.memPages.end(); it++) {
					if (it->getPage() == i) {	// check if next page is referenced in the process
						output 	<< "debug H: " << endl;
						proc.hit++;
						it->addCount();
						output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
							<< proc.name << " referenced page " << i << ". Page in memory. No Eviction." << endl;
						ref = true;		// set flag to true
						break;
					}
				} 
			} 
			
			// main LRU algorithm part
			if (!ref) {					// if next page not referenced in current process
				output 	<< "debug I: " << endl;
				// page lock/unlock
				pthread_mutex_lock(&mutex2);		// lock page mutex
				p = page_list.back();			// grab next page from back
				page_list.pop_back();
				if (p.getProcess() != -1) {		// if page has old data
					output 	<< "debug J: " << endl;
					procNum = p.getProcess();	// save old process name
					pgNum = p.getPage();		// and page number
					pg = pmap[procNum];		// find page list in map
					pg.remove(p);			// remove page from page list
					pmap[procNum] = pg;		// update page list of that process in map
					p.setProcess(proc.name);	// set new process name
					p.setPage(i);			// and page number
					flag = true;			// eviction flag true
				}
				proc.memPages.push_front(p);		// add current page to current process
				free_pages--;				// decrement unused pages
				proc.miss++;
				page_list.push_front(p);		// move page to front of page_list
				output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
					<< proc.name << " referenced page " << i << ". Page not in memory. ";
				if (flag) { output << "Process " << procNum << ": Page " << pgNum << " evicted." << endl; }
				else { output << "No eviction." << endl; }
				flag = false;
				pmap[nm] = proc.memPages;		// add/update process and pages to map
				pthread_mutex_unlock(&mutex2);		// unlock page mutex
				output 	<< "debug K: " << endl;
			} 
			// add
			if (c > 0){
				timer -= 100;				// decrement time
				output 	<< "debug L: " << timer << " " << timeStamp << endl;
				timeStamp += 100;
				//if (timeStamp % 1000 == 0) { mem_map[timeStamp/1000] = proc.name; }
				sleep(0.1);				// locality time is 100 msec
			}
			c++;
			output 	<< "debug M: " << c << endl;
		} // end inner while loop
		output 	<< "debug N: " << endl;
		done.push_front(proc);
		for (auto it = proc.memPages.begin(); it != proc.memPages.end();) {
			p = *it;
			++it;
			proc.memPages.pop_front();
			page_list.push_front(p);
			free_pages++;
		}
		output 	<< "debug O: " << endl;
		pthread_mutex_lock(&mutex2);			// lock starting mutex
		if (free_pages >= 4) { pthread_cond_broadcast(&cond2); }
		pthread_mutex_unlock(&mutex2);
		output 	<< "debug P: " << endl;
		output 	<< setfill ('0') << setw(2) << timeStamp/1000 << "s. Process "
				<< proc.name << " Completed. Size: " << proc.getSize()
				<< "MB. Duration: " << proc.getService()/1000 << "s.\n"
				<< "Memory Map: \n";
		//printMap(output);
		
	} // end outer while loop
	output 	<< "debug Q: " << endl;
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

	// generate 100 pages
	for (i = 0; i < num_page; i++) {
		Page page = Page();
		page_list.push_front(page);
	}
	
	for (i = 0; i <= num_process; i++) {
		Process p = Process();
		p.name = i;
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



