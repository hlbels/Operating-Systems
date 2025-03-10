#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <chrono>
#include <algorithm>

using namespace std;

// Process class representing a single process
class Process {
public:
    string user;
    int id, readyTime, serviceTime, remainingTime;
    bool finished = false;

    Process(string user, int id, int readyTime, int serviceTime)
        : user(user), id(id), readyTime(readyTime), serviceTime(serviceTime), remainingTime(serviceTime) {}
};

// User class representing a user who owns multiple processes
class User {
public:
    string name;
    vector<Process*> processes;

    User(string name) : name(name) {}

    void addProcess(Process* process) {
        processes.push_back(process);
    }
};

// Global Variables
vector<User> users;
queue<Process*> readyQueue;
mutex queueMutex;
condition_variable cv;
int timeQuantum;
bool allProcessesFinished = false;
ofstream outputFile("output.txt");

// Scheduler Function
void scheduler() {
    int currentTime = 1;
    while (!allProcessesFinished) {
        {
            lock_guard<mutex> lock(queueMutex);
            queue<Process*> newQueue;
            swap(readyQueue, newQueue);

            for (auto& user : users) {
                vector<Process*> readyProcesses;
                for (auto* process : user.processes) {
                    if (!process->finished && process->readyTime <= currentTime)
                        readyProcesses.push_back(process);
                }
                int userQuantum = readyProcesses.empty() ? 0 : timeQuantum / readyProcesses.size();
                for (auto* process : readyProcesses) {
                    readyQueue.push(process);
                }
            }
        }

        while (!readyQueue.empty()) {
            Process* process = readyQueue.front();
            thread processThread([process, currentTime]() {
                unique_lock<mutex> lock(queueMutex);
                cv.wait(lock, [&]() { return !readyQueue.empty() && readyQueue.front() == process; });

                int executionTime = min(timeQuantum, process->remainingTime);
                outputFile << "Time " << currentTime << ", User " << process->user 
                        << ", Process " << process->id << ", Started\n";
                outputFile.flush();

                this_thread::sleep_for(chrono::seconds(executionTime));

                process->remainingTime -= executionTime;

                if (process->remainingTime > 0) {
                    outputFile << "Time " << currentTime + executionTime 
                            << ", User " << process->user << ", Process " << process->id << ", Paused\n";
                    readyQueue.push(process);
                } else {
                    outputFile << "Time " << currentTime + executionTime 
                            << ", User " << process->user << ", Process " << process->id << ", Finished\n";
                    process->finished = true;
                }

                outputFile.flush();
                readyQueue.pop();
                cv.notify_all();
            });

            processThread.detach();
        }

        this_thread::sleep_for(chrono::seconds(1));
        currentTime++;

        allProcessesFinished = true;
        for (auto& user : users) {
            for (auto* process : user.processes) {
                if (!process->finished) {
                    allProcessesFinished = false;
                    break;
                }
            }
        }
    }
}