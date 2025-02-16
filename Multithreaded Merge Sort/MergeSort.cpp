#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>
using namespace std;

mutex mtx;

// Function to write output to a file
void WriteOutputFile(const string& filename, const vector<string>& logEntries, const vector<int>& sortedNumbers) {
    ofstream outputFile(filename);
    if (!outputFile) {
        cerr << "Error opening the output file!" << endl;
        return;
    }

    // Write thread execution logs
    for (const string& entry : logEntries) {
        outputFile << entry << endl;
    }

    // Write the final sorted output
    outputFile << "Final Sorted Output: ";
    cout << "Final Sorted Output: ";
    for (size_t i = 0; i < sortedNumbers.size(); i++) {
        cout << sortedNumbers[i] << (i < sortedNumbers.size() - 1 ? ", " : "\n");
        outputFile << sortedNumbers[i] << (i < sortedNumbers.size() - 1 ? ", " : "\n");
    }

    outputFile.close();
}

// Merges subarrays into a single sorted subarray
void merge(vector<int>& arr, int left, int mid, int right) {
    vector<int> leftSub(arr.begin() + left, arr.begin() + mid + 1);
    vector<int> rightSub(arr.begin() + mid + 1, arr.begin() + right + 1);

    int i = 0, j = 0, k = left;

    while (i < leftSub.size() && j < rightSub.size()) {
        if (leftSub[i] < rightSub[j]) {
            arr[k++] = leftSub[i++];
        }
        else {
            arr[k++] = rightSub[j++];
        }
    }

    while (i < leftSub.size()) arr[k++] = leftSub[i++];
    while (j < rightSub.size()) arr[k++] = rightSub[j++];
}

// Merge Sort function that recursively divides the array into two halves and merges them
void mergeSort(vector<int>& arr, int left, int right, int threadID, vector<string>& logEntries)
{
    if (left < right) {
        int mid = left + (right - left) / 2;

        // Log thread start message
        {
            lock_guard<mutex> lock(mtx);
            string logMsg = "Thread " + to_string(threadID) + " started";
            logEntries.push_back(logMsg);
            cout << logMsg << endl;
        }

        // Create child thread IDs to keep track of threads
        int leftThreadID = threadID * 10;
        int rightThreadID = threadID * 10 + 1;

        // Create threads for recursive sorting
        thread t1(mergeSort, ref(arr), left, mid, leftThreadID, ref(logEntries));
        thread t2(mergeSort, ref(arr), mid + 1, right, rightThreadID, ref(logEntries));

        // Join threads to ensure completion before merging
        t1.join();
        t2.join();

        // Merge the sorted halves
        merge(arr, left, mid, right);

        // Log thread finished message for sorted subarrays
        {
            lock_guard<mutex> lock(mtx);
            string logMsg = "Thread " + to_string(threadID) + " finished: ";
            for (int i = left; i <= right; i++)
                logMsg += to_string(arr[i]) + " ";
            logEntries.push_back(logMsg);
            cout << logMsg << endl;
        }
    }
    else {
        // Log single element when the array size is one
        {
            lock_guard<mutex> lock(mtx);
            string logMsg = "Thread " + to_string(threadID) + " finished: ";
            for (int i = left; i <= right; i++)
                logMsg += to_string(arr[i]) + " ";
            logEntries.push_back(logMsg);
            cout << logMsg << endl;
        }
    }
}

vector<int> ReadFile(const string& InFileName) {
    vector<int> numbers;  // Use a vector instead of an array
    ifstream inputFile(InFileName);

    if (!inputFile) {  // Check if the file was successfully opened
        cerr << "Error opening the file!" << endl;
        return {};  // Return an empty vector
    }

    int num;
    while (inputFile >> num) {  // Read integers from the file
        numbers.push_back(num); // Store each number in the vector
    }

    inputFile.close();
    return numbers;  // Return the vector
}

int main()
{
    string filename = "Input.txt";  // Ensure the file exists in the same directory
    vector<int> numbers = ReadFile(filename);  // Array of numbers from the file

    if (numbers.empty()) {
        cerr << "No numbers to sort!" << endl;
        return 1;
    }

    vector<string> logEntries;  // To store the log messages for file output

    int currentParentID = 1;  // Initial parent thread
    mergeSort(numbers, 0, numbers.size() - 1, currentParentID, logEntries);

    // Write the logs and sorted output to a file
    WriteOutputFile("Output.txt", logEntries, numbers);

    return 0;
}
