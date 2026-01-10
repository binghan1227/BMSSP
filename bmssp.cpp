#include "bmssp.h"
#include "block_list.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

using namespace std;

static pair<vector<int>, vector<int>>
find_pivots(double bound, const vector<int>& frontier, int k,
            const vector<vector<Edge>>& adj, vector<double>& min_costs) {
    vector<int> all_layers = frontier;
    vector<int> last_layer = frontier;
    unordered_map<int, int> bp_map;

    for (int i = 0; i < k; ++i) {
        vector<int> new_layer;
        for (int u : last_layer) {
            for (auto& e : adj[u]) {
                double d = min_costs[u] + e.weight;
                if (d <= min_costs[e.to]) {
                    min_costs[e.to] = d;
                    if (d < bound) {
                        new_layer.push_back(e.to);
                        bp_map[e.to] = u;
                    }
                }
            }
        }
        all_layers.insert(all_layers.end(), new_layer.begin(), new_layer.end());
        last_layer = new_layer;
        if (all_layers.size() > (size_t)k * frontier.size())
            return {frontier, all_layers};
    }

    unordered_map<int, int> tree_size;
    unordered_set<int> pivots;
    for (int leaf : last_layer) {
        int cur = leaf;
        int count = 0;
        while (bp_map.count(cur)) {
            cur = bp_map[cur];
            count++;
        }
        tree_size[cur] += count;
        if (tree_size[cur] >= k)
            pivots.insert(cur);
    }
    return {vector<int>(pivots.begin(), pivots.end()), all_layers};
}

static pair<double, vector<int>> base_bmssp(double B, int node_id, int k,
                                            const vector<vector<Edge>>& adj,
                                            vector<double>& min_costs) {
    priority_queue<State, vector<State>, greater<State>> pq;
    pq.push({node_id, min_costs[node_id]});
    vector<int> u_init;
    unordered_set<int> visited;
    double max_cost = min_costs[node_id];

    while (!pq.empty() && u_init.size() < (size_t)k + 1) {
        State top = pq.top();
        pq.pop();
        if (visited.count(top.node_id))
            continue;
        visited.insert(top.node_id);
        u_init.push_back(top.node_id);
        max_cost = max(max_cost, top.cost);

        for (auto& e : adj[top.node_id]) {
            double d = top.cost + e.weight;
            if (d <= min_costs[e.to] && d < B) {
                min_costs[e.to] = d;
                pq.push({e.to, d});
            }
        }
    }
    if (u_init.size() <= (size_t)k)
        return {B, u_init};
    vector<int> filtered_u;
    for (int id : u_init)
        if (min_costs[id] < max_cost)
            filtered_u.push_back(id);
    return {max_cost, filtered_u};
}

static pair<double, vector<int>>
bmssp_bounded(int l, double B, const vector<int>& frontier, int k, int t,
              const vector<vector<Edge>>& adj, vector<double>& min_costs) {
    if (l == 0)
        return base_bmssp(B, frontier[0], k, adj, min_costs);

    auto pivot_data = find_pivots(B, frontier, k, adj, min_costs);
    int M = pow(2, t * (l - 1));
    BlockList block_list(M, B);
    double min_ub = B;

    for (int p : pivot_data.first) {
        block_list.insert(p, min_costs[p]);
        min_ub = min(min_ub, min_costs[p]);
    }

    vector<int> u_set;
    size_t max_u = k * pow(2, t * l);

    while (u_set.size() < max_u && !block_list.is_empty()) {
        auto pulled = block_list.pull();
        auto res = bmssp_bounded(l - 1, pulled.bound, pulled.frontier, k, t,
                                 adj, min_costs);
        min_ub = res.first;

        vector<pair<int, double>> to_prepend;
        for (int u : res.second) {
            u_set.push_back(u);
            for (auto& e : adj[u]) {
                double d = min_costs[u] + e.weight;
                if (d <= min_costs[e.to]) {
                    min_costs[e.to] = d;
                    if (d >= pulled.bound && d < B)
                        block_list.insert(e.to, d);
                    else if (d >= res.first && d < pulled.bound)
                        to_prepend.push_back({e.to, d});
                }
            }
        }
        block_list.batch_prepend(to_prepend);
    }

    for (int id : pivot_data.second)
        if (min_costs[id] < min_ub)
            u_set.push_back(id);
    return {min_ub, u_set};
}

vector<double> solve_sssp(int n, const vector<vector<Edge>>& adj, int start) {
    double logn = log2(n);
    int k = max(2, (int)pow(logn, 1.0 / 3.0));
    int t = max(1, (int)pow(logn, 2.0 / 3.0));
    int l = ceil(logn / t);

    vector<double> min_costs(n, numeric_limits<double>::infinity());
    min_costs[start] = 0;

    bmssp_bounded(l, numeric_limits<double>::infinity(), {start}, k, t, adj,
                  min_costs);
    return min_costs;
}
