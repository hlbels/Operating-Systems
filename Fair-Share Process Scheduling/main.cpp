#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>

using namespace std;

// Process class representing a single process
class Process {
public:
    string user;
    int id, readyTime, serviceTime, remainingTime;
    bool finished = false;

    // Constructor to initialize process attributes
    Process(string user, int id, int readyTime, int serviceTime)
        : user(user), id(id), readyTime(readyTime), serviceTime(serviceTime), remainingTime(serviceTime) {}
};

// User class representing a user who owns multiple processes
class User {
public:
    string name;
    vector<Process *> processes;
    
    User(string name) : name(name) {}

    void addProcess(Process *process) {
        processes.push_back(process);
    }
};

// Global Variables
vector<User> users;
queue<Process *> readyQueue;
mutex queueMutex;
condition_variable cv;
int timeQuantum;
bool allProcessesFinished = false;
ofstream outputFile("output.txt");

// Function to execute a process
void executeProcess(Process* process, int currentTime) {
    {
        unique_lock<mutex> lock(queueMutex);
        cv.wait(lock, [&]() { return !readyQueue.empty() && readyQueue.front() == process; });

        int execTime = min(timeQuantum, process->remainingTime);
        bool isFirstExecution = (process->remainingTime == process->serviceTime);

        if (isFirstExecution) {
            cout << "Time " << currentTime << ", User " << process->user 
                 << ", Process " << process->id << ", Started\n";
            outputFile << "Time " << currentTime << ", User " << process->user 
                       << ", Process " << process->id << ", Started\n";
        }

        cout << "Time " << currentTime << ", User " << process->user 
             << ", Process " << process->id << ", Resumed\n";
        outputFile << "Time " << currentTime << ", User " << process->user 
                   << ", Process " << process->id << ", Resumed\n";

        lock.unlock(); // Unlock mutex before executing

        this_thread::sleep_for(chrono::seconds(execTime));
        
        lock.lock(); // Re-lock mutex before modifying shared state
        process->remainingTime -= execTime;

        if (process->remainingTime > 0) {
            cout << "Time " << currentTime + execTime << ", User " << process->user 
                 << ", Process " << process->id << ", Paused\n";
            outputFile << "Time " << currentTime + execTime << ", User " << process->user 
                       << ", Process " << process->id << ", Paused\n";
            readyQueue.push(process);
        } else {
            process->finished = true;
            cout << "Time " << currentTime + execTime << ", User " << process->user 
                 << ", Process " << process->id << ", Finished\n";
            outputFile << "Time " << currentTime + execTime << ", User " << process->user 
                       << ", Process " << process->id << ", Finished\n";
        }

        readyQueue.pop();
        cv.notify_all();
    }
}

// Scheduler function
void scheduler() {
    int currentTime = 1;
    while (!allProcessesFinished) {
        {
            lock_guard<mutex> lock(queueMutex);
            queue<Process *> newQueue;
            swap(readyQueue, newQueue);

            for (auto &user : users) {
                vector<Process *> readyProcesses;
                for (auto *process : user.processes) {
                    if (!process->finished && process->readyTime <= currentTime)
                        readyProcesses.push_back(process);
                }

                for (auto *process : readyProcesses) {
                    readyQueue.push(process);
                }
            }
        }

        while (!readyQueue.empty()) {
            Process *process = readyQueue.front();
            thread processThread(executeProcess, process, currentTime);
            processThread.join();  // Ensure sequential execution
        }

        this_thread::sleep_for(chrono::seconds(1));
        currentTime++;

        allProcessesFinished = true;
        for (auto &user : users) {
            for (auto *process : user.processes) {
                if (!process->finished) {
                    allProcessesFinished = false;
                    break;
                }
            }
        }
    }
}

// Function to read input file
void ReadInput(string filename) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Error opening input file." << endl;
        return;
    }

    if (inputFile.peek() == ifstream::traits_type::eof()) {
        cout << "File is empty" << endl;
        return;
    }

    string line;
    if (getline(inputFile, line)) {
        istringstream iss(line);
        if (!(iss >> timeQuantum)) {
            cerr << "Invalid time quantum format." << endl;
            return;
        }
    }

    while (getline(inputFile, line)) {
        istringstream iss(line);
        string username;
        int numberOfProcesses;
        if (!(iss >> username >> numberOfProcesses)) continue;

        User user(username);
        for (int id = 0; id < numberOfProcesses; id++) {
            if (!getline(inputFile, line)) break;

            istringstream iss2(line);
            int readyTime, serviceTime;
            if (iss2 >> readyTime >> serviceTime) {
                Process *process = new Process(username, id, readyTime, serviceTime);
                user.addProcess(process);
            }
        }
        users.push_back(user);
    }

    inputFile.close();
}

// Main function
int main() {
    ReadInput("input.txt");

    cout << "Time Quantum: " << timeQuantum << endl;
    for (const User &user : users) {
        cout << "User: " << user.name << "\n";
        for (const Process *process : user.processes) {
            cout << "  Process ID: " << process->id
                 << ", Ready Time: " << process->readyTime
                 << ", Service Time: " << process->serviceTime
                 << ", Remaining Time: " << process->remainingTime
                 << "\n";
        }
    }

    thread schedulerThread(scheduler);
    schedulerThread.join();

    outputFile.close();
    return 0;
}
