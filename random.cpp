#include <iostream>
#include <map>
#include <list>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include <sys/time.h>
#include <pthread.h>

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

int TotalTime = 60000;
int CurrentTime = 0;
list <page> memoryFreePage;
list <process> JobList;
int arrSize[4] = {5, 11, 17, 31};
mutex mut;

int64_t getCurrentTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec;
}

void Reset(page *page) {
    ofile.open("run.txt", ios::app);
    ofile << "Process " << page->bindProcessName << ", page " <<
          page->pageNumber << " evicted." << endl;
    ofile.close();
    cout << "Process " << page->bindProcessName << ", page " <<
         page->pageNumber << " evicted." << endl;
    page->bindProcessName = "";
    page->pageNumber = 0;
}

bool IsUsed(const string name) {
    string str = "";
    return str.compare(name) == 0 ? false : true;
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

    int j = 1;
    page pagInit;
    int k = 0;

    list<process>::iterator job;
    job = JobList.begin();
    int y = job->size;
    while (k < 5) {
        while (y > 0) {
            pagInit.pageNumber = j;
            pagInit.bindProcessName = job->name;
            memoryFreePage.push_back(pagInit);
            j++;
            job++;
            y--;
        }
        k++;
    }

    while (j < 100) {
        pagInit.pageNumber = j;
        pagInit.bindProcessName = "";
        memoryFreePage.push_back(pagInit);
        j++;
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

    ofile.open("run.txt", ios::app);
    ofile << setfill('0') << setw(2) << CurrentTime / 1000
          << "s: Process " << prc->name << " referenced page " << number
          << ". Page not in memory.\n";
    ofile << "--------------------------------------------\n";
    ofile.close();
    prc->missingCount++;
}

int locality(int i, int totalPages) {
    cout << "Job total: " << totalPages << endl;

    int delta, j = 0;
    int r = rand() % 10;
    if (r < 7) {
        delta = rand() % 3 - 1;
    } else {
        delta = rand() % (totalPages - 2) + 2;
    }
    j = (i + delta) % totalPages;
    cout << "Job J: " << j << endl;
    return j;

}





void RandomRepalce(process *prc, int pageNumber) {
    cout << "replace" << endl;
    int oldKey = rand() % 100 + 1;
    string value;
    page *pag;
    list<page>::iterator findRan;
    list<page>::iterator deleteRan;
    cout << "oldKey" << oldKey << endl;
    for (findRan = memoryFreePage.begin(); findRan != memoryFreePage.end(); findRan++) {
        if (findRan->pageNumber == oldKey) {
            value = findRan->bindProcessName;
            break;
        }
    }

    // delete
    for (deleteRan = memoryFreePage.begin(); deleteRan != memoryFreePage.end(); deleteRan++) {
        if (deleteRan->bindProcessName == value) {
            memoryFreePage.erase(deleteRan);
        }
    }


    pag->pageNumber = pageNumber;
    prc->pageList[pageNumber] = *pag;

    ofile.open("run.txt", ios::app);
    ofile << setfill('0') << setw(2) << CurrentTime / 1000
          << "s: Process " << prc->name << " referenced page " << pageNumber
          << ". Page not in memory. Process " << prc->pageList[pageNumber].pageNumber << ", page "
          << oldKey << " evicted.\n";
    ofile << "--------------------------------------------\n";

    ofile.close();
}


static void *Run(void *arg) {
    mut.lock();
    CurrentTime = 0;
    int test = 0;
    while (JobList.size() != 0) {
        cout << "test" << JobList.size() << endl;
        int currentReferenceNumber = 0;
        process job = JobList.front();
        ofile.open("run.txt", ios::app);
        ofile << setfill('0') << setw(2) << CurrentTime / 1000
              << "s: Process " << job.name << " referenced page "
              << currentReferenceNumber << ". Page not in memory.\n " << endl;
        ofile << "--------------------------------------------\n";
        JobList.pop_front();
        ofile.close();

        if (memoryFreePage.size() > 4) {
            AssignPage(&job, currentReferenceNumber);
            auto CurrentTime = 100;
            usleep(100);
        }
        auto localTime = CurrentTime;
        cout << "test2 " << JobList.size() << endl;

        while (CurrentTime < localTime + job.serviceTime && CurrentTime < TotalTime) {
            auto nextReferenceNumber = locality(currentReferenceNumber, job.size);
            if (job.pageList[nextReferenceNumber].bindProcessName != "") {
                job.hitCounts++;
                ofile.open("run.txt", ios::app);
                ofile << setfill('0') << setw(2) << CurrentTime / 1000
                      << "s: Process " << job.name << " referenced page "
                      << nextReferenceNumber << ". Page in memory. No page evicted.\n"
                      << endl;
                ofile << "--------------------------------------------\n";

                ofile.close();

            } else {
                if (memoryFreePage.size() != 0) {
                    AssignPage(&job, nextReferenceNumber);
                } else {
                    RandomRepalce(&job, nextReferenceNumber);
                }
            }

            CurrentTime += 100;
            usleep(10000);
        }

        map<int, page>::iterator it;
        map<int, page>::iterator eraseIt;
        it = job.pageList.begin();

        cout << "size " << job.pageList.size() << endl;
        while (it != job.pageList.end()) {
            cout << "test4" << "it" << endl;
            //eraseIt=it;
            if (it->second.pageNumber != 0) {
                job.pageList.erase(it++);
            } else {
                it++;
            }
            cout << "test4 mid" << " it" << endl;
            cout << "=====" << it->second.pageNumber << endl;
            cout << "test4 end" << " it" << endl;



            //memoryFreePage.push_back(job.pageList[it]);

        }
        cout << "test5 " << endl;


        ofile.open("run.txt", ios::app);
        ofile << setfill('0') << setw(2) << CurrentTime / 1000
              << "s: Process " << job.name << " completed. Total size "
              << job.size << "MB. Duration " << job.serviceTime << "s.\n";
        ofile << "--------------------------------------------\n";
        ofile.close();
    }
    mut.unlock();
}

int main() {
    pthread_t t[25];
    initVariable();
    cout << "Init finish!" << endl;
    for (int i = 0; i < 25; ++i) {
        pthread_create(&t[i], NULL, Run, NULL);
    }
    for (int i = 0; i < 25; ++i) {
        pthread_join(t[i], NULL);
    }
    //pthread_exit(NULL);
    cout << "end!" << endl;
    return 0;
}