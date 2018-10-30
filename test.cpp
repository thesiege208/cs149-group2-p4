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

int main() {
    // test setting page num/process num/counter to process mempages
    forward_list<Page> pgs;
    pgs.emplace_front(Page());
    pgs.emplace_front(Page());
    pgs.emplace_front(Page());
    forward_list<Process> procs;
    for (int i = 0; i < 3; i++) {
        Process proc = Process();
        proc.name = i + 1;
        procs.emplace_front(proc);
    }
    procs.sort();
    Process p = procs.front();
    procs.pop_front(); // REMOVED OLD!!
    Page pg = pgs.front();
    pgs.pop_front();
    pg.addCount();
    pg.setPage(3);
    pg.setProcess(p.name);
    cout << "before adding to mempages: count=" << pg.getCount() << " page=" << pg.getPage() << " process=" << pg.getProcess() << endl;
    p.memPages.emplace_front(pg);
    procs.emplace_front(p); // REPLACE NEW!!
    Page page = p.memPages.front();
    cout << "after adding to mempages: count=" << page.getCount() << " page=" << page.getPage() << " process=" << page.getProcess() << endl;
}
