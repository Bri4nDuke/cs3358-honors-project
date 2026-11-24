// Benchmark.cpp : runs A* and Dijkstra on every generated map and writes the results to CSV (Module 1D).

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include "Grid.h"
#include "Astar.h"

const int REPEATS = 5; // each search is timed this many times and averaged

// Runs one search REPEATS times; returns the first result with its time replaced by the average.
template <typename Search>
SearchResult timed(Search search, const Grid& g)
{
    SearchResult first = search(g, g.startNode(), g.goalNode());
    double total = first.milliseconds;
    for (int i = 1; i < REPEATS; ++i)
        total += search(g, g.startNode(), g.goalNode()).milliseconds;
    first.milliseconds = total / REPEATS;
    return first;
}

int main()
{
    const int sizes[] = { 20, 50, 100, 200 };
    const int densities[] = { 0, 20, 40 };
    const int seeds[] = { 1, 2, 3 };

    std::ofstream csv("benchmark.csv");
    csv << "size,density,seed,cells,blocked_pct,straight_line,distance,"
        "astar_nodes,dijkstra_nodes,reduction_pct,astar_ms,dijkstra_ms,match\n";

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "map                    A* nodes  Dijk nodes  reduction   A* ms  Dijk ms  match\n";

    try
    {
        for (int size : sizes)
            for (int density : densities)
                for (int seed : seeds)
                {
                    std::string name = "grid_" + std::to_string(size) + "_d" + std::to_string(density) +
                        "_s" + std::to_string(seed);
                    Grid g = loadGrid("..\\maps\\gen\\" + name + ".txt");

                    int blocked = 0;
                    for (char c : g.cells)
                        if (c == '#')
                            ++blocked;

                    SearchResult a = timed([](const Grid& m, int s, int t) { return aStar(m, s, t); }, g);
                    SearchResult d = timed([](const Grid& m, int s, int t) { return dijkstra(m, s, t); }, g);

                    bool match = a.found && d.found && std::abs(a.distance - d.distance) < 1e-9;
                    double reduction = 100.0 * (d.nodesExpanded - a.nodesExpanded) / d.nodesExpanded;

                    csv << size << ',' << density << ',' << seed << ',' << g.nodeCount() << ','
                        << 100.0 * blocked / g.nodeCount() << ','
                        << euclidean(g, g.startNode(), g.goalNode()) << ',' << a.distance << ','
                        << a.nodesExpanded << ',' << d.nodesExpanded << ',' << reduction << ','
                        << a.milliseconds << ',' << d.milliseconds << ',' << (match ? "yes" : "NO") << '\n';

                    std::cout << std::left << std::setw(22) << name << std::right
                        << std::setw(9) << a.nodesExpanded << std::setw(12) << d.nodesExpanded
                        << std::setw(10) << reduction << '%' << std::setw(9) << a.milliseconds
                        << std::setw(9) << d.milliseconds << "  " << (match ? "yes" : "NO") << '\n';
                }
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nResults written to Benchmark\\benchmark.csv\n";
    return 0;
}