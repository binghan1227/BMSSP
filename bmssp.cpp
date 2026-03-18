#include "bmssp.h"
#include "block_list.h"
#include "trace.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_set>
#include <vector>

using namespace std;

struct WorkArrays {
    vector<int> bp_map;    // BFS parent, -1 = unset
    vector<int> tree_size; // tree size accumulator, 0 = unset
    vector<int> bp_dirty;  // indices written to bp_map

    WorkArrays(int n) : bp_map(n, -1), tree_size(n, 0) {}

    void reset_bp() {
        for (int i : bp_dirty)
            bp_map[i] = -1;
        bp_dirty.clear();
    }
};

static pair<vector<int>, vector<int>>
find_pivots(double bound, const vector<int>& frontier, int k,
            const vector<vector<Edge>>& adj, vector<double>& min_costs,
            WorkArrays& work) {
    vector<int> all_layers = frontier;
    vector<int> last_layer = frontier;

    for (int i = 0; i < k; ++i) {
        vector<int> new_layer;
        for (int u : last_layer) {
            for (auto& e : adj[u]) {
                double d = min_costs[u] + e.weight;
                if (d <= min_costs[e.to]) {
                    min_costs[e.to] = d;
                    if (d < bound) {
                        new_layer.push_back(e.to);
                        work.bp_map[e.to] = u;
                        work.bp_dirty.push_back(e.to);
                    }
                }
            }
        }
        all_layers.insert(all_layers.end(), new_layer.begin(), new_layer.end());
        last_layer = new_layer;
        if (all_layers.size() > (size_t)k * frontier.size()) {
            work.reset_bp();
            return {frontier, all_layers};
        }
    }

    // Trace from leaves to roots, accumulate tree sizes
    unordered_set<int> roots;
    for (int leaf : last_layer) {
        int cur = leaf;
        int count = 0;
        while (work.bp_map[cur] != -1) {
            cur = work.bp_map[cur];
            count++;
        }
        work.tree_size[cur] += count;
        roots.insert(cur);
    }

    // Collect pivots (roots with large enough trees)
    vector<int> pivots;
    for (int r : roots) {
        if (work.tree_size[r] >= k)
            pivots.push_back(r);
        work.tree_size[r] = 0; // reset inline
    }

    work.reset_bp();

    // Build pairs with distances for trace output
    std::vector<std::pair<int, double>> all_layers_with_dist;
    for (int id : all_layers) {
        all_layers_with_dist.push_back({id, min_costs[id]});
    }
    TRACE("FIND_PIVOTS",
          TF("pivots", vec_json(pivots))
              TF("all_layers", pairs_json(all_layers_with_dist)));
    return {pivots, all_layers};
}

static pair<double, vector<int>>
base_bmssp(double B, int node_id, int base_limit,
           const vector<vector<Edge>>& adj, vector<double>& min_costs) {
    TRACE("BASE_CASE", TF("node", node_id) TF("B", B));
    priority_queue<State, vector<State>, greater<State>> pq;
    pq.push({node_id, min_costs[node_id]});
    vector<int> u_init;
    double max_cost = min_costs[node_id];

    while (!pq.empty() && (int)u_init.size() < base_limit) {
        State top = pq.top();
        pq.pop();
        // Lazy deletion: skip stale entries (replaces visited set)
        if (top.cost > min_costs[top.node_id])
            continue;
        TRACE("BASE_PQ_POP", TF("node", top.node_id) TF("cost", top.cost));
        u_init.push_back(top.node_id);
        max_cost = max(max_cost, top.cost);

        for (auto& e : adj[top.node_id]) {
            double d = top.cost + e.weight;
            if (d <= min_costs[e.to] && d < B) {
                min_costs[e.to] = d;
                TRACE("BASE_RELAX",
                      TF("from", top.node_id) TF("to", e.to) TF("cost", d));
                pq.push({e.to, d});
            }
        }
    }
    if ((int)u_init.size() < base_limit)
        return {B, u_init};
    vector<int> filtered_u;
    for (int id : u_init)
        if (min_costs[id] < max_cost)
            filtered_u.push_back(id);
    return {max_cost, filtered_u};
}

