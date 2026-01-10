#ifndef TYPES_H
#define TYPES_H

struct State {
    int node_id;
    double cost;

    bool operator>(const State& other) const {
        if (cost != other.cost)
            return cost > other.cost;
        return node_id > other.node_id;
    }
};

struct Edge {
    int to;
    double weight;
};

#endif // TYPES_H
