#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <set>
#include <algorithm>
#include <chrono>

using namespace std;

// Data structure to represent a row of traffic data
struct traffic_data
{
    int id;
    std::string times;
    int traffic_light_id;
    int numofcars;
};

int Hours = 12; // Duration after which to print summary

vector<int> in;
vector<int> lights;
vector<int> no_cars;
vector<string> timestamp;

// Array to hold totals of each of the 10 traffic lights
traffic_data sorting_array[10] = {{0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}, {0, "", 4, 0}, {0, "", 5, 0}, {0, "", 6, 0}, {0, "", 7, 0}, {0, "", 8, 0}, {0, "", 9, 0}, {0, "", 10, 0}};

// Comparison function used for sorting traffic data based on the number of cars
bool sort_method(struct traffic_data first, struct traffic_data second)
{
    if (first.numofcars > second.numofcars)
    {
        return true;
    }
    return false;
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
            string id, times, lightID, Cars_No;
            getline(infile, id, ',');
            in.push_back(stoi(id));
            getline(infile, times, ',');
            timestamp.push_back(times);
            getline(infile, lightID, ',');
            lights.push_back(stoi(lightID));
            getline(infile, Cars_No, '\n');
            no_cars.push_back(stoi(Cars_No));
        }
        infile.close();
    }
    else
    {
        cout << "Error: Unable to open file." << endl;
        return;
    }
}

int main()
{
    auto start = chrono::steady_clock::now();
    get_traff_data();

    // Get unique timestamps
    set<string> unique_timestamps(timestamp.begin(), timestamp.end());

    // Iterate over each unique timestamp
    for (const auto &ts : unique_timestamps)
    {
        // Filter traffic data for the current timestamp
        vector<traffic_data> filtered_data;
        for (size_t i = 0; i < timestamp.size(); ++i)
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
        cout << "Timestamp: " << ts << endl;
        cout << "Top 5 Entries:" << endl;
        for (size_t i = 0; i < min(filtered_data.size(), static_cast<size_t>(5)); ++i)
        {
            cout << "Traffic Light ID: " << filtered_data[i].traffic_light_id << " | Number of Cars: " << filtered_data[i].numofcars << endl;
        }
        cout << endl;
    }

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Total execution time: " << duration << " milliseconds" << endl;

    return 0;
}
