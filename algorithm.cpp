/*
    Conventions: Nodes from 1 to N
    MPI processes range from 0 to N
    0th process is the master node
    1st to Nth process are the process nodes which are to be coloured
    Colour of 0th node will be 0
    Colours available for processes are from 1 to N
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

using namespace std;

/* Global variables */
int n;       // Number of nodes along with the master node
int *colour; // Colour of each node as far as this process knows
mutex file_lock;
mutex colour_lock;

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

void print(string str)
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

    string filename = "proc_MK" + to_string(pid) + ".log";
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
    inputfile >> n;
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
    for (auto i : graph->edges)
    {
        if (colours[i.first] == colours[i.second])
        {
            return false;
        }
    }

    return true;
}

void master_func()
{
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int coloured_nodes = 0;

    while (true)
    {
        MPI_Status status;
        int *recv_msg = (int *)malloc(sizeof(int) * n);
        MPI_Recv(recv_msg, n, MPI_INT, MPI::ANY_SOURCE, COLOUR, MPI_COMM_WORLD, &status);
        print("recieved COLOUR message from " + to_string(status.MPI_SOURCE));

        coloured_nodes++;
        if (coloured_nodes == n - 1) // from 1 to total size
        {
            break;
        }
    }

    print("recieved all the COLOUR messages, sending CHECK");

    // It will send CHECK message to nodes saying, everyone has been coloured atleast once, and now you can check the consistency
    for (int i = 1; i < n; i++)
    {
        MPI_Send(colour, n, MPI_INT, i, CHECK, MPI_COMM_WORLD);
    }

    print("sent CHECK");

    // Now it will wait for someone to send ACK that the consistent colourign has been observed.
    int *recv_msg2 = (int *)malloc(sizeof(int) * size - 1);
    MPI_Status status;
    MPI_Recv(recv_msg2, n, MPI_INT, MPI::ANY_SOURCE, ACK, MPI_COMM_WORLD, &status);

    print("recieved ACK, sending FINISH");

    // Given it is done, it will send to all the nodes FINISH and also the set of colours decided.
    for (int i = 1; i < n; i++)
    {
        MPI_Send(recv_msg2, n, MPI_INT, i, FINISH, MPI_COMM_WORLD);
    }

    print("Returning");
    return;
}

void process_func(Graph *graph, int pid)
{
    bool check = false;
    set<int> available;
    for (int i = 1; i < n; i++)
    {
        available.insert(i);
    }

    set<int> unavailable;

    while (true)
    {
        int *recv_msg = (int *)malloc(sizeof(int) * n);
        MPI_Status status;
        MPI_Recv(recv_msg, n, MPI_INT, MPI::ANY_SOURCE, MPI::ANY_TAG, MPI_COMM_WORLD, &status);

        int tag = status.MPI_TAG;
        int sender = status.MPI_SOURCE;

        if (tag == FINISH)
        {
            print("recieved FINISH message");
            for (int i = 0; i < n; i++)
            {
                colour[i] = recv_msg[i];
            }

            print("finishing with colours:");
            break;
        }

        else if (tag == CHECK)
        {
            print("recieved check message");
            check = true;
        }

        else if (tag == COLOUR)
        {
            // From the message recieved, check all the colours of this neighbour
            int myColour = colour[pid];
            for (int i = 1; i < graph->adj[pid].size(); i++)
            {
                // I will change the colour only if my colour is in the unavailable, and I want to change it only if my pid is higher.
                // So whenever equal, if the neighbour's pid is higher, only then I will add to unavailable.
                if (myColour == colour[i])
                {
                    if (i > pid)
                    {
                        unavailable.insert(colour[i]);
                        available.erase(colour[i]);
                    }
                }

                else
                {
                    available.erase(colour[i]);
                    unavailable.insert(colour[i]);
                }

                colour[i] = recv_msg[i];
            }

            // We only change the colour if it is necessary, because we might have sent the colour to neighbours, forcing to colour more
            if (myColour == 0) // If it was uncoloured till now
            {
                colour[pid] = *available.begin();
                print("Coloured just now, sending COLOUR to 0, my new colour is " + to_string(colour[pid]));
                MPI_Send(colour, n, MPI_INT, 0, COLOUR, MPI_COMM_WORLD);
            }

            else if (unavailable.find(myColour) != unavailable.end())
            {
                colour[pid] = *available.begin();
            }

            // If necessary colour wouldve been changed, else remain the same.
            // Updating colours for all except the neighbours : This part should not affect the current colour these are not direct neighbours
            for (int i = 1; i < n; i++)
            {
                if (find(graph->adj[pid].begin(), graph->adj[pid].end(), i) != graph->adj[pid].end())
                {
                    colour[i] = recv_msg[i];
                }
            }

            if (check == true)
            {
                if (check_graph_consistency(graph, colour))
                {
                    print("Sending ACK to the master");
                    MPI_Send(colour, n, MPI_INT, 0, ACK, MPI_COMM_WORLD);
                }
            }

            for (int i = 1; i < graph->adj[pid].size(); i++)
            {
                print("Sending colour to " + to_string(graph->adj[pid][i]));
                MPI_Send(colour, n, MPI_INT, graph->adj[pid][i], COLOUR, MPI_COMM_WORLD);
            }
        }
    }
    print("Exiting");
    return;
}

int main(int argc, char *argv[])
{
    Graph *graph = new Graph;
    getGraph(graph);

    colour = (int *)malloc(sizeof(int) * (n));

    // set all to 0
    for (int i = 0; i < n; i++)
    {
        colour[i] = 0;
    }

    // The array colour represents the knowledge of this process regarding the colour of nodes

    int pid, size;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    if (pid == MASTER)
    {
        master_func();
    }

    else
    {
        if (pid == 1)
        {
            colour[pid] = 1;
            MPI_Send(colour, 1, MPI_INT, 0, COLOUR, MPI_COMM_WORLD);

            vector<int> neighbours = graph->adj[pid];
            for (int i = 0; i < neighbours.size(); i++)
            {
                MPI_Send(colour, n, MPI_INT, i, COLOUR, MPI_COMM_WORLD);
            }
        }

        process_func(graph, pid);
    }

    for (int i = 0; i < n; i++)
    {
        std::cout << colour[i] << " ";
    }
    std::cout << "\n";

    MPI_Finalize();
    return 0;
}