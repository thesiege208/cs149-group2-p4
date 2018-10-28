/* SJSU Fall 2018
 * CS149 Operating Systems
 * Project 4
 * Group 2
 * 
 * Common1 LRU Header file
 * Borrowed from Sijing
 * Minor changes
 */

#ifndef common1_hpp
#define common1_hpp

#include <cstdlib>
#include <list>
#include <iostream>	// printf for debug

//add here

class Page {
	int counter;
	int pageNum;
	int processNum;

	public:
	bool operator == (const Page& pg) const { return pageNum == pg.pageNum; }
	bool operator != (const Page& pg) const { return !operator==(pg); }
	Page() {
		counter = 0;
		pageNum = -1;
		processNum = -1;
	}

	int getCount() { return counter; }
	void addCount() { counter++; }
	void resetCount() { counter = 0; }

	int getPage() { return pageNum; }
	void setPage(int n) { pageNum = n; }

	int getProcess() { return processNum; }
	void setProcess(int n) { processNum = n; }

	bool operator <(Page pageObj) const {
		return counter <= pageObj.getCount();
	}

};

class Process {
	int sizes[4] = {5, 11, 17, 31};

	int size;	// process size
	int arrival; 	// time in msec
	int service; 	// time in msec

	public:
	int name;
	int hit = 0;
	int miss = 0;
	std::list<Page> memPages;

	Process() {
		size = sizes[rand()%4];
		arrival = (rand() % 60) * 1000;
		service = (rand() % 5 + 1) * 1000;
		memPages = std::list<Page>(size);
	}

	int getSize() { return size; }
	int getArrival() { return arrival; }
	int getService() { return service; }

	bool operator <(Process processObj) const {
		return arrival <= processObj.getArrival();
	}

};

// where i = page number, from 0 - (totalPages-1)
int locality(int i, int totalPages) {
	int delta, j;
	int r = rand() % 10;
	if (r < 7) {
		delta = rand() % 3 - 1;
		j = i + delta;
	} else {
		if ((i+2) >= totalPages) { j = rand() % (i-1); }
		else if ((i-1) <= 0) { j = (i+2) + rand() % (totalPages-(i-2)); }
		else {
			switch (rand()%2) {
				case 0: j = rand() % (i-1);
					break;
				case 1: j = (i+2) + rand() % (totalPages-(i-2));
					break;
			}
		}
	}

	if (j < 0 || j > totalPages - 1) { j %= totalPages-1; }
	return j;
}

#endif
