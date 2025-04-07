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
    Page(string variableId, unsigned int value, unsigned int LastAccessValue) : variableId(variableId), value(value), LastAccessValue(LastAccessValue) {}
};

class MainMemory
{
private:
    int size;
    bool isFull = false;
    vector<Page *> pages;

public:
    MainMemory(int size) : size(size) {}
    ~MainMemory()
    {
        for (auto page : pages)
        {
            delete page;
        }
    }
    void writePage(Page *page)
    {
        pages.push_back(page);
        if (pages.size() >= size)
            isFull = true;
    }

    void deletePage(string variableId)
    {
        for (auto it = pages.begin(); it != pages.end(); ++it)
        {
            if ((*it)->variableId == variableId)
            {
                pages.erase(it);
                isFull = false;
                break;
            }
        }
    }

    int readPage(string variableId)
    {
        for (auto &page : pages)
        {
            if (page->variableId == variableId)
            {
                return page->value;
            }
        }
        return -1;
    }
    Page *MainMemory::getPageById(string variableId)
    {
        // Iterate over the pages in memory to find the page with the given variableId
        for (auto &page : pages)
        {
            if (page->variableId == variableId)
            {
                return page; // Return the pointer to the page if found
            }
        }
        return nullptr; // Return nullptr if the page with the given variableId is not found
    }
    bool IsFull() const { return isFull; }

    vector<Page *> &getPages()
    {
        return pages;
    }
    string getLeastRecentlyUsed()
    { // Parsa
        unsigned int minTime = UINT_MAX;
        string lruVar;
        for (auto &page : pages)
        {
            if (page->LastAccessValue < minTime)
            {
                minTime = page->LastAccessValue;
                lruVar = page->variableId;
            }
        }
        return lruVar;
    }
};
class DiskMemory
{
public:
    DiskMemory() {};
    // DiskMemory: function to simulate reading from disk
    void writePage(Page *page)
    {                                      // Parsa
        ofstream file("vm.txt", ios::app); // 'app' = append mode

        if (!file.is_open())
        {
            cout << "Failed to open vm.txt for writing." << endl;
            return;
        }

        file << page->variableId << ":" << page->value << endl;
        file.close();

        cout << "Stored variable " << page->variableId
             << " with value " << page->value << " on disk." << endl;
    };
    Page *readPageFromFile(const string &variableId)
    {
        ifstream file("vm.txt");
        if (!file.is_open())
        {
            cout << "Disk file not found!" << endl;
            return nullptr;
        }

        string line;
        bool found = false;

        while (getline(file, line))
        {
            size_t pos = line.find(':');
            if (pos != string::npos)
            {
                string var = line.substr(0, pos);
                string val = line.substr(pos + 1);
                if (var == variableId)
                {
                    cout << "Loaded from disk: " << var << " = " << val << endl;
                    Page *page = new Page{var, static_cast<unsigned int>(stoi(val)), 0};
                    file.close();
                    return page;
                }
            }
        }
        if (!found)
        {
            cout << "Variable not found on disk." << endl;
        }

        file.close();
        return nullptr;
    }
    void deletePage(const string &variableId)
    {
        ifstream inFile("vm.txt");
        ofstream outFile("temp.txt");

        if (!inFile || !outFile)
        {
            cout << "Error opening file(s)." << endl;
            return;
        }

        string line;
        while (getline(inFile, line))
        {
            size_t pos = line.find(':');
            if (pos != string::npos)
            {
                string var = line.substr(0, pos);
                if (var == variableId)
                    continue; // Skip this line (delete it)
            }
            outFile << line << endl; // Write other lines
        }

        inFile.close();
        outFile.close();

        // Replace original file with updated one
        remove("vm.txt");
        rename("temp.txt", "vm.txt");
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
    bool IsReady(atomic<int> &currentTime) { return currentTime >= start; }
    bool HasFinished() { return BurstTime == 0; };
    Process(int id, int start, int BurstTime)
        : id(id), start(start), BurstTime(BurstTime) {}
};

// ===== Hala's Contribution =====

class VirtualMemoryManager
{
public:
    int current_time = 0;
    MainMemory MainMemory;
    DiskMemory DiskMemory;

