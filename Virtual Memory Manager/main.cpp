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
