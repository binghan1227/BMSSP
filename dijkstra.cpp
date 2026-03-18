#include "types.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <queue>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    bool quiet = false;
    bool binary = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0)
            quiet = true;
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--binary") == 0)
            binary = true;
    }

    int n, m, source;
    vector<vector<Edge>> adj;

    if (binary) {
        int32_t header[3];
        if (fread(header, sizeof(int32_t), 3, stdin) != 3)
            return 0;
        n = header[0];
        m = header[1];
        source = header[2];
        adj.resize(n);

        struct BinaryEdge {
            int32_t u, v;
            double w;
        };
        vector<BinaryEdge> edges(m);
        if (fread(edges.data(), sizeof(BinaryEdge), m, stdin) != (size_t)m)
            return 1;
        for (int i = 0; i < m; ++i) {
            if (edges[i].u < n && edges[i].v < n)
                adj[edges[i].u].push_back({edges[i].v, edges[i].w});
        }
    } else {
        if (!(cin >> n >> m))
            return 0;
        adj.resize(n);
        for (int i = 0; i < m; ++i) {
            int u, v;
            double w;
            cin >> u >> v >> w;
            if (u < n && v < n)
                adj[u].push_back({v, w});
        }
        cin >> source;
    }

    auto start_time = chrono::high_resolution_clock::now();

    // Standard Dijkstra using min-heap
    vector<double> dist(n, numeric_limits<double>::infinity());
    priority_queue<State, vector<State>, greater<State>> pq;
    dist[source] = 0.0;
    pq.push({source, 0.0});

    while (!pq.empty()) {
        State cur = pq.top();
        pq.pop();

        if (cur.cost > dist[cur.node_id])
            continue;

        for (const Edge& e : adj[cur.node_id]) {
            double new_dist = cur.cost + e.weight;
            if (new_dist < dist[e.to]) {
                dist[e.to] = new_dist;
                pq.push({e.to, new_dist});
            }
        }
    }

    auto end_time = chrono::high_resolution_clock::now();

    auto duration =
        chrono::duration_cast<chrono::microseconds>(end_time - start_time);
    cout << "Dijkstra Time: " << duration.count() / 1000.0 << " ms" << endl;

    if (!quiet) {
        cout << "--------------------" << endl;
        for (int i = 0; i < n; ++i) {
            cout << "Node " << i << ": ";
            if (dist[i] == numeric_limits<double>::infinity())
                cout << "INF";
            else
                cout << dist[i];
            cout << endl;
        }
    }

    return 0;
}
