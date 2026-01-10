#include "block_list.h"

BlockList::BlockList(int m_val, double b_val) : M(m_val), upper_bound(b_val) {}

void BlockList::insert(int u, double d) {
    if (d < upper_bound && (dists.find(u) == dists.end() || d < dists[u])) {
        dists[u] = d;
        pq.push({u, d});
    }
}

void BlockList::batch_prepend(const vector<pair<int, double>>& elements) {
    for (auto& p : elements)
        insert(p.first, p.second);
}

BlockList::PullResult BlockList::pull() {
    vector<int> frontier;
    while (!pq.empty() && frontier.size() < (size_t)M) {
        State top = pq.top();
        pq.pop();
        if (top.cost > dists[top.node_id])
            continue;
        frontier.push_back(top.node_id);
        dists.erase(top.node_id);
    }
    double next_b = pq.empty() ? upper_bound : pq.top().cost;
    return {frontier, next_b};
}

bool BlockList::is_empty() {
    while (!pq.empty() && (dists.find(pq.top().node_id) == dists.end() ||
                           pq.top().cost > dists[pq.top().node_id])) {
        pq.pop();
    }
    return pq.empty();
}
