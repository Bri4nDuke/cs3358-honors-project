#include "../Common/Astar.h"
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
using namespace std;

namespace
{
    const char* const kRequiredTestMap = "Open_100x100";
    const double kRequiredReductionPercent = 70.0;

    bool fileExists(const string& path)
    {
        ifstream f(path);
        return static_cast<bool>(f);
    }
    string findRepoRoot()
    {
        const char* const prefixes[] = { "", "../", "../../", "../../../" };
        for (const char* prefix : prefixes)
            if (fileExists(string(prefix) + "Common/Grid.h"))
                return prefix;
        return "";
    }

    // Accepts a map path as typed, or relative to the repository folder ("maps/x.txt").
    string resolveMapPath(const string& path, const string& root)
    {
        if (fileExists(path))
            return path;
        if (fileExists(root + path))
            return root + path;
        return path;  
    }

    void makeDirectory(const string& path)
    {
#ifdef _WIN32
        (void)_mkdir(path.c_str());
#else
        (void)mkdir(path.c_str(), 0755);
#endif
    }

    // "../maps/sample_10x10.txt" -> "sample_10x10"
    string mapName(const string& path)
    {
        const size_t slash = path.find_last_of("/\\");
        const string file = (slash == string::npos) ? path : path.substr(slash + 1);
        const size_t dot = file.find_last_of('.');
        return (dot == string::npos) ? file : file.substr(0, dot);
    }

    // Node 11 on a 10-wide grid -> "11 (1,1)"
    string describeNode(const Grid& g, int node)
    {
        ostringstream s;
        s << node << " (" << g.xOf(node) << "," << g.yOf(node) << ")";
        return s.str();
    }

    int parseInt(const char* text, const char* what)
    {
        istringstream in(text);
        int value = 0;
        char extra = 0;
        if (!(in >> value) || (in >> extra))
            throw runtime_error(string(what) + " must be a whole number, got \"" + text + "\"");
        return value;
    }

    string yesNo(bool value) { return value ? "yes" : "no"; }

    string formatDistance(const SearchResult& r)
    {
        if (!r.found)
            return "no path";
        ostringstream s;
        s << fixed << setprecision(4) << r.distance;
        return s.str();
    }

    string formatTime(double ms)
    {
        ostringstream s;
        s << fixed << setprecision(3) << ms;
        return s.str();
    }

