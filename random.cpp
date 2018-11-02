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

int TotalTime = 60;
int CurrentTime = 0;
list <page> memoryFreePage;
list <process> JobList;
list <process> DoneJob;

int arrSize[4] = {5, 11, 17, 31};
pthread_mutex_t mut;

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

    page->bindProcessName = "";
    page->pageNumber = 0;
}

bool IsUsed(const string name) {
    string str = "";
    return str.compare(name) == 0 ? false : true;
}

Page *GetPage(page pa) {
    //pa = memoryFreePage.front();
    //memoryFreePage.pop_front();
    //return &pa;
    Page *pg = new Page;
    pg = &memoryFreePage.front();
    memoryFreePage.pop_front();
    return pg;
}

bool sortArrTime(const process &m1, const process &m2) {
    return m1.arrivalTime < m2.arrivalTime;
}

void initVariable() {
     TotalTime = 60;
     CurrentTime = 0;
     JobList.clear();
     DoneJob.clear();
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
        prc->hitCounts++;
        Reset(pag);
    }else{
        prc->missingCount++;
    }



    pag->bindProcessName = prc->name;
    pag->pageNumber = number;
    prc->pageList[number] = *pag;

    ofile.open("run.txt", ios::app);
    ofile << setfill('0') << setw(2) << CurrentTime
          << "s: Process " << prc->name << " referenced page " << number
          << ". Page not in memory.\n";
    ofile << "--------------------------------------------\n";
    ofile.close();
   // prc->missingCount++;
}

int locality(int i, int totalPages) {

    int delta, j = 0;
    int r = rand() % 10;
    if (r < 7) {
      int r2=rand()%2;
         if (r2==0){
            return totalPages-1;
       }else{
            return totalPages+1;
         }
    } else {
        int r3=rand()%9;
        return r3+totalPages;
    }
    j = (((i + delta) % totalPages) + totalPages) % totalPages;
    return j;

}


void RandomRepalce(process *prc, int pageNumber) {
    int oldKey = rand() % 100 + 1;
    string value;
    page *pag;
    //list<page>::iterator findRan;
    //list<page>::iterator deleteRan;
    for (auto findRan = memoryFreePage.begin(); findRan != memoryFreePage.end(); findRan++) {
        if (findRan->pageNumber == oldKey) {
            value = findRan->bindProcessName;
            break;
        }
    }

    // delete
    for (auto deleteRan = memoryFreePage.begin(); deleteRan != memoryFreePage.end(); deleteRan++) {
        if (deleteRan->bindProcessName == value) {
            memoryFreePage.erase(deleteRan);
        }
    }
    pag->pageNumber = pageNumber;
    prc->pageList[pageNumber] = *pag;

    ofile.open("run.txt", ios::app);
    ofile << setfill('0') << setw(2) << CurrentTime
          << "s: Process " << prc->name << " referenced page " << pageNumber
          << ". Page not in memory. Process " << prc->pageList[pageNumber].pageNumber << ", page "
          << oldKey << " evicted.\n";
    ofile << "--------------------------------------------\n";
    ofile.close();
}


static void *Run(void *arg) {
    pthread_mutex_lock(&mut);
    CurrentTime = 0;
    int test = 0;
    while (JobList.size() != 0) {
        int currentReferenceNumber = 0;
        process job = JobList.front();
        ofile.open("run.txt", ios::app);
        ofile << setfill('0') << setw(2) << CurrentTime
              << "s: Process " << job.name << " referenced page "
              << currentReferenceNumber << ". Page not in memory.\n " << endl;
        ofile << "--------------------------------------------\n";
        JobList.pop_front();
        ofile.close();

        if (memoryFreePage.size() > 4) {
            AssignPage(&job, currentReferenceNumber);
            auto CurrentTime = 1;
            sleep(1);
        }
        auto localTime = CurrentTime;

        while (CurrentTime < localTime + job.serviceTime && CurrentTime < TotalTime) {
            auto nextReferenceNumber = locality(currentReferenceNumber, job.size);
            if (job.pageList[nextReferenceNumber].bindProcessName != "") {

                ofile.open("run.txt", ios::app);
                ofile << setfill('0') << setw(2) << CurrentTime
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

            CurrentTime += 1;
            sleep(1);
        }

        map<int, page>::iterator it;
        map<int, page>::iterator eraseIt;
        it = job.pageList.begin();

        while (it != job.pageList.end()) {
            if (it->second.pageNumber != 0) {
                job.pageList.erase(it++);
            } else {
                it++;
            }

            //memoryFreePage.push_back(job.pageList[it]);

        }


        ofile.open("run.txt", ios::app);
        ofile << setfill('0') << setw(2) << CurrentTime
              << "s: Process " << job.name << " completed. Total size "
              << job.size << "MB. Duration " << job.serviceTime << "s.\n";
        ofile << "--------------------------------------------\n";
        ofile.close();

        DoneJob.push_back(job);
    }
    pthread_mutex_unlock(&mut);
}

int main() {
    int totalHits=0;
    int totalMiss = 0;
    int totalComplete=0;
    for(int i=0;i<5;i++){
     pthread_t t[25];
     initVariable();
     cout << "Init finish!" << endl;
     pthread_mutex_init(&mut, NULL);

     for (int i = 0; i < 25; ++i) {
        pthread_create(&t[i], NULL, Run, NULL);
    }


    for (int i = 0; i < 25; ++i) {
        pthread_join(t[i], NULL);
    }


    int hits = 0, misses = 0, complete = 0;
    for (auto it = DoneJob.begin(); it != DoneJob.end(); ++it) {
        process p = *it;
        hits += p.hitCounts;
        misses += p.missingCount;
        complete++;
    }


    ofile.open("run.txt", ios::app);
    ofile << " total Rum:  " << complete << " Hits: "<< hits
          << " miss " << misses << " miss%: "<< float(misses)/float(complete) <<" Hit%: "<< float(hits)/float(complete)<<"\n";
    ofile << "--------------------------------------------\n";
    ofile.close();
        totalHits += hits;
        totalMiss += misses;
        totalComplete += complete;


    //pthread_exit(NULL);
    cout << "end!" << endl;
    }

    ofile.open("run.txt", ios::app);
    ofile << " total Rum:  " << totalComplete << " Hits: "<< totalHits
          << " miss " << totalMiss << " miss%: "<< float(totalMiss)/float(totalComplete) <<" Hit%: "<< float(totalHits)/float(totalComplete)<<"\n";
    ofile << "--------------------------------------------\n";
    ofile.close();
    return 0;
}
