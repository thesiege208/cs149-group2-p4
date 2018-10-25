#include <iostream>
#include <map>
#include <list>
#include<fstream>
#include <unistd.h>


using namespace std;

ofstream ofile;

typedef struct pag  {
    int    PageNumber;
    string BindProcessName;
}Page, *pPage;

struct Prcoess  {
    string        Name;
    int           Size;
    int           ArrivalTime;
    int           ServiceTime;
    map<int,Page> PageList;
    int           HitCounts;
    int           MissingCount;
};


void Reset(Page *page) {

    ofile.open("/home/page.log");
    ofile << "Process " << page->BindProcessName << ", page " <<
          page->PageNumber << " evicted." << endl;
    ofile.close();

    //LOG输出
    cout << "Process " << page->BindProcessName << ", page " <<
         page->PageNumber << " evicted." << endl;

    page->BindProcessName = "";
    page->PageNumber = 0;
}

bool IsUsed(Page *page) {
    string str = "";
    return str.compare(page->BindProcessName) == 0 ? true : false;
}

void  *GetPage(Page *page){
    return NULL;
}

void initVariable(){

}


int TotalTime = 6000;
int CurrentTime = 0;
list<Page>    memoryFreePage(100); //size 100 类型Page
list<Prcoess> JobList(150);     //size 150, 类型Process

void AssignPage(Prcoess *prcoess, int number){
//    auto page = GetPage(prcoess->PageList);

//    if IsUsed(){
//        Reset(page)
//    }
//    page.BindProcessName = process.Name
//    page.PageNumber = number
//    process.PageList[number]=page
//    /*
//     file << setfill('0') << setw(2) << timestamp / 1000
//             << "s: Process " << curr.name << " referenced page " << i
//             << ". Page not in memory. ";

    prcoess->MissingCount++;
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

void RandomRepalce(Prcoess *process, int pageNumber){


    int  oldKey;
    Page *page;

//    //随机获取一个Key
//    for oldKey,page=range process->{
//        break
//    }
//    page.PageNumber = pageNumber
//    delete(process.PageList,oldKey)
//    process.PageList[pageNumber]=page
    /*
    file << setfill('0') << setw(2) << timestamp / 1000
                         << "s: Process " << curr.name << " referenced page " << pageNumber
                         << ". Page not in memory. Process " << procNum << ", page "
                         << oldKey << " evicted." << endl;
    */
}

void Run() {
    while (JobList.size() != 0 && TotalTime <= CurrentTime) {

        int currentReferenceNumber = 0;
        Prcoess job = JobList.front();


        //ofile << std::setfill(0) << std::setw(2) << timestamp / 1000
        ofile << "s: Process " << job.PageList[1].BindProcessName << " referenced page "
              << job.PageList[1].PageNumber << ". Page not in memory. " << endl;
        ofile.close();

        if (memoryFreePage.size() > 4) {
            AssignPage(job, currentReferenceNumber);
            auto CurrentTime =100;
            usleep(100);;
        }

        auto localTime = CurrentTime; // preserving start time
        while (CurrentTime<localTime+jobServiceTime && CurrentTime < TotalTime){
            auto nextReferenceNumber = locality(currentReferenceNumber,job.PageList[1].PageNumber);
            if (job.PageList[nextReferenceNumber]){
                job.HitCounts++;

                ofile << setfill('0') << setw(2) << timestamp / 1000
                      << "s: Process " << curr.name << " referenced page "
                      << i << ". Page in memory. No page evicted."
                      << endl;

            }else{
                if (memoryFreePage.size()!=0){
                    AssignPage(&job,nextReferenceNumber);
                }else{
                    RandomRepalce(&job,nextReferenceNumber);
                }
            }

            CurrentTime +=100;
            usleep(100);
        }


    }
}

int main() {
    initVariable();
    Run();
    return 0;
}