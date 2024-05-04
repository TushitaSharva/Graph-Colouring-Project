/*
    Conventions: Nodes from 0 to N
    MPI processes range from 0 to N
    0th process is the master node
    1st to Nth process are the process nodes which are to be coloured
    Colour of 0th node will be 0
    Colours available for processes are from 0 to N
*/

/*
    Algorithm:
    - Partitions are made number wise

    - First all the nodes are uncoloured
    - The leader of each partition will initiate the process among their paritions. They will initially assign themselves the colour of their partition
    - because this is a fully connected graph, initialising with same is meaning less
    - The nodes will recieve the message from the neighbours of their partition, they will colour themselves using that and send to the leader node
    - The leader when it recieves the colours that are all different, will send message to all it's leaders telling it's colours.
    - If two leaders have any set of colour the same, the lower pid will inform the higher pid the set of unavailable colours.
    - The leader will inform those processes which sent those colours that those colours are taken and they need to be changed.
    - Again this will repeat untill all leaders have exhaustive set.
*/

#include <iostream>
#include <set>
#include <queue>
#include <utility>
#include <random>
#include <string.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <map>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <time.h>
#include <chrono>
#include <math.h>
#include <fstream>
#include <atomic>
#include <mutex>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <mpi.h>
#include <algorithm>

#define MASTER 0 // For master node
#define COLOUR 1 // Process node to process node as well as master node
#define CHECK 2  // Master node to process nodes
#define ACK 3    // Process node to Master node
#define FINISH 4 // Master node to process node
#define CHANGE 5

using namespace std;

/* Global variables */
int n; // Number of nodes along with the master node
mutex file_lock;
mutex colour_lock;
int *colour;
int message_complexity;
int my_partition_number;
int num_partitions;
int my_range_start;
int my_range_end;
int my_colour;
set<int> leaders;

class Graph
{
public:
    int size;
    vector<vector<int>> adj;
    set<pair<int, int>> edges;

    void init(int size)
    {
        this->size = size;
        adj.resize(size);
    }
};

void print(string str, int *colour)
{
    file_lock.lock();
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    time_t current = time(0);
    struct tm *timeinfo = localtime(&current);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    std::string timeString(buffer);

    string filename = "proc_PART" + to_string(pid) + ".log";
    ofstream outfile(filename, ios::app);

    outfile << "[" << timeString << "]"
            << " Process " << pid << " " << str << " [" << colour[0];

    for (int i = 1; i < size; i++)
    {
        outfile << ", " << colour[i];
    }
    outfile << "]\n";

    outfile.close();
    file_lock.unlock();
}

void getGraph(Graph *graph)
{
    ifstream inputfile("inp-params.txt");
    inputfile >> n >> num_partitions;
    graph->init(n);
    graph->adj[0].push_back({0});

    string line;
    getline(inputfile, line);

    for (int i = 1; i < n; i++)
    {
        std::getline(inputfile, line);
        stringstream ss(line);

        int number;
        while (ss >> number)
        {
            graph->adj[i].push_back(number);
        }
    }

    for (int i = 1; i < n; i++)
    {
        int u = graph->adj[i][0];
        for (int j = 1; j < graph->adj[i].size(); j++)
        {
            graph->edges.insert(make_pair(u, graph->adj[i][j]));
        }
    }
}

bool check_graph_consistency(Graph *graph, int *colours)
{
    for(int i = my_range_start; i <= my_range_end; i++)
    {
        for(int j = my_range_start; j <= my_range_end; i++)
        {
            if(i != j && colours[i] == colours[j])
            {
                return false;
            }
        }
    }

    return true;
}

void process_func()
{
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    set<int> recieved;
    MPI_Status status;
    int *recv_msg = new int[n];
    MPI_Recv(recv_msg, 1, MPI_INT, MPI::ANY_SOURCE, MPI::ANY_TAG, MPI_COMM_WORLD, &status);
    bool should_change = false;

    int tag = status.MPI_TAG;
    int sender = status.MPI_SOURCE;

    if(recv_msg[sender] == colour[pid] && sender < pid)
    {
        should_change = true;
    }
}

void leader_func()
{
    map<int, int> recvd_colours;
}

int main(int argc, char *argv[])
{
    Graph *graph = new Graph;
    getGraph(graph);

    colour = (int *)malloc(sizeof(int) * (n)); // Colour of each node as far as this process knows

    // set all to 0
    for (int i = 0; i < n; i++)
    {
        colour[i] = 0;
    }

    // The array colour represents the knowledge of this process regarding the colour of nodes

    int pid, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    int my_partition = pid / num_partitions;
    int num_procs_per_partition = size / num_partitions;
    my_range_start = my_partition * num_procs_per_partition; // is also the leader
    
    if(my_partition == num_partitions)
    {
        my_range_end = size -1;
    }

    else
    {
        my_range_end = (my_partition + 1) * num_procs_per_partition - 1;
    }
    
    if(pid == my_range_start) // i.e., the leader of this process
    {
        colour[pid] = pid;
        
        vector<int> neighbours = graph->adj[pid];
        for(int i = my_range_start; i <= my_range_end; i++)
        {
            MPI_Send(colour, 1, MPI_INT, neighbours[i], COLOUR, MPI_COMM_WORLD);
        }

        leader_func();
    }

    else
    {
        process_func();
    }


    for (int i = 0; i < n; i++)
    {
        std::cout << colour[i] << " ";
    }
    std::cout << "\n";

    MPI_Finalize();

    ofstream complexity("complexity_partition.txt", ios::app);
    complexity << message_complexity << " ";
    complexity.close();

    return 0;
}