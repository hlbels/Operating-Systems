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
class Process
{
public:
    string user;
    int id, readyTime, serviceTime, remainingTime;
    bool finished = false;

    // Constructor to initialize process attributes
    Process(string user, int id, int readyTime, int serviceTime)
        : user(user), id(id), readyTime(readyTime), serviceTime(serviceTime), remainingTime(serviceTime) {}
};

// User class representing a user who owns multiple processes
class User
{
public:
    string name;                 // Name of the user
    vector<Process *> processes; // List of processes owned by the user
    int No_Processes;

    // Constructor to initialize the user's name
    User(string name, int No_Processes) : name(name), No_Processes(No_Processes) {}

    // Adds a process to the user's process list
    void addProcess(Process *process)
    {
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
void executeProcess(Process* process, int& currentTime) {
    lock_guard<mutex> lock(queueMutex); // Ensure thread safety

    // Determine execution time: full quantum time or remaining time
    int execTime = min(timeQuantum, process->remainingTime);
    bool isFirstExecution = (process->remainingTime == process->serviceTime); // If first execution

    // Start or Resume
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

    
    this_thread::sleep_for(chrono::seconds(execTime));

    
    process->remainingTime -= execTime;
    currentTime += execTime; 

    if (process->remainingTime > 0) {
        // Process is paused, but not finished
        cout << "Time " << currentTime << ", User " << process->user 
             << ", Process " << process->id << ", Paused\n";
        outputFile << "Time " << currentTime << ", User " << process->user 
                   << ", Process " << process->id << ", Paused\n";
    } else {
        // Process has finished execution
        process->finished = true;
        cout << "Time " << currentTime << ", User " << process->user 
             << ", Process " << process->id << ", Finished\n";
        outputFile << "Time " << currentTime << ", User " << process->user 
                   << ", Process " << process->id << ", Finished\n";
    }
}


// Scheduler Function - Manages process execution
void scheduler()
{
    int currentTime = 1; // Keeps track of the current simulation time
    while (!allProcessesFinished)
    {
        {
            lock_guard<mutex> lock(queueMutex); // Lock to ensure safe access to the queue
            queue<Process *> newQueue;
            swap(readyQueue, newQueue); // Clear readyQueue by swapping with an empty queue

            // Iterate over all users to find processes that are ready to execute
            for (auto &user : users)
            {
                vector<Process *> readyProcesses; // Store processes that are ready at currentTime
                for (auto *process : user.processes)
                {
                    if (!process->finished && process->readyTime <= currentTime)
                        readyProcesses.push_back(process);
                }

                // Distribute time quantum among the user's ready processes
                int userQuantum = readyProcesses.empty() ? 0 : timeQuantum / readyProcesses.size();

                // Add ready processes to the scheduling queue
                for (auto *process : readyProcesses)
                {
                    readyQueue.push(process);
                }
            }
        }

        // Execute all processes in the ready queue
        while (!readyQueue.empty())
        {
            Process *process = readyQueue.front();                      // Get the process at the front of the queue
            thread processThread(executeProcess, process, currentTime); // Execute
            processThread.detach();                                     // Detach thread to run independently
        }

        this_thread::sleep_for(chrono::seconds(1)); // Simulate time progression in the scheduler
        currentTime++;

        // Check if all processes have finished execution
        allProcessesFinished = true;
        for (auto &user : users)
        {
            for (auto *process : user.processes)
            {
                if (!process->finished)
                {
                    allProcessesFinished = false;
                    break;
                }
            }
        }
    }
}
void ReadInput(string filename)
{
    ifstream inputFile(filename);

    if (!inputFile.is_open())
    {
        cerr << "Error opening input file." << endl;
        return;
    }

    // Check if the file is empty
    if (inputFile.peek() == ifstream::traits_type::eof())
    {
        cout << "File is empty" << endl;
        return;
    }

    // Read time quantum as an integer (first line)
    string line;
    if (getline(inputFile, line))
    {
        istringstream iss(line);
        if (!(iss >> timeQuantum))
        {
            cerr << "Invalid time quantum format." << endl;
            return;
        }
    }

    // Read users and their processes
    while (getline(inputFile, line))
    {
        istringstream iss(line);
        string username;
        int numberOfProcesses;

        if (!(iss >> username >> numberOfProcesses))
            continue; // Read username and number of processes

        users.push_back(User(username, numberOfProcesses)); // Add user to list
        // Read processes for this user
        for (int id = 0; id < numberOfProcesses; id++)
        {

            if (!getline(inputFile, line))
                break; // Read process details

            istringstream iss2(line);
            int readyTime, serviceTime;
            if (iss2 >> readyTime >> serviceTime)
            {
                Process *process = new Process(username, id, readyTime, serviceTime);
                users.back().addProcess(process);
            }
        }
    }

    inputFile.close();
}

int main()
{
    ReadInput("Input.txt");
    cout << timeQuantum << endl;

    for (const User &user : users)
    { // Loop through each user
        cout << "User: " << user.name << "\n";

        for (const Process *process : user.processes)
        { // Loop through each process of the user
            cout << "  Process ID: " << process->id
                 << ", Ready Time: " << process->readyTime
                 << ", Service Time: " << process->serviceTime
                 << ", Remaining Time: " << process->remainingTime
                 << "\n";
        }
    }

    return 0;

    //    scheduler();
}