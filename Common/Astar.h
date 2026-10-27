#pragma once
#include "Grid.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <queue>
#include <vector>
using namespace std;

struct SearchResult
{
    bool found = false;
    double distance = numeric_limits<double>::infinity();
    vector<int> path;
    long long nodesExpanded = 0;
    double milliseconds = 0.0;
};

namespace astar_detail
{
    struct QueueEntry
    {
        double f;  
        double h;  
        int node;
    };

    struct SmallestFOnTop
    {
        bool operator()(const QueueEntry& a, const QueueEntry& b) const
        {
            if (a.f != b.f)
                return a.f > b.f;
            return a.h > b.h;
        }
    };
}

inline SearchResult findShortestPath(const Grid& g, int start, int target, bool useHeuristic,
                                     bool allowDiagonal = true)
{
    using Clock = chrono::steady_clock;
    const Clock::time_point timerStart = Clock::now();

    SearchResult result;
    const int n = g.nodeCount();
    const bool startOk = start >= 0 && start < n && g.cells[start] == '.';
    const bool targetOk = target >= 0 && target < n && g.cells[target] == '.';

    if (startOk && targetOk)
    {
        const double INF = numeric_limits<double>::infinity();
        vector<double> gCost(g.cells.size(), INF);  
        vector<int> parent(g.cells.size(), -1);      
        vector<char> expanded(g.cells.size(), 0);    
        priority_queue<astar_detail::QueueEntry, vector<astar_detail::QueueEntry>,
                            astar_detail::SmallestFOnTop> queue;
        vector<Edge> next;
        next.reserve(8);

        gCost[start] = 0.0;
        const double hStart = useHeuristic ? euclidean(g, start, target) : 0.0;
        queue.push({ hStart, hStart, start });

        while (!queue.empty())
        {
            const int current = queue.top().node;
            queue.pop();

            if (expanded[current])
                continue;
            expanded[current] = 1;
            ++result.nodesExpanded;

            if (current == target)
            {
                result.found = true;
                break;
            }

            neighbors(g, current, next, allowDiagonal);
            for (const Edge& e : next)
            {
                if (expanded[e.to])
                    continue;
                const double newCost = gCost[current] + e.cost;
                if (newCost < gCost[e.to])
                {
                    gCost[e.to] = newCost;
                    parent[e.to] = current;
                    const double h = useHeuristic ? euclidean(g, e.to, target) : 0.0;
                    queue.push({ newCost + h, h, e.to });
                }
            }
        }

        if (result.found)
        {
            result.distance = gCost[target];
            for (int node = target; node != -1; node = parent[node])
                result.path.push_back(node);
            reverse(result.path.begin(), result.path.end());
        }
    }

    result.milliseconds = chrono::duration<double, milli>(Clock::now() - timerStart).count();
    return result;
}

// A* search: shortest path using the Euclidean distance heuristic.
inline SearchResult aStar(const Grid& g, int start, int target, bool allowDiagonal = true)
{
    return findShortestPath(g, start, target, true, allowDiagonal);
}

// Dijkstra's algorithm: the same search with no heuristic (h = 0), used as the baseline.
inline SearchResult dijkstra(const Grid& g, int start, int target, bool allowDiagonal = true)
{
    return findShortestPath(g, start, target, false, allowDiagonal);
}