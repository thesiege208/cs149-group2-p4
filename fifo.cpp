#include <iostream>
#include <queue>
#include <list>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>

using namespace std;

struct ProcessPageReference {
    int processId;
    int vPageId;
    int pPageId;
};

struct Process {
    int processId;
    int arrivalTime;
    int serviceTime;
    int numOfPages; // How many pages this Process can have.
    queue<int> referencePages; // The sequence of pages it will refer to
};

// Count the number of page hit (in memory), and page miss/replace (not in memory)
int pageHit, pageMiss;

//compare arrival time
int comp (const Process & a, const Process & b) {
    return a.arrivalTime < b.arrivalTime;
}

// Locality of refernce
int localityOfReference(int pageSize, int initialPage){
    int deltaI, pages;
    //get a vector containing 2<= |delata i| <= (pageSize-1)
    vector<int> deltaI30Vector;
    int pagedelta = -pageSize; // (max delta if last page i reference the farest page) + 1
    for (int i = 0; i < (2*pageSize - 1); i++) {
        pagedelta += 1;
        if (pagedelta == -1) {
            continue;
        }
        else if (pagedelta == 0) {
            continue;
        }
        else if (pagedelta == 1) {
            continue;
        }
        else {
            deltaI30Vector.push_back(pagedelta);
        }
    }
    
    int r = rand() % 11;
    if ( r >= 0 && r < 7) {
        deltaI = rand() % 3 - 1;
        if ( (initialPage + deltaI) < 0 || (initialPage + deltaI) >=pageSize) {
            deltaI = -deltaI;
        }
        pages = initialPage + deltaI;
    } else {
        int deltaIIndex = rand() % (pageSize*2 - 4);
        deltaI = deltaI30Vector[deltaIIndex];
        if ( (initialPage + deltaI) < 0 ) {
            pages = 0;
        }
        else if ( (initialPage + deltaI) >= pageSize) {
            pages = pageSize -1;
        }
        else {
            pages = initialPage + deltaI;
        }
    }
    return pages;
}

// Given a integer n, generate n random processes
queue<Process> simulateIncomingProcess() {
    queue<Process> allProcess;
    vector<Process> processVector;
    for ( int i = 0; i < 150; i++) {
        Process randomprocess;
        randomprocess.arrivalTime = rand() % 60;
        randomprocess.serviceTime = rand() % 5 + 1;
        int randomSize = rand() % 4;
        if (randomSize == 0)    randomprocess.numOfPages = 5;
        if (randomSize == 1)    randomprocess.numOfPages = 11;
        if (randomSize == 2)    randomprocess.numOfPages = 17;
        if (randomSize == 3)    randomprocess.numOfPages = 31;
        
        int refPage = localityOfReference(randomprocess.numOfPages, 0);
        randomprocess.referencePages.push(0);
        for ( int i = 1; i < randomprocess.serviceTime * 10 ; i++) {
            
            refPage = localityOfReference(randomprocess.numOfPages, refPage);
            randomprocess.referencePages.push(refPage);
        }
        processVector.push_back(randomprocess);
    }
    
    //sort the process by arrival time
    sort(processVector.begin(), processVector.end(), comp);
    
    for (int k = 0; k < 150; k++ ) {
        if ( !processVector.empty() ) {
            processVector.front().processId = k + 1;
            cout << "process id is " << processVector.front().processId << "; arrival time is " << processVector.front().arrivalTime << "; service time is " << processVector.front().serviceTime << "; num of pages is " << processVector.front().referencePages.size() << endl;
            allProcess.push(processVector.front());
            processVector.erase(processVector.begin());
        } else break;
    }
    return allProcess;
}

// Print the physical memory.
void printPhysicalMemory(string physicalMem[100]) {
    for (int i = 0; i < 100; i++) {
        cout << physicalMem[i] << " ";
    }
    cout <<endl;
}

