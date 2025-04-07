#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <sstream>

using namespace std;

struct Page
{
    string variableId;
    unsigned int value;
    unsigned int LastAccessValue;
};
class MainMemory
{
private:
    int size;
    bool isFull = false;
    vector<Page *> pages;

public:
    MainMemory(int size) : size(size) {};
    void readPage(Page *page) { pages.push_back(page); };
    void writePage(Page *page) { pages.push_back(page); };
    void searchPage(Page *page) { pages.push_back(page); };
    bool IsFull() { return isFull; };
    void setIsFull(bool val) { isFull = val; }
    vector<Page*>& getPages();//Parsa
    string getLeastRecentlyUsed() { //Parsa
        unsigned int minTime = UINT_MAX;
        string lruVar;
        for (auto& page : pages) {
            if (page->LastAccessValue < minTime) {
                minTime = page->LastAccessValue;
                lruVar = page->variableId;
            }
        }
        return lruVar;
    }
    
};
class DiskMemory
{
private:
   string FileName;

public:
    DiskMemory() {};
    void readPage(Page *page) {  }; //file operations
    void writePage(Page *page) { //Parsa
        ofstream file("vm.txt", ios::app); // 'app' = append mode

    if (!file.is_open()) {
        cout << "Failed to open vm.txt for writing." << endl;
        return;
    }

    file << page->variableId << ":" << page->value << endl;
    file.close();

    cout << "Stored variable " << page->variableId
         << " with value " << page->value << " on disk." << endl;
     };
    void searchPage(Page *page) { };
    void readPageFromFile(const string& variableId); //Parsa
    void printDiskContents() { //Parsa
        ifstream file("vm.txt");

    if (!file.is_open()) {
        cout << "Failed to open vm.txt." << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != string::npos) {
            string variableId = line.substr(0, pos);
            string value = line.substr(pos + 1);
            cout << "Variable: " << variableId << ", Value: " << value << endl;
        }
    }

    file.close();
    }
};
class VirtualMemoryManager
{
public:
    int current_time;
    MainMemory MainMemory;
    DiskMemory DiskMemory;
    // Implementation 3 main commands
    void Store(string variableId, unsigned int value)
    {
        Page page;
        page.variableId = variableId;
        page.value = value;
        page.LastAccessValue = current_time;
        if (!MainMemory.IsFull())
        {
            MainMemory.readPage(&page);
        }
        else
        {
            cout << "Memory is full , your variable will be stored on the disk";
            DiskMemory.readPage(&page);
        }
        
    }
};
class Process
{
public:
    int id;
    int start, BurstTime;
    bool isReady = false, isFinished = false;

public:
    void SetReady() { isReady = true; }
    void SetFinished() { isFinished = true; }
    bool IsReady(atomic<int> &currentTime) { return currentTime <= start; }
    bool HasFinished(atomic<int> &currentTime) { return BurstTime==0; };
    Process(int id, int start, int BurstTime)
        : id(id), start(start), BurstTime(BurstTime) {}

};

// ===== Hala's Contribution =====

class VirtualMemoryManager {
public:
    int current_time = 0;
    MainMemory MainMemory;
    DiskMemory DiskMemory;

    VirtualMemoryManager(int memSize) : MainMemory(memSize) {}

    void Store(string variableId, unsigned int value) {
        Page* page = new Page{variableId, value, current_time};
        
        if (!MainMemory.IsFull()) {
            MainMemory.readPage(page);
        } else {
            cout << "Main memory is full. Storing variable on disk." << endl;
            DiskMemory.writePage(page); 
        }
    }

    void Load(string variableId) {
        for (auto& page : MainMemory.getPages()) {
            if (page->variableId == variableId) {
                page->LastAccessValue = current_time;
                cout << "Loaded from main memory: " << variableId
                     << " = " << page->value << endl;
                return;
            }
        }
        cout << "Page not found in main memory. Trying to load from disk..." << endl;
        DiskMemory.readPageFromFile(variableId);
    }

    void PrintMemoryState() {
        cout << "---------- Main Memory ----------" << endl;
        for (auto& page : MainMemory.getPages()) {
            cout << "Variable: " << page->variableId
                 << ", Value: " << page->value
                 << ", Last Access Time: " << page->LastAccessValue << endl;
        }

        cout << "---------- Disk Memory ----------" << endl;
        DiskMemory.printDiskContents(); // To be implemented
    }

    void AdvanceTime() {
        current_time++;
    }
    //=====Parsa====//
    void swap(string newVarId, unsigned int newVal) { //Parsa
        string lruVarId = MainMemory.getLeastRecentlyUsed();
    
        // Remove LRU page from memory
        auto& pages = MainMemory.getPages();
        for (auto it = pages.begin(); it != pages.end(); ++it) {
            if ((*it)->variableId == lruVarId) {
                // Move it to disk
                DiskMemory.writePage(*it);
                pages.erase(it);
                break;
            }
        }
    
        // Add new page to memory
        Page* newPage = new Page{newVarId, newVal, current_time};
        MainMemory.readPage(newPage);
    
        cout << "Swapped out variable " << lruVarId
             << " and loaded variable " << newVarId << " into memory." << endl;
    }

