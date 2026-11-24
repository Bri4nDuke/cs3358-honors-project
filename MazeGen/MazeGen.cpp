// MazeGen.cpp : generates grid maps for benchmarking A* vs. Dijkstra (Module 1D, Dataset 2).

#include <direct.h> // _mkdir
#include <iostream>
<<<<<<< HEAD
using namespace std;

int main()
{
    cout << "Hello World!\n";
}
=======
#include <queue>
#include <random>
#include <string>
#include <vector>
#include "Grid.h"

// Returns true if the goal can be reached from the start using the same moves A* uses.
bool pathExists(const Grid& g)
{
    std::vector<char> visited(g.nodeCount(), 0);
    std::vector<Edge> next;
    std::queue<int> frontier;
    frontier.push(g.startNode());
    visited[g.startNode()] = 1;

    while (!frontier.empty())
    {
        int node = frontier.front();
        frontier.pop();
        if (node == g.goalNode())
            return true;

        neighbors(g, node, next);
        for (const Edge& e : next)
        {
            if (!visited[e.to])
            {
                visited[e.to] = 1;
                frontier.push(e.to);
            }
        }
    }
    return false;
}

// Builds a size x size grid with the start in the top-left corner and the goal in the
// bottom-right. Every other cell is blocked with probability `density`. Retries until a path exists.
Grid makeGrid(int size, double density, std::mt19937& rng, int& attempts)
{
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    Grid g;
    g.width = size;
    g.height = size;
    g.startX = 0;
    g.startY = 0;
    g.goalX = size - 1;
    g.goalY = size - 1;

    attempts = 0;
    do
    {
        ++attempts;
        g.cells.assign(g.nodeCount(), '.');
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                if (coin(rng) < density)
                    g.setBlocked(x, y, true);
        g.setBlocked(g.startX, g.startY, false);
        g.setBlocked(g.goalX, g.goalY, false);
    } while (!pathExists(g));

    return g;
}

int main()
{
    const int sizes[] = { 20, 50, 100, 200 };
    const int densities[] = { 0, 20, 40 }; // percent of cells blocked
    const int seeds[] = { 1, 2, 3 };
    const std::string folder = "..\\maps\\gen\\";
    (void)_mkdir("..\\maps");
    (void)_mkdir(folder.c_str());
>>>>>>> 92047cdd944c9ed1b8984c0067d4467e00fb07ff

    try
    {
        int count = 0;
        for (int size : sizes)
            for (int density : densities)
                for (int seed : seeds)
                {
                    std::mt19937 rng(seed);
                    int attempts = 0;
                    Grid g = makeGrid(size, density / 100.0, rng, attempts);

                    std::string name = "grid_" + std::to_string(size) + "_d" + std::to_string(density) +
                        "_s" + std::to_string(seed) + ".txt";
                    std::string comment = "seed " + std::to_string(seed) + ", density " + std::to_string(density) +
                        "%, attempts " + std::to_string(attempts);
                    saveGrid(g, folder + name, comment);

                    std::cout << name << "  (" << attempts << (attempts == 1 ? " attempt)\n" : " attempts)\n");
                    ++count;
                }
        std::cout << count << " maps written to maps\\gen\\\n";
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}