    VirtualMemoryManager(int memSize, int current_time) : MainMemory(memSize), current_time(current_time) {}

    void Store(string variableId, unsigned int value)
    {
        Page *page = new Page{variableId, value, current_time};

        if (!MainMemory.IsFull())
        {
            MainMemory.writePage(page);
        }
        else
        {
            cout << "Main memory is full. Storing variable on disk." << endl;
            DiskMemory.writePage(page);
        }
    }

    int Lookup(string variableId)
    {
        current_time++;
        int value = MainMemory.readPage(variableId);
        if (value != -1)
        {
            // If found in main memory
            Page *page = MainMemory.getPageById(variableId); // Assuming you have a way to get the page object
            page->LastAccessValue = current_time;            // Update last access time
            cout << "Variable found in main memory: " << variableId << " = " << value << endl;
            return value;
        }

        // Not in memory, check in disk
        Page *diskPage = DiskMemory.readPageFromFile(variableId);
        if (diskPage != nullptr)
        {
            cout << "Page fault: " << variableId << " found on disk." << endl;

            // If memory is full, swap out LRU
            if (MainMemory.IsFull())
            {
                string lruVarId = MainMemory.getLeastRecentlyUsed();
                int lruValue = MainMemory.readPage(lruVarId);
                swap(lruVarId, lruValue);
            }
            Page *page = new Page{variableId, value, current_time};
            // Step 4: Load from disk into memory
            MainMemory.writePage(page);
            DiskMemory.deletePage(variableId);

            cout << "Swapped in " << variableId << " from disk to memory." << endl;
            return value;
        }

        // Step 5: Not found at all
        cout << "Variable " << variableId << " not found in memory or disk." << endl;
        return -1;
    }

    void AdvanceTime()
    {
        current_time++;
    }
    //=====Parsa====//
    void swap(string newVarId, unsigned int newVal)
    { // Parsa
        Page *lruPage = new Page{newVarId, newVal, current_time};
        // Move LRU page to disk
        DiskMemory.writePage(lruPage);
        MainMemory.deletePage(newVarId); // Assuming you have a deletePage function to remove the page from memory
        cout << "Swapped out LRU page " << newVarId << " to disk." << endl;
    }

    void Release(string variableId)
    {
        if (MainMemory.readPage(variableId) != -1)
        { // if variable is in main memory
            MainMemory.deletePage(variableId);
        }
        else if (DiskMemory.readPageFromFile(variableId) != nullptr)
        {
            DiskMemory.deletePage(variableId);
        }
        else
        {
            cout << "Variable not found in memory or disk." << endl;
        }
    } //====Parsa====//
};

// Parsa
int main()
{
    // Read memory config
    ifstream memFile("memconfig.txt");
    int memSize;
    memFile >> memSize;
    memFile.close();

    // Read process and core info
    ifstream procFile("processes.txt");
    int coreCount;
    int processCount;

    procFile >> coreCount >> processCount;

    vector<Process *> processes;
    for (int i = 0; i < processCount; ++i)
    {
        int start;
        int duration;
        procFile >> start >> duration;
        processes.push_back(new Process(i + 1, start, duration));
    }

    procFile.close();

    // Read commands
    ifstream cmdFile("commands.txt");
    vector<string> commands;
    string line;
    while (getline(cmdFile, line))
    {
        if (!line.empty())
        {
            commands.push_back(line);
        }
    }
    cmdFile.close();

    // Clock thread
    atomic<int> clock(0);
    VirtualMemoryManager vmm(memSize, clock);
    thread clockThread([&clock]()
                       {
        while(true) {
            this_thread::sleep_for(chrono::milliseconds(1000));
            ++clock;
        } });

    // Scheduler thread
    mutex coutMutex;
    atomic<int> activeCores(0);
    condition_variable cv;
    vector<thread> processThread;

    thread schedulerThread([&]()
                           {
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
                                vmm.Lookup(varId);
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
        } });

    schedulerThread.join();
    for (auto &t : processThread)
    {
        if (t.joinable())
            t.join();
    }

    clockThread.detach(); // Not sure if needed

    for (auto process : processes)
    {
        delete process;
    }

    return 0;
}