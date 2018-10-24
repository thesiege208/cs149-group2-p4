/* SJSU Fall 2018
 * CS149 Operating Systems
 * Sijing Xie (013829478), Group 2
 *
 * This header file includes some declarations used
 * in the LFU/MFU page allocation simulations.
 */

#include <cstdlib>
#include <forward_list>

class Page {
    int counter; // track # references
    int pageNum; // process page loaded in
    int processNum; // process using this page

    public:
    Page() {
        counter = 0;
        pageNum = -1;
        processNum = -1;
    }
    
    int getCount() { return counter; }
    void addCount() { counter++; }
    void resetCount() { counter = 0; }

    void setPage(int n) { pageNum = n; }
    int getPage() { return pageNum; }

    void setProcess(int n) { processNum = n; }
    int getProcess() { return processNum; }
    
    bool operator <(Page pageObj) const {
        return counter <= pageObj.getCount();
    }
};

class Process {
    const int sizes[4] = {5, 11, 17, 31};

    int size; // in MB
    int arrival; // in msecs
    int service; // in msecs
    
    public:
    int name;
    int hit = 0;
    int miss = 0;
    std::forward_list<Page> memPages;

    Process() {
        size = sizes[rand() % 4];
        arrival = (rand() % 60) * 1000;
        service = (rand() % 5 + 1) * 1000;
        memPages = std::forward_list<Page>(size);
    }
    
    int getSize() { return size; }
    int getArrival() { return arrival; }
    int getService() { return service; }
    
    bool operator <(Process processObj) const {
        return arrival <= processObj.getArrival();
    }
};

/* helper function to calculate locality of reference.
 * takes previously reference pg # as arg, returns page # for next ref pg.
 */
int locality(int i, int totalPages) {
    int delta, j;
    int r = rand() % 10;
    if (r >= 0 && r < 7) {
        delta = rand() % 3 - 1;
        j = i + delta;
    }
    else {
        delta = rand() % (totalPages - 2);
        j = rand() % delta;
    }
    return j;
}
