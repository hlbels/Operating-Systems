#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <condition_variable>

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
};
class DiskMemory
{
private:
   string FileName;

public:
    DiskMemory() {};
    void readPage(Page *page) {  }; //file operations
    void writePage(Page *page) { };
    void searchPage(Page *page) { };
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
            DiskMemory.writePage(page); // To be implemented
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
};

// Extension to MainMemory: allow access to internal pages
vector<Page*>& MainMemory::getPages() {
    return pages;
}

// DiskMemory: function to simulate reading from disk
void DiskMemory::readPageFromFile(const string& variableId) {
    ifstream file("disk_storage.txt");
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
