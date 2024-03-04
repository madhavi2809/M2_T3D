// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <algorithm>
// #include <condition_variable>

// // Define the structure for traffic signal data
// struct TrafficData {
//     int timestamp;
//     int traffic_light_id;
//     int num_cars_passed;
// };

// // Bounded buffer queue implementation
// template<typename T>
// class BoundedBuffer {
// private:
//     std::queue<T> buffer;
//     const unsigned int size;
//     std::mutex mtx;
//     std::condition_variable cv_full;
//     std::condition_variable cv_empty;

// public:
//     BoundedBuffer(unsigned int size) : size(size) {}

//     void produce(const T& data) {
//         std::unique_lock<std::mutex> lock(mtx);
//         cv_full.wait(lock, [this] { return buffer.size() < size; });
//         buffer.push(data);
//         cv_empty.notify_one();
//     }

//     T consume() {
//         std::unique_lock<std::mutex> lock(mtx);
//         cv_empty.wait(lock, [this] { return !buffer.empty(); });
//         T data = buffer.front();
//         buffer.pop();
//         cv_full.notify_one();
//         return data;
//     }
// };

// // Comparator function for sorting TrafficData based on num_cars_passed
// bool compareTrafficData(const TrafficData& data1, const TrafficData& data2) {
//     return data1.num_cars_passed > data2.num_cars_passed;
// }

// // Function executed by producer threads
// void produceData(BoundedBuffer<TrafficData>& buffer) {
//     // Simulate traffic data generation
//     // Here you would read data from the input file or generate it programmatically
//     TrafficData data = {0, 1, 10}; // Example traffic data
//     // Produce data to the buffer
//     buffer.produce(data);
//     // Simulate continuous production
//     std::this_thread::sleep_for(std::chrono::seconds(5));
// }

// // Function executed by consumer threads
// void consumeData(BoundedBuffer<TrafficData>& buffer, std::mutex& sortedListMutex, std::vector<TrafficData>& sortedList) {
//     // Consume data from the buffer
//     TrafficData data = buffer.consume();
//     // Update the sorted list
//     std::lock_guard<std::mutex> lock(sortedListMutex);
//     sortedList.push_back(data);
//     std::sort(sortedList.begin(), sortedList.end(), compareTrafficData);
// }

// int main() {
//     // Initialize bounded buffer
//     BoundedBuffer<TrafficData> buffer(10); // Example buffer size

//     // Initialize sorted list
//     std::mutex sortedListMutex;
//     std::vector<TrafficData> sortedList;

//     // Create producer threads
//     std::vector<std::thread> producerThreads;
//     for (int i = 0; i < 5; ++i) { // Example of 5 producer threads
//         producerThreads.emplace_back(produceData, std::ref(buffer));
//     }

//     // Create consumer threads
//     std::vector<std::thread> consumerThreads;
//     for (int i = 0; i < 3; ++i) { // Example of 3 consumer threads
//         consumerThreads.emplace_back(consumeData, std::ref(buffer), std::ref(sortedListMutex), std::ref(sortedList));
//     }

//     // Join producer threads
//     for (auto& thread : producerThreads) {
//         thread.join();
//     }

//     // Join consumer threads
//     for (auto& thread : consumerThreads) {
//         thread.join();
//     }

//     return 0;
// }

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <sstream>

// Define the structure for traffic signal data
struct TrafficData {
    int timestamp;
    int traffic_light_id;
    int num_cars_passed;
};

// Bounded buffer queue implementation
template<typename T>
class BoundedBuffer {
private:
    std::queue<T> buffer;
    const unsigned int size;
    std::mutex mtx;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;

public:
    BoundedBuffer(unsigned int size) : size(size) {}

    void produce(const T& data) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_full.wait(lock, [this] { return buffer.size() < size; });
        buffer.push(data);
        cv_empty.notify_one();
    }

    T consume() {
        std::unique_lock<std::mutex> lock(mtx);
        cv_empty.wait(lock, [this] { return !buffer.empty(); });
        T data = buffer.front();
        buffer.pop();
        cv_full.notify_one();
        return data;
    }
};

// Comparator function for sorting TrafficData based on num_cars_passed
bool compareTrafficData(const TrafficData& data1, const TrafficData& data2) {
    return data1.num_cars_passed > data2.num_cars_passed;
}

// Function executed by producer threads
void produceData(BoundedBuffer<TrafficData>& buffer, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int timestamp, light_id, num_cars;
        char comma;
        if (!(iss >> timestamp >> comma >> light_id >> comma >> num_cars)) {
            std::cerr << "Invalid input line: " << line << std::endl;
            continue;
        }
        TrafficData data = {timestamp, light_id, num_cars};
        buffer.produce(data);
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Simulate measurement every 5 minutes
    }
}

// Function executed by consumer threads
void consumeData(BoundedBuffer<TrafficData>& buffer, std::mutex& sortedListMutex, std::vector<TrafficData>& sortedList) {
    while (true) {
        TrafficData data = buffer.consume();
        {
            std::lock_guard<std::mutex> lock(sortedListMutex);
            sortedList.push_back(data);
            std::sort(sortedList.begin(), sortedList.end(), compareTrafficData);
        }
        std::cout << "Timestamp: " << data.timestamp << ", Light ID: " << data.traffic_light_id << ", Cars Passed: " << data.num_cars_passed << std::endl;
    }
}

int main() {
    // Initialize bounded buffer
    BoundedBuffer<TrafficData> buffer(10); // Example buffer size

    // Initialize sorted list
    std::mutex sortedListMutex;
    std::vector<TrafficData> sortedList;

    // Create producer thread
    std::thread producerThread(produceData, std::ref(buffer), "traffic_data.txt");

    // Create consumer threads
    std::vector<std::thread> consumerThreads;
    for (int i = 0; i < 3; ++i) { // Example of 3 consumer threads
        consumerThreads.emplace_back(consumeData, std::ref(buffer), std::ref(sortedListMutex), std::ref(sortedList));
    }

    // Join producer thread
    producerThread.join();

    // Join consumer threads
    for (auto& thread : consumerThreads) {
        thread.join();
    }

    return 0;
}