    void Release(string variableId) { //Parsa
        auto& pages = MainMemory.getPages();
        for (auto it = pages.begin(); it != pages.end(); ++it) {
            if ((*it)->variableId == variableId) {
                delete *it;
                pages.erase(it);
                cout << "Released variable " << variableId << " from main memory." << endl;
                return;
            }
        }
    }//====Parsa====//
};

// Extension to MainMemory: allow access to internal pages
vector<Page*>& MainMemory::getPages() {
    return pages;
}

// DiskMemory: function to simulate reading from disk
void DiskMemory::readPageFromFile(const string& variableId) {
    ifstream file("vm.txt");
    if (!file.is_open()) {
        cout << "Disk file not found!" << endl;
        return;
    }

    string line;
    bool found = false;

    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != string::npos) {
            string var = line.substr(0, pos);
            string val = line.substr(pos + 1);
            if (var == variableId) {
                cout << "Loaded from disk: " << var << " = " << val << endl;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        cout << "Variable not found on disk." << endl;
    }

    file.close();
}

//Parsa
int main() {
    //Read memory config
    ifstream memFile("memconfig.txt");
    int memSize;
    memFile >> memSize;
    memFile.close();

    VirtualMemoryManager vmm(memSize);

    //Read process and core info
    ifstream procFile("processes.txt");
    int coreCount;
    int processCount;

    procFile >> coreCount >> processCount;

    vector<Process*> processes;
    for (int i = 0; i < processCount; ++i) {
        int start;
        int duration;
        procFile >> start >> duration;
        processes.push_back(new Process(i + 1, start, duration));
    }

    procFile.close();

    //Read commands
    ifstream cmdFile("commands.txt");
    vector<string> commands;
    string line;
    while (getline(cmdFile, line)) {
        if (!line.empty()) {
            commands.push_back(line);
        }
    }
    cmdFile.close();

    //Clock thread
    atomic<int> clock(0);
    thread clockThread([&clock]() {
        while(true) {
            this_thread::sleep_for(chrono::milliseconds(1000));
            ++clock;
        }
    });

    //Scheduler thread
    mutex coutMutex;
    atomic<int> activeCores(0);
    condition_variable cv;
    vector<thread> processThread;

    thread schedulerThread([&]() {
        size_t currentIndex = 0;
        while (currentIndex < processes.size()) {
            if (activeCores < coreCount) {
                Process* p = processes[currentIndex];
                if (clock >= p->start) {
                    activeCores++;
                    processThread.emplace_back([&, p]() {
                        {
                            lock_guard<mutex> lock(coutMutex);
                            cout << "Clock: " << clock << ", Process " << p->id << ": Started." << endl;
                        }

                        auto startTime = clock.load();
                        int commandIndex = 0;

                        while (clock - startTime < p->BurstTime) {
                            string cmd = commands[commandIndex];
                            istringstream iss(cmd);
                            string action, varId;
                            unsigned int val;

                            iss >> action >> varId;
                            
                            if (action == "Store") {
                                iss >> val;
                                vmm.Store(varId, val);
                                {
                                    lock_guard<mutex> lock(coutMutex);
                                    cout << "Clock: " << clock << ", Process " << p->id
                                         << ", Store: Variable " << varId << ", Value: " << val << endl;
                                }
                            } else if (action == "Lookup") {
                                vmm.Load(varId);
                                {
                                    lock_guard<mutex> lock(coutMutex);
                                    cout << "Clock: " << clock << ", Process " << p->id
                                         << ", Lookup: Variable " << varId << endl;
                                }
                            } else if (action == "Release") {
                                vmm.Release(varId);
                                {
                                    lock_guard<mutex> lock(coutMutex);
                                    cout << "Clock: " << clock << ", Process " << p->id
                                         << ", Release: Variable " << varId << endl;
                                }
                            }

                            commandIndex = (commandIndex + 1) % commands.size();
                            this_thread::sleep_for(chrono::milliseconds(rand() % 1000 + 1));
                        }

                        {
                            lock_guard<mutex> lock(coutMutex);
                            cout << "Clock: " << clock << ", Process " << p->id << ": Finished." << endl;
                        }

                        activeCores--;
                    });

                    currentIndex++;
                }
            }

            this_thread::sleep_for(chrono::microseconds(100));
        }
    });

    schedulerThread.join();
    for (auto& t : processThread) {
        if (t.joinable()) t.join();
    }

    clockThread.detach(); //Not sure if needed



    return 0;
}