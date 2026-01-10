#ifndef BLOCK_LIST_H
#define BLOCK_LIST_H

#include "types.h"
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

using namespace std;

struct BlockList {
    int M;
    double upper_bound;
    priority_queue<State, vector<State>, greater<State>> pq;
    unordered_map<int, double> dists;

    BlockList(int m_val, double b_val);

    void insert(int u, double d);

    void batch_prepend(const vector<pair<int, double>>& elements);

    struct PullResult {
        vector<int> frontier;
        double bound;
    };

    PullResult pull();

    bool is_empty();
};

#endif // BLOCK_LIST_H
