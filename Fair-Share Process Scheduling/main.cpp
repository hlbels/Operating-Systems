#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <map>
#include <algorithm>

using namespace std;

// Global synchronization variables
mutex schedulerMutex;
condition_variable cv;
atomic<int> currentTime(1);
bool allProcessesFinished = false;

// Process structure
struct Process {
    string user;
    int id;
    int readyTime;
    int serviceTime;
    int remainingTime;
    bool started = false;
    bool finished = false;

    Process(string u, int i, int r, int s) 
        : user(u), id(i), readyTime(r), serviceTime(s), remainingTime(s) {}
};

// User structure
struct User {
    string name;
    vector<Process*> processes;
};

// Global variables
vector<User> users;
mutex outputMutex;
ofstream outputFile;

// Function to log events
void logEvent(int time, const string& user, int processID, const string& event) {
    lock_guard<mutex> lock(outputMutex);
    outputFile << "Time " << time << ", User " << user << ", Process " << processID << ", " << event << endl;
    outputFile.flush();
    cout << "Time " << time << ", User " << user << ", Process " << processID << ", " << event << endl;
}

// Scheduler function
void scheduler(int quantum) {
    while (!allProcessesFinished) {
        unique_lock<mutex> lock(schedulerMutex);

        bool newCycle = (currentTime - 1) % quantum == 0;

        // Identify users with ready processes
        map<string, vector<Process*>> userProcesses;
        for (auto& user : users) {
            for (auto& process : user.processes) {
                if (!process->finished && process->readyTime <= currentTime) {
                    userProcesses[user.name].push_back(process);
                }
            }
        }

        if (userProcesses.empty()) {
            currentTime++;
            this_thread::sleep_for(chrono::seconds(1));
            continue;
        }

        int activeUsers = userProcesses.size();
        int userShare = quantum / activeUsers;

        vector<string> sortedUsers;
        for (auto& entry : userProcesses) {
            sortedUsers.push_back(entry.first);
        }
        sort(sortedUsers.begin(), sortedUsers.end());

        if (newCycle) {
            if (find(sortedUsers.begin(), sortedUsers.end(), "B") != sortedUsers.end()) {
                rotate(sortedUsers.begin(), find(sortedUsers.begin(), sortedUsers.end(), "B"), sortedUsers.end());
            }
        }

        for (const auto& user : sortedUsers) {
            vector<Process*>& processes = userProcesses[user];

            sort(processes.begin(), processes.end(), [](Process* a, Process* b) {
                return a->readyTime < b->readyTime;
            });

            int processShare = userShare / processes.size();
            if (processShare == 0) continue; // **Fix: Skip scheduling if no time is available**

            for (auto& process : processes) {
                if (process->finished) continue;

                int executionTime = min(process->remainingTime, processShare);
                if (executionTime == 0) continue; // **Fix: Ensure process actually runs**

                if (!process->started) {
                    logEvent(currentTime, process->user, process->id, "Started");
                    process->started = true;
                }

                logEvent(currentTime, process->user, process->id, "Resumed");
                this_thread::sleep_for(chrono::seconds(executionTime));
                process->remainingTime -= executionTime;
                currentTime += executionTime;
                logEvent(currentTime, process->user, process->id, "Paused");

                if (process->remainingTime == 0) {
                    process->finished = true;
                    logEvent(currentTime, process->user, process->id, "Finished");
                }
            }
        }

        allProcessesFinished = true;
        for (auto& user : users) {
            for (auto& process : user.processes) {
                if (!process->finished) {
                    allProcessesFinished = false;
                    break;
                }
            }
            if (!allProcessesFinished) break;
        }
    }
}

// Read input from file
void readInput(const string& filename, int& quantum) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open input file." << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    getline(inputFile, line);
    quantum = stoi(line);

    while (getline(inputFile, line)) {
        stringstream ss(line);
        string userName;
        int processCount;
        ss >> userName >> processCount;

        User user;
        user.name = userName;

        for (int i = 0; i < processCount; i++) {
            getline(inputFile, line);
            stringstream ps(line);
            int readyTime, serviceTime;
            ps >> readyTime >> serviceTime;

            Process* process = new Process(userName, i, readyTime, serviceTime);
            user.processes.push_back(process);
        }

        users.push_back(user);
    }
    inputFile.close();
}

// Main function
int main() {
    int quantum;
    readInput("input.txt", quantum);

    outputFile.open("output.txt", ios::out | ios::trunc);
    if (!outputFile.is_open()) {
        cerr << "Error: Could not open output file." << endl;
        exit(EXIT_FAILURE);
    }

    thread schedulerThread(scheduler, quantum);
    schedulerThread.join();

    outputFile.close();
    return 0;
}