    bool runMap(Grid g, const string& mapPath, const string& root, bool requiredTest)
    {
        const int start = g.startNode();
        const int target = g.goalNode();

        const SearchResult a = aStar(g, start, target);
        const SearchResult d = dijkstra(g, start, target);

        const bool sameAnswer = (a.found == d.found) && (!a.found || fabs(a.distance - d.distance) < 1e-9);
        const double reduction = (d.nodesExpanded > 0)
            ? 100.0 * (1.0 - static_cast<double>(a.nodesExpanded) / static_cast<double>(d.nodesExpanded))
            : 0.0;
        const bool passed = requiredTest ? (sameAnswer && a.found && reduction >= kRequiredReductionPercent)
                                         : sameAnswer;

        ostringstream reductionText;
        reductionText << fixed << setprecision(1) << reduction << "% fewer nodes than Dijkstra";

        // results file 
        makeDirectory(root + "results");
        const string outPath = root + "results/" + mapName(mapPath) + "_result.txt";
        ofstream out(outPath);
        if (!out)
            throw runtime_error("Cannot write results file: " + outPath);

        out << "Module 1B - A* Search vs Dijkstra\n";
        out << "=================================\n";
        out << "Map file   : " << mapPath << '\n';
        out << "Map size   : " << g.width << " x " << g.height << " (" << g.nodeCount() << " nodes)\n";
        out << "Start      : node " << describeNode(g, start) << '\n';
        out << "Target     : node " << describeNode(g, target) << '\n';
        out << "Movement   : 8 directions; straight step = 1, diagonal step = sqrt(2) = 1.414\n";
        out << "             diagonal past the corner of one wall: allowed\n";
        out << "             diagonal squeezing between two walls: not allowed\n";
        out << "Heuristic  : Euclidean (straight-line) distance to the target\n";
        out << "Node ID    : y * width + x, shown as ID (x,y); x = column, y = row, (0,0) = top-left\n\n";

        out << left;
        out << setw(20) << "" << setw(15) << "A*" << "Dijkstra\n";
        out << setw(20) << "Path found" << setw(15) << yesNo(a.found) << yesNo(d.found) << '\n';
        out << setw(20) << "Distance" << setw(15) << formatDistance(a) << formatDistance(d) << '\n';
        out << setw(20) << "Nodes expanded" << setw(15) << a.nodesExpanded << d.nodesExpanded << '\n';
        out << setw(20) << "Time (ms)" << setw(15) << formatTime(a.milliseconds) << formatTime(d.milliseconds) << "\n\n";

        out << setw(20) << "Same distance" << (sameAnswer ? "YES" : "NO") << '\n';
        out << setw(20) << "A* expanded" << reductionText.str() << '\n';
        if (requiredTest)
            out << setw(20) << "Required test" << (passed ? "PASS" : "FAIL")
                << "  (same distance and at least " << kRequiredReductionPercent << "% fewer nodes)\n";
        out << '\n';

        if (a.found)
        {
            out << "A* path: " << a.path.size() << " nodes, " << (a.path.size() - 1) << " steps\n";
            for (size_t i = 0; i < a.path.size(); ++i)
            {
                if (i % 6 == 0)
                    out << "  ";
                out << describeNode(g, a.path[i]);
                if (i + 1 == a.path.size())
                    out << '\n';
                else
                    out << (i % 6 == 5 ? " ->\n" : " -> ");
            }
        }
        else
        {
            out << "No path found: the target cannot be reached from the start.\n";
        }
        out.close();

        // ---- screen summary ----
        cout << (requiredTest ? "[Required test] " : "") << "Map " << mapName(mapPath) << " ("
                  << g.width << " x " << g.height << "), from (" << g.startX << "," << g.startY << ") to ("
                  << g.goalX << "," << g.goalY << ")\n";
        cout << "  A*        distance " << formatDistance(a) << ", nodes expanded " << a.nodesExpanded
                  << ", " << formatTime(a.milliseconds) << " ms\n";
        cout << "  Dijkstra  distance " << formatDistance(d) << ", nodes expanded " << d.nodesExpanded
                  << ", " << formatTime(d.milliseconds) << " ms\n";
        cout << "  Same distance: " << (sameAnswer ? "YES" : "NO") << "   A* expanded " << reductionText.str() << '\n';
        if (requiredTest)
            cout << "  Required test: " << (passed ? "PASS" : "FAIL") << '\n';
        cout << "  Full results saved to " << outPath << "\n\n";

        return passed;
    }

    void printUsage()
    {
        cout << "Usage:\n"
                  << "  Pathfinding                                     run the sample map and the 100 x 100 test\n"
                  << "  Pathfinding <map file>                          use START and GOAL from the map file\n"
                  << "  Pathfinding <map file> <startX> <startY> <goalX> <goalY>\n";
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const string root = findRepoRoot();

        if (argc == 1)
        {
            const string sample = root + "maps/sample_10x10.txt";
            const string required = root + "maps/" + kRequiredTestMap + ".txt";
            bool ok = runMap(loadGrid(sample), sample, root, false);
            ok = runMap(loadGrid(required), required, root, true) && ok;
            return ok ? 0 : 1;
        }

        if (argc == 2 || argc == 6)
        {
            const string mapPath = resolveMapPath(argv[1], root);
            Grid g = loadGrid(mapPath);
            bool customStartGoal = false;

            if (argc == 6)
            {
                const int sx = parseInt(argv[2], "startX");
                const int sy = parseInt(argv[3], "startY");
                const int gx = parseInt(argv[4], "goalX");
                const int gy = parseInt(argv[5], "goalY");
                if (!g.isOpen(sx, sy))
                    throw runtime_error("start (" + to_string(sx) + "," + to_string(sy) +
                                             ") is outside the map or on a blocked cell");
                if (!g.isOpen(gx, gy))
                    throw runtime_error("goal (" + to_string(gx) + "," + to_string(gy) +
                                             ") is outside the map or on a blocked cell");
                customStartGoal = (sx != g.startX || sy != g.startY || gx != g.goalX || gy != g.goalY);
                g.startX = sx;
                g.startY = sy;
                g.goalX = gx;
                g.goalY = gy;
            }

            const bool requiredTest = !customStartGoal && mapName(mapPath) == kRequiredTestMap;
            return runMap(g, mapPath, root, requiredTest) ? 0 : 1;
        }

        printUsage();
        return 1;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}