#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

using namespace std;
using namespace chrono;

struct Variable {
    string id;
    int value;
    long lastAccess;
    bool occupied;
};

struct Process {
    int id;
    int startTime;
    int duration;
    bool started = false;
    bool finished = false;
    Process(int _id, int _start, int _dur)
        : id(_id), startTime(_start), duration(_dur) {}
};

int cores;
int memoryPages;
vector<Variable> memory;
unordered_map<string, pair<int, long>> disk;
vector<Process> processes;
vector<string> commands;
int globalCommandIndex = 0;

ofstream out("output.txt");
mutex logMutex, memoryMutex, commandMutex, schedulerMutex;
atomic<long> logicalClock(0); // global clock ticking every 10ms

int activeProcesses = 0;

long currentTimeMs() {
    return logicalClock.load();
}

void logEvent(const string &msg) {
    lock_guard<mutex> lock(logMutex);
    out << "Clock: " << currentTimeMs() << ", " << msg << endl;
}

int findInMemory(const string &id) {
    for (int i = 0; i < memory.size(); ++i)
        if (memory[i].occupied && memory[i].id == id)
            return i;
    return -1;
}

int findFreeSlot() {
    for (int i = 0; i < memory.size(); ++i)
        if (!memory[i].occupied)
            return i;
    return -1;
}

int findLRU() {
    long minAccess = LONG_MAX;
    int idx = -1;
    for (int i = 0; i < memory.size(); ++i)
        if (memory[i].occupied && memory[i].lastAccess < minAccess) {
            minAccess = memory[i].lastAccess;
            idx = i;
        }
    return idx;
}

void loadDisk() {
    disk.clear();
    ifstream f("vm.txt");
    string id;
    int val;
    long last;
    while (f >> id >> val >> last)
        disk[id] = make_pair(val, last);
}

void saveDisk() {
    ofstream f("vm.txt");
    for (auto &d : disk)
        f << d.first << " " << d.second.first << " " << d.second.second << endl;
}

void store(const string &id, int val) {
    lock_guard<mutex> lock(memoryMutex);
    int pos = findInMemory(id);
    if (pos != -1) {
        memory[pos].value = val;
        memory[pos].lastAccess = currentTimeMs();
        return;
    }
    int free = findFreeSlot();
    if (free != -1) {
        memory[free] = {id, val, currentTimeMs(), true};
    } else {
        disk[id] = make_pair(val, currentTimeMs());
        saveDisk();
    }
}

void release(const string &id) {
    lock_guard<mutex> lock(memoryMutex);
    int pos = findInMemory(id);
    if (pos != -1) {
        memory[pos].occupied = false;
    } else {
        disk.erase(id);
        saveDisk();
    }
}

int lookup(const string &id) {
    lock_guard<mutex> lock(memoryMutex);
    int pos = findInMemory(id);
    if (pos != -1) {
        memory[pos].lastAccess = currentTimeMs();
        return memory[pos].value;
    }

    loadDisk();
    if (disk.count(id)) {
        int lru = findLRU();
        if (lru != -1) {
            string replaced = memory[lru].id;
            disk[replaced] = make_pair(memory[lru].value, memory[lru].lastAccess);
            memory[lru] = {id, disk[id].first, currentTimeMs(), true};
            logEvent("Memory Manager, SWAP: Variable " + id + " with Variable " + replaced);
            disk.erase(id);
            saveDisk();
            return memory[lru].value;
        }
    }

    return -1;
}

void executeCommand(int pid) {
    string line;
    {
        lock_guard<mutex> lock(commandMutex);
        line = commands[globalCommandIndex];
        globalCommandIndex = (globalCommandIndex + 1) % commands.size();
    }

    stringstream ss(line);
    string cmd;
    ss >> cmd;

    if (cmd == "Store") {
        string id;
        int val;
        ss >> id >> val;
        store(id, val);
        logEvent("Process " + to_string(pid + 1) + ", Store: Variable " + id + ", Value: " + to_string(val));
    } else if (cmd == "Release") {
        string id;
        ss >> id;
        release(id);
        logEvent("Process " + to_string(pid + 1) + ", Release: Variable " + id);
    } else if (cmd == "Lookup") {
        string id;
        ss >> id;
        int val = lookup(id);
        logEvent("Process " + to_string(pid + 1) + ", Lookup: Variable " + id + ", Value: " + to_string(val));
    }
}

void processThread(Process &p) {
    while (currentTimeMs() < p.startTime * 1000)
        this_thread::sleep_for(milliseconds(10));

    {
        lock_guard<mutex> lock(schedulerMutex);
        if (!p.started && activeProcesses < cores) {
            p.started = true;
            activeProcesses++;
            logEvent("Process " + to_string(p.id + 1) + ": Started.");
        }
    }

    for (int i = 0; i < p.duration; ++i) {
        while (currentTimeMs() % 1000 != 0)
            this_thread::sleep_for(milliseconds(1));

        this_thread::sleep_for(milliseconds(p.id * 10));  // stagger commands
        executeCommand(p.id);
    }

    this_thread::sleep_for(milliseconds(p.id * 10));
    logEvent("Process " + to_string(p.id + 1) + ": Finished.");

    lock_guard<mutex> lock(schedulerMutex);
    p.finished = true;
    activeProcesses--;
}

void clockThread() {
    while (true) {
        this_thread::sleep_for(milliseconds(10));
        logicalClock += 10;
    }
}

void loadInputs() {
    ifstream mem("memconfig.txt");
    mem >> memoryPages;
    memory.resize(memoryPages);

    ifstream proc("processes.txt");
    int n;
    proc >> cores >> n;
    for (int i = 0; i < n; ++i) {
        int start, dur;
        proc >> start >> dur;
        processes.emplace_back(i, start, dur);
    }

    ifstream cmd("commands.txt");
    string line;
    while (getline(cmd, line))
        if (!line.empty()) commands.push_back(line);

    ofstream clear("vm.txt");
    clear.close();
}

int main() {
    loadInputs();

    thread clk(clockThread);
    clk.detach(); // run in background forever

    vector<thread> threads;
    for (int i = 0; i < processes.size(); ++i)
        threads.emplace_back(processThread, ref(processes[i]));

    for (auto &t : threads)
        t.join();

    out.close();
    return 0;
}
