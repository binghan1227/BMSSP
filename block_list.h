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
        vector<Element> elements;
        double upper_bound;
        int id;
    };

    enum ListType { LIST_D0, LIST_D1 };

    struct LocatorInfo {
        list<Block>::iterator block_it;
        double dist;
        int elem_idx;
        ListType type;
    };

    list<Block> D0; // Batch prepends
    list<Block> D1; // Inserts

    // Index for D1: map (upper_bound, id) -> block iterator.
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
    void partition_into_blocks_d0(vector<Element>& arr, int start, int end,
                                   list<Block>& blocks);
    void erase_element(list<Block>::iterator block_it, int elem_idx);
};

#endif // BLOCK_LIST_H