static pair<double, vector<int>>
bmssp_bounded(int l, double B, const vector<int>& frontier, int k, int t,
              int n, int base_limit, const vector<vector<Edge>>& adj,
              vector<double>& min_costs, WorkArrays& work,
              bool is_top = false) {
    TRACE("RECURSION_ENTER",
          TF("l", l) TF("B", B) TF("frontier", vec_json(frontier)));

    if (frontier.empty())
        return {B, {}};
    // Opt 1: Early base-case fallback for single-node frontiers at non-top
    // levels. Avoids find_pivots + BlockList overhead when the parent loop
    // will continue the expansion.
    if (l == 0 || (!is_top && frontier.size() <= 1))
        return base_bmssp(B, frontier[0], base_limit, adj, min_costs);

    auto pivot_data = find_pivots(B, frontier, k, adj, min_costs, work);

    // Opt 6: Integer arithmetic instead of floating-point pow
    int shift = t * (l - 1);
    int M = (shift >= 30) ? (1 << 30) : (1 << shift);
    BlockList block_list(M, B);
    double min_ub = B;

    std::vector<std::pair<int, double>> pivot_inserts;
    for (int p : pivot_data.first) {
        block_list.insert(p, min_costs[p]);
        min_ub = min(min_ub, min_costs[p]);
        pivot_inserts.push_back({p, min_costs[p]});
    }
    if (!pivot_inserts.empty())
        TRACE("BL_INSERT", TF("elements", pairs_json(pivot_inserts)));

    vector<int> u_set;
    int shift_u = t * l;
    size_t max_u = (shift_u >= 60) ? (size_t)k << 60
                                   : (size_t)k << shift_u;

    while (u_set.size() < max_u && !block_list.is_empty()) {
        auto pulled = block_list.pull();
        TRACE("BL_PULL",
              TF("nodes", vec_json(pulled.frontier)) TF("bound", pulled.bound));
        auto res = bmssp_bounded(l - 1, pulled.bound, pulled.frontier, k, t, n,
                                 base_limit, adj, min_costs, work);
        min_ub = res.first;

        vector<pair<int, double>> to_prepend;
        vector<pair<int, double>> d1_inserts;
        for (int u : res.second) {
            u_set.push_back(u);
            for (auto& e : adj[u]) {
                double d = min_costs[u] + e.weight;
                if (d <= min_costs[e.to]) {
                    min_costs[e.to] = d;
                    if (d >= pulled.bound && d < B) {
                        block_list.insert(e.to, d);
                        d1_inserts.push_back({e.to, d});
                    } else if (d >= res.first && d < pulled.bound)
                        to_prepend.push_back({e.to, d});
                }
            }
        }
        if (!d1_inserts.empty())
            TRACE("BL_INSERT", TF("elements", pairs_json(d1_inserts)));
        block_list.batch_prepend(to_prepend);
        TRACE("BL_PREPEND", TF("elements", pairs_json(to_prepend)));
    }

    for (int id : pivot_data.second)
        if (min_costs[id] < min_ub)
            u_set.push_back(id);
    TRACE("RECURSION_EXIT",
          TF("l", l) TF("min_ub", min_ub) TF("u_set", vec_json(u_set)));
    return {min_ub, u_set};
}

vector<double> solve_sssp(int n, const vector<vector<Edge>>& adj, int start) {
    double logn = log2(n);
    int k = max(2, (int)pow(logn, 1.0 / 3.0));
    int t = max(1, (int)pow(logn, 2.0 / 3.0));
    int l = ceil(logn / t);
    TRACE("SOLVE_START",
          TF("n", n) TF("k", k) TF("t", t) TF("l", l) TF("source", start));

    vector<double> min_costs(n, numeric_limits<double>::infinity());
    min_costs[start] = 0;

    // Opt 2: Pre-allocate flat arrays for find_pivots
    WorkArrays work(n);

    // Opt 5: Enlarged base case limit
    int base_limit = max(k + 1, 1 << t);

    bmssp_bounded(l, numeric_limits<double>::infinity(), {start}, k, t, n,
                  base_limit, adj, min_costs, work, /*is_top=*/true);
    return min_costs;
}
