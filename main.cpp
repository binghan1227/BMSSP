#include "bmssp.h"
#include "types.h"
#include <chrono>
#include <iostream>
#include <limits>
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
    vector<double> results = solve_sssp(n, adj, source);
    auto end_time = chrono::high_resolution_clock::now();

    auto duration =
        chrono::duration_cast<chrono::microseconds>(end_time - start_time);
    cout << "BMSSP Time: " << duration.count() / 1000.0 << " ms" << endl;
    cout << "--------------------" << endl;

    for (int i = 0; i < n; ++i) {
        cout << "Node " << i << ": ";
        if (results[i] == numeric_limits<double>::infinity())
            cout << "INF";
        else
            cout << results[i];
        cout << endl;
    }

    return 0;
}
