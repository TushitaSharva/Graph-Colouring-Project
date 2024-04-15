/* Sequential Greedy Algorithm for Graph colouring */
#include <iostream>
#include <vector>
using namespace std;

int main()
{
    int V = 4; // Number of vertices
    vector<vector<int>> E = {{0, 1, 3}, {1, 0, 2, 3}, {2, 1}, {3, 0, 1}}; // Edges
    int C = 1; // Number of colours required

    vector<int> colour(V);
    colour[0] = 1;

    for(int i = 1; i <= V; i++)
    {
        int myColour = 1;
        int row = 0;
        int col = 0;

        for(row = 0; row < V - 1; row++)
        {
            int thisrow = colour[E[row][0]];
            for(col = 1; col < E[row].size(); col++)
            {
                if(E[row][col] == i)
                {
                    myColour++;
                }
            }
        }

        colour[i] = myColour;
        C = max(C, myColour);
    }

    std::cout << C << "\n";

    return 0; 
}