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
#include <random>
#include <algorithm>

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
    int timeLeft;
    int commandIndex;
    bool started;
    bool finished;

    Process(int _id, int _start, int _dur)
        : id(_id), startTime(_start), duration(_dur), timeLeft(_dur),
          commandIndex(0), started(false), finished(false) {}
};

int cores;
int memoryPages;
vector<Variable> memory;
unordered_map<string, pair<int, long> > disk;
vector<Process> processes;
vector<string> commands;
ofstream out("output.txt");

mutex logMutex, memoryMutex, schedulerMutex;
condition_variable cv;
int activeProcessCount = 0;
bool simulationDone = false;
auto simulationStart = steady_clock::now();

long currentTimeMs() {
    return duration_cast<milliseconds>(steady_clock::now() - simulationStart).count();
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

int findLRUSlot() {
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
    ifstream fin("vm.txt");
    string id;
    int val;
    long last;
    while (fin >> id >> val >> last)
        disk[id] = make_pair(val, last);
}

void saveDisk() {
    ofstream fout("vm.txt");
    for (unordered_map<string, pair<int, long> >::iterator it = disk.begin(); it != disk.end(); ++it)
        fout << it->first << " " << it->second.first << " " << it->second.second << endl;
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
        int lru = findLRUSlot();
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

void processThread(Process &p) {
    default_random_engine rng(p.id);
    uniform_int_distribution<int> delay(10, 1000);

    unique_lock<mutex> lock(schedulerMutex);
    cv.wait(lock, [&] { return p.started; });
    lock.unlock();

    logEvent("Process " + to_string(p.id + 1) + ": Started.");
    auto startTime = currentTimeMs();

    while (currentTimeMs() - startTime < p.duration * 1000) {
        this_thread::sleep_for(milliseconds(delay(rng)));

        stringstream ss(commands[p.commandIndex]);
        string cmd;
        ss >> cmd;

        if (cmd == "Store") {
            string id;
            int val;
            ss >> id >> val;
            store(id, val);
            logEvent("Process " + to_string(p.id + 1) + ", Store: Variable " + id + ", Value: " + to_string(val));
        } else if (cmd == "Release") {
            string id;
            ss >> id;
            release(id);
            logEvent("Process " + to_string(p.id + 1) + ", Release: Variable " + id);
        } else if (cmd == "Lookup") {
            string id;
            ss >> id;
            int val = lookup(id);
            logEvent("Process " + to_string(p.id + 1) + ", Lookup: Variable " + id + ", Value: " + to_string(val));
        }

        p.commandIndex = (p.commandIndex + 1) % commands.size();
    }

    logEvent("Process " + to_string(p.id + 1) + ": Finished.");

    lock_guard<mutex> lg(schedulerMutex);
    p.finished = true;
    activeProcessCount--;
    cv.notify_all();
}

void schedulerThread() {
    while (!simulationDone) {
        this_thread::sleep_for(milliseconds(100));
        long ms = currentTimeMs();

        lock_guard<mutex> lock(schedulerMutex);
        for (int i = 0; i < processes.size(); ++i) {
            Process &p = processes[i];
            if (!p.started && ms >= p.startTime * 1000 && activeProcessCount < cores) {
                p.started = true;
                activeProcessCount++;
                cv.notify_all();
            }
        }

        simulationDone = all_of(processes.begin(), processes.end(), [](const Process &p) { return p.finished; });
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
        Process p(i, start, dur);
        processes.push_back(p);
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

    thread scheduler(schedulerThread);
    vector<thread> threads;
    for (int i = 0; i < processes.size(); ++i)
        threads.emplace_back(processThread, ref(processes[i]));

    for (int i = 0; i < threads.size(); ++i)
        threads[i].join();

    scheduler.join();
    out.close();
    return 0;
}
