#include "types.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <queue>
#include <vector>

using namespace std;

int main() {
    int n, m, source;
    if (!(cin >> n >> m))
        return 0;

    vector<vector<Edge>> adj(n);
    for (int i = 0; i < m; ++i) {
        int u, v;
        double w;
        cin >> u >> v >> w;
        if (u < n && v < n) {
            adj[u].push_back({v, w});
        }
    }

    cin >> source;

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
    cout << "--------------------" << endl;

    for (int i = 0; i < n; ++i) {
        cout << "Node " << i << ": ";
        if (dist[i] == numeric_limits<double>::infinity())
            cout << "INF";
        else
            cout << dist[i];
        cout << endl;
    }

    return 0;
}
