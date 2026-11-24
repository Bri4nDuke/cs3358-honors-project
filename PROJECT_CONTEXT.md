# CS 3358 Honors Project — Shared Context

> Paste this whole file into your AI assistant at the start of a session so it works from the same basis as your partner's AI. Update it (and commit it) whenever the team makes a new decision.

## How the AI should help

- This is graded coursework. **We write all project code ourselves.** The AI explains concepts, points to Visual Studio menus, answers questions, and reviews code we wrote. It does not write finished project files for us. Short illustrative snippets are fine.
- All AI chat logs must be shared with the professor in the status check, so keep the conversation.
- Tools: **Visual Studio Community (C++)**, Git through VS's built-in Git tools, GitHub.

## The course and project

- Course: CS 3358 (Data Structures & Algorithms), Texas State, honors credit project.
- Project 1, **Advanced Shortest Paths and Routing Systems (Graphs)**. The class covered Dijkstra and Bellman-Ford; this project extends them.
- Team of two, each owning one module:
  - **Module 1B — A\* Search (partner).**
  - **Module 1D — Performance Evaluation & Datasets (Brian).** Grid/maze generation, benchmarking harness, charts.
- Modules 1A (Johnson's algorithm) and 1C (math proofs) are **not** assigned to us.

### 1B spec — A\* Search (partner)

- Find the single-source, single-destination shortest path on a spatial graph.
- Heuristic `h(n)` = **Euclidean distance** between node n's (x, y) and the target's (x, y). Must be admissible (never overestimate).
- Priority queue ordered by `f(n) = g(n) + h(n)`.
- Input: `start_node`, `target_node`. Output: total shortest-path distance and the ordered list of node IDs on the path. Track and log the **number of nodes expanded**.
- Professor's test: run A\* and standard Dijkstra on a **100×100 grid from (0,0) to (99,99)**. Both must return the **same distance**; A\* must expand **70–90% fewer nodes**.

### 1D spec — Performance Evaluation & Datasets (Brian)

- **Dataset 1 (sparse vs. dense graphs):** directed graphs, N = 10 to 1,000 nodes, edge density 5% to 80%, positive weights and negative weights *without* negative cycles.
- **Dataset 2 (grid maps):** 20×20 up to 200×200, obstacle density 0%, 20%, 40%, cells have (x, y) coordinates, for A\* testing.
- **Benchmarks:**
  - Johnson's vs. Floyd-Warshall on Dataset 1: execution time vs. number of vertices; execution time vs. edge density.
  - A\* vs. Dijkstra on identical grid maps: total nodes expanded; execution time vs. Manhattan/Euclidean distance to target.
- **Charts:** log-log or linear plots of Johnson's vs. Floyd-Warshall; bar/scatter charts of A\*'s node-expansion reduction vs. Dijkstra under varying obstacle density.
- Professor also requires: **share the input file format for both of our works.**

## Decisions made

| Decision | Choice | Why |
|---|---|---|
| Language / IDE | C++ in Visual Studio Community | Course requirement / team choice |
| Movement on grids | **8-directional**: straight = 1, diagonal = √2. **Corner rule B:** a diagonal may cut past one wall's corner, but is blocked if *both* side cells are walls (no squeezing between diagonal walls). | With 4-directional movement, the Euclidean heuristic is too weak and A\* expands almost as many nodes as Dijkstra on open grids, failing the 70–90% test |
| Shared grid code | `Common/Grid.h` (header-only), used by every project | Guarantees A\*, Dijkstra, the generator and the benchmark all use the same graph |
| `Grid.h` authorship | Started from a Claude-written reference; Brian hand-typed and revised the final version | Disclosed in the status check |
| Johnson's (1A) | Source TBD: we write it, AI-generated, or provided by the professor. Logged in the status check either way | Needed for the 1D Johnson's vs. Floyd-Warshall benchmark |
| Comparing distances | Use a tolerance, e.g. `std::abs(a - b) < 1e-9`, not `==` | √2 costs make distances floating-point |

## Repository and solution setup

- GitHub (private): `https://github.com/Bri4nDuke/cs3358-honors-project`
- One VS solution, `Benchmark.slnx`, with three C++ console app projects:

```
Benchmark.slnx
├── Common/
│   └── Grid.h              shared, header-only (talk before editing)
├── maps/
│   └── sample_10x10.txt    hand-made test map (shortest path = 17.414 with rule B)
├── MazeGen/                Brian: generates map files
├── Pathfinding/            partner: A* + Dijkstra and a test driver
├── Benchmark/              Brian: runs both algorithms, writes CSV results
└── .gitignore              excludes x64/, .vs/, *.exe, results/
```

- Every project has `$(SolutionDir)Common` on its include path, so `#include "Grid.h"` works anywhere. To see the file in Solution Explorer: right-click the project's **Header Files → Add → Existing Item → ..\Common\Grid.h**.
- Programs run from inside their project folder when started with F5, so the sample map path is `..\\maps\\sample_10x10.txt`.

## Grid map file format

```
; optional comment lines start with ';' (MazeGen records seed and density here)
GRID <width> <height>
START <x> <y>
GOAL <x> <y>
<height rows, each exactly <width> characters: '.' open, '#' blocked>
```

- x = column (0 = left), y = row (0 = top).
- **Node ID = y * width + x.**

## Grid.h API (summary)

| Item | What it does |
|---|---|
| `struct Grid` | `width`, `height`, `startX/Y`, `goalX/Y`, `cells` (row-by-row `'.'`/`'#'`) |
| `id(x, y)`, `xOf(node)`, `yOf(node)` | Convert between (x, y) and node ID |
| `startNode()`, `goalNode()`, `nodeCount()` | Convenience accessors |
| `inBounds(x, y)`, `isOpen(x, y)`, `setBlocked(x, y, b)` | Cell checks / edits |
| `struct Edge { int to; double cost; }` | One move to a neighbor |
| `neighbors(g, node, out)` | Fills `out` with open neighbors and their costs (8-dir, corner rule B). **A\* and Dijkstra should both use this.** |
| `euclidean(g, a, b)` | Straight-line distance between two nodes: the A\* heuristic |
| `loadGrid(path)` | Reads a map file; throws `std::runtime_error` with a clear message if the file is invalid |
| `saveGrid(g, path, comment)` | Writes a map file |

## Still to agree on

1. **A\* / Dijkstra function signature.** The Benchmark calls the partner's code, so this must be fixed early. A starting proposal to discuss:
   ```cpp
   struct PathResult {
       double distance;        // total path cost (infinity if no path)
       std::vector<int> path;  // node IDs from start to goal
       int nodesExpanded;      // nodes removed from the priority queue and processed
   };
   PathResult aStar(const Grid& g, int start, int goal);
   PathResult dijkstra(const Grid& g, int start, int goal);
   ```
   Agree on the exact definition of "nodes expanded" so the comparison is fair.
2. **Who writes Dijkstra:** suggested partner, since it is nearly identical to A\*.
3. **Where algorithm files live:** suggested `Common/AStar.h/.cpp` and `Common/Dijkstra.h/.cpp` (partner-owned) so Benchmark can call them. Any project using a `.cpp` in `Common` adds it via **Add → Existing Item**.
4. **Tie-breaking in A\*:** on open grids many nodes have equal `f`. Preferring the node with the larger `g` (closer to the goal) reduces expansions noticeably.
5. **Dataset 1 file format** for general directed graphs (Grid.h only covers grids). Suggested edge list:
   ```
   GRAPH <nodes> <edges>
   <from> <to> <weight>
   ...
   ```
6. **Johnson's source** (see Decisions) and **Floyd-Warshall** (simple triple loop, we write it).

## Ownership and Git workflow

- Brian owns `MazeGen`, `Benchmark`, `maps/`. Partner owns `Pathfinding` and the A\*/Dijkstra files. `Common/Grid.h` is shared: tell each other before editing.
- Only Brian adds projects or changes project settings (those rewrite `.slnx`/`.vcxproj` files, which merge badly).
- Routine (VS → **View → Git Changes**): **Pull** before starting, **Commit** small working steps with clear messages, **Push** when done. See each other's work via **Git → View Branch History** or the repo's Commits page on GitHub.
- On a merge conflict: stop and message each other before clicking anything. Never force-push.

## Timeline

| When | Brian (1D) | Partner (1B) |
|---|---|---|
| By 10/29 | Agree on function signature; MazeGen loads and prints the sample map | Clone repo; A\* skeleton loads the sample map |
| 10/29–10/31 | Grid generator: obstacles, seed, guaranteed path | A\* + Dijkstra working on the sample map |
| **10/31** | **Status check due** (each person submits individually) | same |
| Week of 11/2 | Benchmark harness, all grid sizes/densities, CSV output | Pass the 100×100 test, nodes-expanded logging |
| Week of 11/9 | Dataset 1 graph generator; Johnson's / Floyd-Warshall | Edge cases (no path, start = goal), tie-breaking |
| After | Charts, write-up, integration | Write-up |

Final due date: **TBD** (check Canvas).

## Status check (due 10/31) — each person answers

a. Overall status (200 words) · b. Everything done since assignment, with readings, sources, **AI chat logs**, team tasks and individual progress (500 words) · c. Where stuck (200) · d. Help needed from the professor (200), e.g. the Johnson's question · e. Next steps (200).
Testing rule: test with multiple datasets, **varying one parameter at a time** (e.g., fixed size with changing obstacle density, then fixed density with changing size).
