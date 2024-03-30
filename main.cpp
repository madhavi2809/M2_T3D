#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <set>
#include <algorithm>
#include <chrono>
#include <unistd.h>

using namespace std;

mutex MUTEX; // Mutex for protecting shared resources

int PThreads = 4; // Number of producer threads
int CThreads = 4; // Number of consumer threads
int Hours = 12; // Duration after which to print summary
int ProducerCount = 0; // Counter for producer threads
int consumerCount = 0; // Counter for consumer threads
int m = 0; // Number of rows in traffic data

condition_variable producer_cv, consumer_cv;

string id, times, lightID, Cars_No;
vector<int> in;
vector<int> lights;
vector<int> no_cars;
vector<string> timestamp;

// Data structure to represent a row of traffic data
struct traffic_data
{
    int id;
    std::string times;
    int traffic_light_id;
    int numofcars;
};

// Array to hold totals of each of the 10 traffic lights
traffic_data sorting_array[10] = {{0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}, {0, "", 4, 0}, {0, "", 5, 0}, {0, "", 6, 0}, {0, "", 7, 0}, {0, "", 8, 0}, {0, "", 9, 0}, {0, "", 10, 0}};

queue<traffic_data> signal_queue; // Queue for storing traffic light data
traffic_data sig;                  // Current traffic data being processed

// Comparison function used for sorting traffic data based on the number of cars
bool sort_method(struct traffic_data first, struct traffic_data second)
{
    if (first.numofcars > second.numofcars)
    {
        return true;
    }
    return false;
}

// Producer thread function
void *producer(void *args)
{
    while (ProducerCount < m)
    {
        unique_lock<mutex> lock(MUTEX);

        if (ProducerCount < m)
        {
            // Push traffic data onto the queue
            signal_queue.push(traffic_data{in[ProducerCount], timestamp[ProducerCount], lights[ProducerCount], no_cars[ProducerCount]});
            consumer_cv.notify_all(); // Notify consumer threads that data is available
            ProducerCount++; 
        }

        else
        {
            // Wait if no data is available
            producer_cv.wait(lock, []
                             { return ProducerCount < m; });
        }

        lock.unlock();
        sleep(rand() % 5); // Wait if no data is available
    }
}

// Consumer thread function
void *consumer(void *args)
{
    while (consumerCount < m)
    {
        unique_lock<mutex> lock(MUTEX);

        if (!signal_queue.empty())
        {
            sig = signal_queue.front(); // Get data from the queue

            if (sig.traffic_light_id >= 1 && sig.traffic_light_id <= 10)
            {
                // Add the number of cars into the respective traffic light ID
                sorting_array[sig.traffic_light_id - 1].numofcars += sig.numofcars; // Using traffic_light_id as index, so adjust index to start from 0
            }

            signal_queue.pop(); // Remove processed data from the queue
            producer_cv.notify_all(); // Notify producer threads
            consumerCount++;
        }
        else
        {
            // Wait if no data is available
            consumer_cv.wait(lock, []
                             { return !signal_queue.empty(); });
        }

        if (consumerCount % (Hours * PThreads) == 0 && consumerCount != 0)
        {
            // Sort the entire array
            sort(sorting_array, sorting_array + 10, sort_method); // Sort entire array

            // Print top 5 entries
            cout << "Top 5 Entries:" << endl;
            for (int i = 0; i < min(5, 10); i++)
            {
                // Display traffic_light_id starting from 1
                cout << "Traffic Light ID: " << sorting_array[i].traffic_light_id << " | Number of Cars: " << sorting_array[i].numofcars << endl;
            }

            // Reset congestion data
            for (int i = 0; i < 10; i++)
            {
                sorting_array[i].numofcars = 0;
            }
        }

        lock.unlock();
        sleep(rand() % 5); // Simulate some processing time
    }
}

// Function to read traffic data from file
void get_traff_data()
{
    ifstream infile;

    infile.open("traffic_data.txt");

    if (infile.is_open())
    {
        std::string line;
        getline(infile, line);

        // Read data from file and populate vectors
        while (!infile.eof())
        {
            getline(infile, id, ',');
            in.push_back(stoi(id));
            getline(infile, times, ',');
            timestamp.push_back(times);
            getline(infile, lightID, ',');
            lights.push_back(stoi(lightID));
            getline(infile, Cars_No, '\n');
            no_cars.push_back(stoi(Cars_No));

            m += 1;
        }
        infile.close();
    }
    else
    {
        cout << "Error: Unable to open file." << endl;
        return;
    }

    // Get unique timestamps
    set<string> unique_timestamps(timestamp.begin(), timestamp.end());

    // Iterate over each unique timestamp
    for (const auto &ts : unique_timestamps)
    {
        // Filter traffic data for the current timestamp
        vector<traffic_data> filtered_data;
        for (size_t i = 0; i <timestamp.size(); ++i)
        {
            if (timestamp[i] == ts)
            {
                filtered_data.push_back({in[i], timestamp[i], lights[i], no_cars[i]});
            }
        }

        // Sort filtered data based on the number of cars
        sort(filtered_data.begin(), filtered_data.end(), [](const traffic_data &a, const traffic_data &b) {
            return a.numofcars > b.numofcars;
        });

        // Print top 5 entries for the current timestamp
        cout<<"Timestamp: "<< ts<<endl;
        cout<<"Top 5 Entries:"<<endl;
        for (size_t i = 0; i<min(filtered_data.size(), static_cast<size_t>(5)); ++i)
        {
            cout << "Traffic Light ID: "<< filtered_data[i].traffic_light_id <<" | Number of Cars: " << filtered_data[i].numofcars << endl;
        }
        cout << endl;
    }
}

int main()
{
    auto start = chrono::steady_clock::now();
    get_traff_data();

    pthread_t producers[PThreads];
    pthread_t consumers[CThreads];

    // Create producer threads
    for (long i = 0; i < PThreads; i++)
        pthread_create(&producers[i], NULL, producer, (void *)i);
    
    // Create consumer threads
    for (long i = 0; i < CThreads; i++)
        pthread_create(&consumers[i], NULL, consumer, NULL);

    // Join producer threads
    for (long i = 0; i < PThreads; i++)
        pthread_join(producers[i], NULL);

    // Join consumer threads
    for (long i = 0; i < CThreads; i++)
        pthread_join(consumers[i], NULL);

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Total execution time including threading: " << duration << " milliseconds" << endl;

    return 0;
}
