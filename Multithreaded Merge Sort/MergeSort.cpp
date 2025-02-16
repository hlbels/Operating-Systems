#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>

using namespace std;

mutex mtx;

vector<int> ReadFile(const std::string& InFileName) {   
    vector<int> numbers;
    ifstream inputFile(InFileName);
    
    if (!inputFile) {  // Check if the file was successfully opened
        cerr << "Error opening the file!" << std::endl;
        return {};  // Return an empty vector
    }

    int num;
    while (inputFile >> num) {  // Read integers from the file
        numbers.push_back(num); // Store each number in the vector
    }

    inputFile.close();
    return numbers;  // Return the vector
}

// Recursive function to divide the array using threads
void divide(vector<int>& numbers, int left, int right, vector<int>& arr) {
    
    if (left >= right) {
        lock_guard<mutex> lock(mtx); // Ensure thread safety when modifying arr (Not sure if needed)
        arr.push_back(numbers[left]);
        return;
    }

    int mid = left + (right - left) / 2;
    
    // Create threads to recursively divide left and right halves
    thread leftThread(divide, ref(numbers), left, mid, ref(arr));
    leftThread.join();
    thread rightThread(divide, ref(numbers), mid + 1, right, ref(arr));
    rightThread.join();
}



// Function to merge two sorted subarrays
void merge(vector<int>& arr, int left, int mid, int right) {
    vector<int> leftSub(arr.begin() + left, arr.begin() + mid + 1);
    vector<int> rightSub(arr.begin() + mid + 1, arr.begin() + right + 1);

    int i = 0, j = 0, k = left;

    while (i < leftSub.size() && j < rightSub.size()) {
        if (leftSub[i] < rightSub[j]) {
            arr[k++] = leftSub[i++];
        } else {
            arr[k++] = rightSub[j++];
        }
    }

    while (i < leftSub.size()) arr[k++] = leftSub[i++];
    while (j < rightSub.size()) arr[k++] = rightSub[j++];
}

// Recursive threaded merge sort function
void mergeSort(vector<int>& arr, int left, int right){
    if(left >= right) return;
    
    int mid = left + (right - left) / 2;
    
    thread leftThread(mergeSort, ref(arr), left, mid);
    thread rightThread(mergeSort, ref(arr), mid + 1, right);
    
    leftThread.join();
    rightThread.join();
    
    thread mergeThread(merge,ref(arr), left, mid, right);
    mergeThread.join();
    
}

// Function to write sorted array to the output file
void WriteOutputFile(const string& filename, const vector<int>& sortedNumbers) {
    ofstream outputFile(filename);
    if (!outputFile) {
        cerr << "Error opening the output file!" << endl;
        return;
    }

    outputFile << "Final Sorted Output: ";
    cout << "Final Sorted Output: ";
    for (size_t i = 0; i < sortedNumbers.size(); i++) {
        cout << sortedNumbers[i] << (i < sortedNumbers.size() - 1 ? ", " : "\n");
        outputFile << sortedNumbers[i] << (i < sortedNumbers.size() - 1 ? ", " : "\n");
    }

    outputFile.close();
}

int main()
{    
    
    vector<int> arr; //To store the base case values
    
    string filename = "Input.txt";  // Ensure the file exists in the same directory
    vector<int> numbers = ReadFile(filename);//Array of numbers from the file
    
    cout << "Array dividion starts:\n";
    thread divideThread(divide, ref(numbers), 0, numbers.size() - 1, ref(arr));
    divideThread.join(); 
    
    cout << "Array division completed.\n";
    
    cout << "Base case values stored in arr: ";
    for (int num : arr) cout << num << " ";
    cout << endl;
    
    // Apply multithreaded merge sort
    thread sortThread(mergeSort, ref(arr), 0, arr.size() - 1);
    sortThread.join();
    
    // Print sorted array
    cout << "Sorted arr: ";
    for (int num : arr) cout << num << " ";
    cout << endl;
    
    cout << "Sorting completed.";


    return 0;
}
