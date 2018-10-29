

#include <iostream>
#include <map>
#include <list>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include <sys/time.h>

using namespace std;

ofstream ofile;

typedef struct Page {
    int pageNumber;
    string bindProcessName;
} page;

typedef struct Prcoess {
    string name;
    int size;
    int arrivalTime;
    int serviceTime;
    map<int, page> pageList;
    int hitCounts;
    int missingCount;
} process;

int TotalTime = 6000;
int CurrentTime = 0;
list<page> memoryFreePage(100);
list<Prcoess> JobList(150);
int arrSize[4] = {5, 11, 17, 31};


int64_t getCurrentTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec;
}

void Reset(page *page) {

    ofile.open("page.txt");
    ofile << "Process " << page->bindProcessName << ", page " <<
          page->pageNumber << " evicted."<<endl;
    ofile.close();

    cout << "Process " << page->bindProcessName << ", page " <<
         page->pageNumber << " evicted." << endl;

    page->bindProcessName = "";
    page->pageNumber = 0;
}

bool IsUsed(const string name) {
    string str = "";
    return str.compare(name) == 0 ? true : false;
}

page *GetPage(page pa) {
    pa = memoryFreePage.front();
    memoryFreePage.pop_front();
    return &pa;
}

bool sortArrTime(const process &m1, const process &m2) {
    return m1.arrivalTime < m2.arrivalTime;
}

void initVariable() {
    for (int i = 0; i < 150; ++i) {
        process proInit;
        int size = (rand() % 4);
        proInit.size = arrSize[size];
        proInit.arrivalTime = (rand() % 6000) + 1;
        proInit.serviceTime = (rand() % 5) + 1;
        char s[40];
        sprintf(s, "%s%d", "P", proInit.arrivalTime);
        proInit.name = s;

        JobList.push_back(proInit);
    }
    JobList.sort(sortArrTime);
    for (int j = 0; j < 100; ++j) {
        page pagInit;
        pagInit.pageNumber = 0;
        pagInit.bindProcessName = "";
        memoryFreePage.push_back(pagInit);
    }
}

void AssignPage(process *prc, int number) {
    auto pag = GetPage(prc->pageList[number]);
    if (IsUsed(pag->bindProcessName)) {
        Reset(pag);
    }
    pag->bindProcessName = prc->name;
    pag->pageNumber = number;
    prc->pageList[number] = *pag;

    ofile.open("AssignPage.txt");
    ofile << setfill('0') << setw(2) << getCurrentTime() / 1000
          << "s: Process " << prc->name << " referenced page " << prc->pageList[number].pageNumber
          << ". Page not in memory. ";
    ofile.close();
    prc->missingCount++;
}

int locality(int i, int totalPages) {
    int delta, j;
    int r = rand() % 10;
    if (r >= 0 && r < 7) {
        delta = rand() % 3 - 1;
        j = i + delta;
    } else {
        delta = rand() % (totalPages - 2);
        j = rand() % delta;
    }
    return j;
}

void RandomRepalce(process *prc, int pageNumber) {
    int oldKey = rand() % 150 + 1;
    page *page;

    page->pageNumber = pageNumber;
    prc->pageList[pageNumber] = *page;

    ofile.open("RandomRepalce.txt");
    ofile << setfill('0') << setw(2) << getCurrentTime() / 1000
          << "s: Process " << prc->name << " referenced page " << pageNumber
          << ". Page not in memory. Process " << prc->pageList[pageNumber].pageNumber << ", page "
          << oldKey << " evicted.";
    ofile.close();
}

void Run() {
    CurrentTime = getCurrentTime();
    while (JobList.size() != 0) {
        int currentReferenceNumber = 0;
        Prcoess job = JobList.front();
        JobList.pop_front();
        ofile.open("Run.txt");
        ofile << setfill('0') << setw(2) << getCurrentTime() / 1000
              << "s: Process " << job.pageList[0].bindProcessName << " referenced page "
              << job.pageList[0].pageNumber << ". Page not in memory. " << endl;
        ofile.close();

        if (memoryFreePage.size() > 4) {
            AssignPage(&job, currentReferenceNumber);
            auto CurrentTime = 100;
            usleep(100);;
        }
        auto localTime = CurrentTime; // preserving start time
        while (CurrentTime < localTime + job.serviceTime && CurrentTime < TotalTime) {
            auto nextReferenceNumber = locality(currentReferenceNumber, job.pageList[1].pageNumber);
            if (job.pageList[nextReferenceNumber].bindProcessName != "") {
                job.hitCounts++;
                ofile.open("Run.txt");
                ofile << setfill('0') << setw(2) << getCurrentTime() / 1000
                      << "s: Process " << job.name << " referenced page "
                      << job.hitCounts << ". Page in memory. No page evicted."
                      << endl;
                ofile.close();

            } else {
                if (memoryFreePage.size() != 0) {
                    AssignPage(&job, nextReferenceNumber);
                } else {
                    RandomRepalce(&job, nextReferenceNumber);
                }
            }

            CurrentTime += 100;
            usleep(100);
        }
        map<int, page>::iterator it;
        it = job.pageList.begin();
        while (it != job.pageList.end()) {
            if (it->first != 0) {
                job.pageList.erase(it);
            }
            it++;
        }
        ofile.open("Run.txt");
        ofile << setfill('0') << setw(2) << getCurrentTime() / 1000
              << "s: Process " << job.name << " completed. Total size "
              << job.size << "MB. Duration " << job.serviceTime / 1000 << "s.\n"
              << "MEMORY MAP. MUST REPLACE!\n";
        ofile.close();
    }
}

int main() {
    initVariable();
    cout<<"Init finish!"<<endl;
    Run();
    return 0;
}
