/* SJSU Fall 2018
 * CS149 Operating Systems
 * Sijing Xie (013829478), Group 2
 *
 * This header file includes some declarations used
 * in the LFU/MFU page allocation simulations.
 */

#include <cstdlib>
#include <string>

class Process {
    const int sizes[4] = {5, 11, 17, 31};

    int size; // in MB
    int arrival; // in secs
    int service; // in secs
    
    public:
    std::string name;

    Process() {
        size = sizes[rand() % 4];
        arrival = rand() % 60;
        service = rand() % 5 + 1;
    }
    
    int getSize() { return size; }
    int getArrival() { return arrival; }
    int getService() { return service; }
};

/* helper function to calculate locality of reference. */
int locality() {
}