// Given the processId, remove all its pages in referredPages
void removeReferencedPagesOfEndProcess(int processId,
                                       list<ProcessPageReference>& referredPages,
                                       string physicalMem[100],
                                       vector<set<int>>& processInMemPages) {
    //cout << "we are going to remove process " << processId << "'s reference pages:" << endl;
    
    // // For testing
    // cout << "the referred pages" << endl;
    // for (list<ProcessPageReference>::iterator iter = referredPages.begin(); iter != referredPages.end();iter++) {
    //     cout << iter->processId << ", " << iter->vPageId << ", " << iter->pPageId << endl;
    // }
    
    processInMemPages[processId].clear();
    for (list<ProcessPageReference>::iterator iter = referredPages.begin(); iter != referredPages.end(); ) {
        if (iter->processId == processId) {
            physicalMem[iter->pPageId] = ".";
            iter = referredPages.erase(iter);
        } else {
            iter++;
        }
    }
}


// Go over the current processes list, if any process ended (arrivalTime + serviceTime <= currentTime)
// (1) Remove it from the currentProcesses,
// (2) and remove the corresponding ProcessPageReference from referredPages.
void removeEndedProcess(int currentTime,
                        list<Process>& currentProcesses,
                        list<ProcessPageReference>& referredPages,
                        string physicalMem[100],
                        vector<set<int>>& processInMemPages) {
    for (list<Process>::iterator processIter=currentProcesses.begin(); processIter!=currentProcesses.end();) {
        Process process = *processIter;
        if (process.arrivalTime + process.serviceTime <= currentTime) {
            // the process should end
            removeReferencedPagesOfEndProcess(process.processId, referredPages, physicalMem, processInMemPages);
            cout << "= Time Stamp 0:" << setw(2) << setfill('0') << currentTime <<" - Process Name: " << process.processId << " - Exit - Size in Pages: " << process.numOfPages << " - Service Duration: " << process.serviceTime << " = " << "\n";
            //printPhysicalMemory(physicalMem);
            cout << "\n";
            
            // Remove the process from current process.
            processIter = currentProcesses.erase(processIter);
        } else {
            processIter++;
        }
    }
}

// Check what's the next reference page of this process, update referredPages list and corresponding physical memory
void handleProcessNextReferencePage(list<Process>::iterator iter,
                                    list<ProcessPageReference>& referredPages,
                                    string physicalMem[100],
                                    vector<set<int>>& processInMemPages) {
    if (iter->referencePages.size() == 0) {
        // There is no more reference pages.
        cout << "No more reference page for process:" << iter->processId << endl;
        return;
    }
    
    int vPageId = iter->referencePages.front();
    // The page is in memory, page hit.
    if (processInMemPages[iter->processId].find(vPageId) != processInMemPages[iter->processId].end()) {
        iter->referencePages.pop();
        pageHit++;
        return;
    }
    
    //fifo replacement algorithm
    if (referredPages.size() == 100) {
        // the physical meomory is full, try to remove an old page.
        // To be 100% correct we might need to check whether this page is the process latest reference page.
        // If so we can't remove it. And need to keep searching.
        // Here for simplicity we assume it is the oldest one, and we just remove it.
        int oldPhysicalMemId = referredPages.front().pPageId;
        int oldProcess = referredPages.front().processId;
        int oldVirtualPageId = referredPages.front().vPageId;
        
        processInMemPages[oldProcess].erase(oldVirtualPageId);
        physicalMem[oldPhysicalMemId] = ".";
        referredPages.pop_front();
    }
    
    // The page is not in memory, repacement happens.
    pageMiss++;
    // Check which physical memory is available.
    for (int i = 0; i < 100; i++) {
        // Go over each page of physical memory and check whether one is empty
        if (physicalMem[i] == ".") {
            // page i is empty
            ProcessPageReference reference;
            reference.processId = iter->processId;
            reference.vPageId = vPageId;
            reference.pPageId = i;
            
            physicalMem[i] = to_string(iter->processId);
            referredPages.push_back(reference);
            
            processInMemPages[iter->processId].insert(vPageId);
            
            iter->referencePages.pop();
            break;
        }
    }
}

