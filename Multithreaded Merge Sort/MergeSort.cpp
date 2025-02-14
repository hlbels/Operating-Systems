#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>

using namespace std;

mutex mtx;

vector<int> ReadFile(const std::string& InFileName) {   
    vector<int> numbers;  // Use a vector instead of an array
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

int main()
{    
    string filename = "Input.txt";  // Ensure the file exists in the same directory
    vector<int> numbers = ReadFile(filename);//Array of numbers from the file
    cout << "Sorted array: ";
    for (int num : numbers)
        cout << num << " ";
    cout << endl;

    return 0;
}
