/* SJSU Fall 2018
 * CS149 Operating Systems
 * Sijing Xie (013829478), Group 2
 *
 * This header file includes some declarations used
 * in the LFU/MFU page allocation simulations.
 */

#include <cstdlib>
#include <forward_list>

class Process {
    const int sizes[4] = {5, 11, 17, 31};

    int size; // in MB
    int arrival; // in secs
    int service; // in secs
    
    public:

    int name;
    forward_list<Page> memPages;

    Process() {
        size = sizes[rand() % 4];
        arrival = rand() % 60;
        service = rand() % 5 + 1;
        memPages = forward_list(size);
    }
    
    int getSize() { return size; }
    int getArrival() { return arrival; }
    int getService() { return service; }
    
    bool operator <=(Process & processObj) {
        return arrival <= processObj.getArrival();
    }
};

class Page {
};

/* helper function to calculate locality of reference. */
int locality() {
}