// Check any new process arrive, if so add them to currentProcesses
void checkAnyNewArrivalProcess(int currentTime,
                               queue<Process>& allProcess,
                               list<Process>& currentProcesses) {
    while (allProcess.empty() == false && allProcess.front().arrivalTime <= currentTime) {
        cout << "= Time Stamp 0:" << setw(2) << setfill('0') << currentTime <<" - Process Name: " << allProcess.front().processId << " - Enter - Size in Pages: " << allProcess.front().numOfPages << " - Service Duration: " << allProcess.front().serviceTime << " = " << "\n";
        currentProcesses.push_back(allProcess.front());
        allProcess.pop();
    }
}

void printRequiredRecords(int timeStamp, list<Process>& currentProcesses, list<ProcessPageReference>& referredPages) {
    cout << endl << "= Time Stamp 0:" << setw(2) << setfill('0')<< timeStamp << endl;
    if (referredPages.empty() == false) {
        cout << "  Process " << referredPages.front().processId << "'s virtual page "
        << referredPages.front().vPageId << " will be evicted if needed." << endl;
    }
    map<int, vector<ProcessPageReference>> processPageRefMap;
    for (list<Process>::iterator processIter=currentProcesses.begin(); processIter!=currentProcesses.end(); processIter++) {
        processPageRefMap.insert({processIter->processId, vector<ProcessPageReference>()});
    }
    
    for (list<ProcessPageReference>::iterator refIter=referredPages.begin();
         refIter!=referredPages.end();
         refIter++) {
        int proceeId = refIter->processId;
        processPageRefMap.at(proceeId).push_back(*refIter);
    }
    
    for (map<int, vector<ProcessPageReference>>::iterator mapIter=processPageRefMap.begin();
         mapIter!=processPageRefMap.end();
         mapIter++) {
        if (mapIter->second.size() > 0) {
            cout << "  Process " << mapIter->first << " is under service, its reference page (virtual_page:physical_page) is: ";
            for (vector<ProcessPageReference>::iterator vecIter = mapIter->second.begin();
                 vecIter != mapIter->second.end();
                 vecIter++) {
                cout<<"(" << vecIter->vPageId << ":" << vecIter->pPageId << "), ";
            }
            cout << endl;
        }
    }
}


// The main similation.
void simulation(queue<Process>& allProcess) {
    
    // Indicate the referred relationship.
    list<ProcessPageReference> referredPages;
    
    // the process we are handling (already came and not ended yet).
    list<Process> currentProcesses;
    
    //
    vector<set<int>> processInMemPages(151, set<int>());
    
    // Initialize the physical memory
    string physicalMem[100];
    for (int i = 0; i < 100; i++) {
        physicalMem[i] = ".";
    }
    
    cout << endl << "Start the simulation" << endl;
    for (int s = 0; s < 60; s++) { // simulate every second
        // Check any process has ended, is so remove them
        removeEndedProcess(s,
                           currentProcesses,
                           referredPages,
                           physicalMem,
                           processInMemPages);
        // Check any new arrival Process
        if (referredPages.size() <= 96) {
            checkAnyNewArrivalProcess(s, allProcess, currentProcesses);
        }
        
        // simulate each 100 milli sec for all the process in currentProcesses list
        for (int i = 0; i < 10; i++) {
            // Go over each process we are handling
            for (list<Process>::iterator processIter=currentProcesses.begin(); processIter!=currentProcesses.end(); processIter++) {
                if (processIter->referencePages.empty() == false) {
                    // Handle process next reference page.
                    handleProcessNextReferencePage(processIter,
                                                   referredPages,
                                                   physicalMem,
                                                   processInMemPages);
                }
            }
        }
        printRequiredRecords(s, currentProcesses, referredPages);
        cout << "\n" << "= Memory Map: =" << "\n";
        printPhysicalMemory(physicalMem);
        
        
    }
}

int main()
{
    pageHit = 0;
    pageMiss = 0;
    srand((unsigned)time(0));
    
    // simulate the random process
    queue<Process> allProcess = simulateIncomingProcess();
    
    simulation(allProcess);
    
    cout << endl << "page hit:" << pageHit << ", pge miss/relacement: " << pageMiss <<
    ", page hit ratio: " << pageHit * 1.0 / (pageHit + pageMiss) << endl;
    return 0;
}



