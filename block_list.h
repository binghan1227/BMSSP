#ifndef BLOCK_LIST_H
#define BLOCK_LIST_H

#include "types.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

using namespace std;

struct BlockList {
    int M;
    double B_global;

    struct Element {
        int u;
        double d;
    };

    struct Block {
        list<Element> elements;
        double upper_bound;
        int id;
    };

    enum ListType { LIST_D0, LIST_D1 };

    struct LocatorInfo {
        ListType type;
        list<Block>::iterator block_it;
        list<Element>::iterator elem_it;
        double dist;
    };

    list<Block> D0; // Batch prepends
    list<Block> D1; // Inserts

    // Index for D1: map (upper_bound, id) -> block iterator.
    // Used to find the block with smallest upper_bound >= d deterministically.
    map<pair<double, int>, list<Block>::iterator> D1_Index;

    int next_block_id = 0;

    unordered_map<int, LocatorInfo> locator;

    BlockList(int m_val, double b_val);

    void insert(int u, double d);

    void batch_prepend(const vector<pair<int, double>>& elements);

    struct PullResult {
        vector<int> frontier;
        double bound;
    };

    PullResult pull();

    bool is_empty();

  private:
    void split_block_d1(list<Block>::iterator block_it);
};

#endif // BLOCK_LIST_H
