#pragma once

// Shared grid definition and map-file reader/writer used by MazeGen, Pathfinding, and Benchmark.
// Header-only: just #include "Grid.h", no .cpp file to add to your project.
//
// ---------------------------------------------------------------------------
// MAP FILE FORMAT (.txt)
// ---------------------------------------------------------------------------
//   GRID <width> <height>
//   START <x> <y>
//   GOAL <x> <y>
//   <height lines, each exactly <width> characters>
//
//   '.' = open cell, '#' = blocked cell.
//   x is the column (0 = left), y is the row (0 = top).
//   Lines starting with ';' anywhere in the header are comments (MazeGen writes the
//   seed and obstacle density there so every map can be regenerated).
//
// Example (5 x 4, start top-left, goal bottom-right):
//   GRID 5 4
//   START 0 0
//   GOAL 4 3
//   ..#..
//   .##..
//   ...#.
//   #....
//
// ---------------------------------------------------------------------------
// GRAPH MODEL (what A* and Dijkstra search)
// ---------------------------------------------------------------------------
//   Node ID   = y * width + x
//   Movement  = 8-directional by default: straight moves cost 1, diagonal moves cost sqrt(2).
//               A diagonal move may pass the corner of ONE blocked cell (corner-cutting), but
//               may not squeeze between TWO blocked cells that touch diagonally.
//               Set allowDiagonal = false for 4-directional movement (all moves cost 1).
//   Heuristic = Euclidean distance between (x, y) coordinates, which never overestimates
//               the true remaining cost under either movement rule (admissible).

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

struct Grid
{
    int width = 0;
    int height = 0;
    int startX = 0, startY = 0;
    int goalX = 0, goalY = 0;
    vector<char> cells; // width * height entries, row by row; '.' open, '#' blocked

    int nodeCount() const { return width * height; }
    int id(int x, int y) const { return y * width + x; }
    int xOf(int node) const { return node % width; }
    int yOf(int node) const { return node / width; }
    int startNode() const { return id(startX, startY); }
    int goalNode() const { return id(goalX, goalY); }

    bool inBounds(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
    bool isOpen(int x, int y) const { return inBounds(x, y) && cells[id(x, y)] == '.'; }
    void setBlocked(int x, int y, bool blocked) { cells[id(x, y)] = blocked ? '#' : '.'; }
};

struct Edge
{
    int to;
    double cost;
};

// Straight-line distance between two nodes; use as h(n) with b = the goal node.
inline double euclidean(const Grid& g, int a, int b)
{
    double dx = g.xOf(a) - g.xOf(b);
    double dy = g.yOf(a) - g.yOf(b);
    return sqrt(dx * dx + dy * dy);
}

// Fills `out` with the open neighbors of `node`. Pass the same vector every call to avoid
// reallocating it inside the search loop.
inline void neighbors(const Grid& g, int node, vector<Edge>& out, bool allowDiagonal = true)
{
    static const int dx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
    static const int dy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    static const double diagonalCost = sqrt(2.0);

    out.clear();
    int x = g.xOf(node), y = g.yOf(node);
    int directions = allowDiagonal ? 8 : 4;
    for (int i = 0; i < directions; ++i)
    {
        int nx = x + dx[i], ny = y + dy[i];
        if (!g.isOpen(nx, ny))
            continue;
        if (i >= 4 && !g.isOpen(nx, y) && !g.isOpen(x, ny))
            continue; // no squeezing diagonally between two walls (passing one wall's corner is fine)
        out.push_back({ g.id(nx, ny), i < 4 ? 1.0 : diagonalCost });
    }
}

// Reads a map file. Throws runtime_error with a readable message if the file is bad.
inline Grid loadGrid(const string& path)
{
    ifstream in(path);
    if (!in)
        throw runtime_error("Cannot open map file: " + path);

    Grid g;
    bool haveGrid = false, haveStart = false, haveGoal = false;
    string line;
    while (!(haveGrid && haveStart && haveGoal) && getline(in, line))
    {
        if (line.empty() || line[0] == ';')
            continue;
        istringstream fields(line);
        string key;
        fields >> key;
        if (key == "GRID")
            haveGrid = static_cast<bool>(fields >> g.width >> g.height);
        else if (key == "START")
            haveStart = static_cast<bool>(fields >> g.startX >> g.startY);
        else if (key == "GOAL")
            haveGoal = static_cast<bool>(fields >> g.goalX >> g.goalY);
        else
            throw runtime_error(path + ": unexpected header line: " + line);
    }
    if (!haveGrid || !haveStart || !haveGoal)
        throw runtime_error(path + ": missing GRID, START, or GOAL line");
    if (g.width <= 0 || g.height <= 0)
        throw runtime_error(path + ": grid size must be positive");

    g.cells.reserve(static_cast<size_t>(g.width) * g.height);
    for (int y = 0; y < g.height; ++y)
    {
        if (!getline(in, line))
            throw runtime_error(path + ": expected " + to_string(g.height) + " rows, found " + to_string(y));
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (static_cast<int>(line.size()) != g.width)
            throw runtime_error(path + ": row " + to_string(y) + " has " + to_string(line.size()) +
                                     " characters, expected " + to_string(g.width));
        for (char c : line)
        {
            if (c != '.' && c != '#')
                throw runtime_error(path + ": row " + to_string(y) + " contains '" + c + "' (use '.' or '#')");
            g.cells.push_back(c);
        }
    }

    if (!g.isOpen(g.startX, g.startY))
        throw runtime_error(path + ": START is outside the grid or on a blocked cell");
    if (!g.isOpen(g.goalX, g.goalY))
        throw runtime_error(path + ": GOAL is outside the grid or on a blocked cell");
    return g;
}

// Writes a map file. `comment` (optional) is written as ';' lines, e.g. "seed 42, density 0.20".
inline void saveGrid(const Grid& g, const string& path, const string& comment = "")
{
    ofstream out(path);
    if (!out)
        throw runtime_error("Cannot write map file: " + path);

    if (!comment.empty())
        out << "; " << comment << '\n';
    out << "GRID " << g.width << ' ' << g.height << '\n';
    out << "START " << g.startX << ' ' << g.startY << '\n';
    out << "GOAL " << g.goalX << ' ' << g.goalY << '\n';
    for (int y = 0; y < g.height; ++y)
    {
        out.write(&g.cells[g.id(0, y)], g.width);
        out << '\n';
    }
}