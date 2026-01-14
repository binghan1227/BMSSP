#include "block_list.h"
#include <algorithm>
#include <climits>
#include <limits>

using namespace std;

BlockList::BlockList(int m_val, double b_val) : M(m_val), B_global(b_val) {
    if (M < 1)
        M = 1;
    Block b;
    b.upper_bound = B_global;
    b.id = next_block_id++;
    D1.push_back(b);
    D1_Index.insert({{b.upper_bound, b.id}, D1.begin()});
}

void BlockList::insert(int u, double d) {
    auto loc_it = locator.find(u);
    if (loc_it != locator.end()) {
        if (d >= loc_it->second.dist)
            return;

        // Remove existing
        auto& info = loc_it->second;
        info.block_it->elements.erase(info.elem_it);

        if (info.block_it->elements.empty()) {
            if (info.type == LIST_D1) {
                // Remove from Index
                D1_Index.erase({info.block_it->upper_bound, info.block_it->id});
                D1.erase(info.block_it);
            } else {
                D0.erase(info.block_it);
            }
        }
        locator.erase(loc_it);
    }

    if (D1.empty()) {
        Block b;
        b.upper_bound = B_global;
        b.id = next_block_id++;
        D1.push_back(b);
        D1_Index.insert({{b.upper_bound, b.id}, D1.begin()});
    }

    auto idx_it = D1_Index.lower_bound({d, INT_MIN});
    list<Block>::iterator target_block;

    if (idx_it == D1_Index.end()) {
        target_block = --D1.end();
    } else {
        target_block = idx_it->second;
    }

    target_block->elements.push_back({u, d});
    locator[u] = {LIST_D1, target_block, --target_block->elements.end(), d};

    if (target_block->elements.size() > M) {
        split_block_d1(target_block);
    }
}

void BlockList::split_block_d1(list<Block>::iterator block_it) {
    int n = block_it->elements.size();
    vector<Element> els;
    els.reserve(n);
    for (const auto& el : block_it->elements)
        els.push_back(el);

    int mid = n / 2;
    // Partition around the median position (not fully sorted)
    nth_element(els.begin(), els.begin() + mid, els.end(),
                [](const Element& a, const Element& b) { return a.d < b.d; });

    // Compute upper bound for the left block as the max of left partition
    double left_max = els[0].d;
    for (int i = 1; i < mid; ++i)
        left_max = max(left_max, els[i].d);

    double old_ub = block_it->upper_bound;
    int old_id = block_it->id;

    // Remove old index entry
    D1_Index.erase({old_ub, old_id});

    // Update first block (reused)
    block_it->elements.clear();
    block_it->upper_bound = left_max;
    for (int i = 0; i < mid; ++i) {
        block_it->elements.push_back(els[i]);
        locator[els[i].u] = {LIST_D1, block_it, --block_it->elements.end(),
                             els[i].d};
    }
    D1_Index.insert({{block_it->upper_bound, block_it->id}, block_it});

    // Create second block
    Block new_b;
    new_b.upper_bound = old_ub;
    new_b.id = next_block_id++;
    for (int i = mid; i < n; ++i) {
        new_b.elements.push_back(els[i]);
    }

    auto new_block_it = D1.insert(next(block_it), new_b);
    D1_Index.insert(
        {{new_block_it->upper_bound, new_block_it->id}, new_block_it});

    for (auto it = new_block_it->elements.begin();
         it != new_block_it->elements.end(); ++it) {
        locator[it->u] = {LIST_D1, new_block_it, it, it->d};
    }
}

void BlockList::partition_into_blocks_d0(vector<Element>& arr, int start,
                                          int end, list<Block>& blocks) {
    int size = end - start;
    int threshold = (M + 1) / 2; // ceil(M/2)

    if (size <= threshold) {
        // Base case: create a single block
        Block b;
        b.upper_bound = 0;
        b.id = 0; // D0 blocks don't use BST index
        for (int i = start; i < end; ++i) {
            b.elements.push_back(arr[i]);
        }
        blocks.push_back(b);
        return;
    }

    // Recursive case: partition around median
    int mid = start + size / 2;
    nth_element(arr.begin() + start, arr.begin() + mid, arr.begin() + end,
                [](const Element& a, const Element& b) { return a.d < b.d; });

    // Recurse on left (smaller values) first, then right (larger values)
    // This ensures blocks are appended in ascending value order
    partition_into_blocks_d0(arr, start, mid, blocks);
    partition_into_blocks_d0(arr, mid, end, blocks);
}

void BlockList::batch_prepend(const vector<pair<int, double>>& elements) {
    unordered_map<int, double> best;
    best.reserve(elements.size());
    for (const auto& p : elements) {
        auto it = best.find(p.first);
        if (it == best.end() || p.second < it->second)
            best[p.first] = p.second;
    }

    vector<Element> to_add;
    to_add.reserve(best.size());

    for (const auto& kv : best) {
        Element el{kv.first, kv.second};
        auto loc_it = locator.find(el.u);
        if (loc_it != locator.end()) {
            if (el.d >= loc_it->second.dist)
                continue;
            auto& info = loc_it->second;
            info.block_it->elements.erase(info.elem_it);
            if (info.block_it->elements.empty()) {
                if (info.type == LIST_D1) {
                    D1_Index.erase(
                        {info.block_it->upper_bound, info.block_it->id});
                    D1.erase(info.block_it);
                } else {
                    D0.erase(info.block_it);
                }
            }
            locator.erase(loc_it);
        }
        to_add.push_back(el);
    }

    if (to_add.empty())
        return;

    if ((int)to_add.size() <= M) {
        // Single block, O(L)
        Block b;
        b.upper_bound = 0;
        for (const auto& el : to_add) {
            b.elements.push_back(el);
        }
        D0.push_front(b);
        auto it = D0.begin();
        for (auto el_it = it->elements.begin(); el_it != it->elements.end();
             ++el_it) {
            locator[el_it->u] = {LIST_D0, it, el_it, el_it->d};
        }
        return;
    }

    // Use recursive median partitioning: O(L log(L/M)) instead of O(L log L)
    list<Block> new_blocks;
    partition_into_blocks_d0(to_add, 0, (int)to_add.size(), new_blocks);

    // Set up locators while blocks are in new_blocks
    // (iterators remain valid after splice)
    for (auto block_it = new_blocks.begin(); block_it != new_blocks.end();
         ++block_it) {
        for (auto el_it = block_it->elements.begin();
             el_it != block_it->elements.end(); ++el_it) {
            locator[el_it->u] = {LIST_D0, block_it, el_it, el_it->d};
        }
    }

    // Splice all new blocks to front of D0
    D0.splice(D0.begin(), new_blocks);
}

BlockList::PullResult BlockList::pull() {
    vector<int> frontier_ids;
    double next_bound = std::numeric_limits<double>::infinity();

    vector<pair<double, int>> candidates;

    int collected_d0 = 0;
    for (auto bit = D0.begin(); bit != D0.end(); ++bit) {
        for (auto& el : bit->elements) {
            candidates.push_back({el.d, el.u});
            collected_d0++;
        }
        if (collected_d0 >= M)
            break;
    }

    int collected_d1 = 0;
    for (auto bit = D1.begin(); bit != D1.end(); ++bit) {
        for (auto& el : bit->elements) {
            candidates.push_back({el.d, el.u});
            collected_d1++;
        }
        if (collected_d1 >= M)
            break;
    }

    if (candidates.empty())
        return {{}, B_global};

    int K = (int)candidates.size();
    if (K <= M) {
        // Pull everything collected
        for (const auto& p : candidates)
            frontier_ids.push_back(p.second);
    } else {
        // Linear-time selection of the M-th order statistic
        nth_element(candidates.begin(), candidates.begin() + M,
                    candidates.end(),
                    [](const pair<double, int>& a, const pair<double, int>& b) {
                        return a.first < b.first;
                    });
        double dM = candidates[M].first;

        // Select elements with value < dM first (ensures max(S') < x = dM)
        for (int i = 0; i < M; ++i) {
            if (candidates[i].first < dM)
                frontier_ids.push_back(candidates[i].second);
        }

        // If all candidates[0..M-1] tie at dM, return all of them to ensure progress.
        if (frontier_ids.empty()) {
            for (int i = 0; i < M; ++i) {
                frontier_ids.push_back(candidates[i].second);
            }
        }
    }

    // Erase selected elements from the structure
    for (int u : frontier_ids) {
        auto loc_it = locator.find(u);
        if (loc_it != locator.end()) {
            auto& info = loc_it->second;
            info.block_it->elements.erase(info.elem_it);

            if (info.block_it->elements.empty()) {
                if (info.type == LIST_D1) {
                    D1_Index.erase(
                        {info.block_it->upper_bound, info.block_it->id});
                    D1.erase(info.block_it);
                } else {
                    D0.erase(info.block_it);
                }
            }
            locator.erase(loc_it);
        }
    }

    // Compute the actual minimum remaining value in D0 ∪ D1
    if (locator.empty()) {
        next_bound = B_global;
    } else {
        next_bound = B_global;
        // Check first non-empty block of D0
        for (auto& block : D0) {
            if (!block.elements.empty()) {
                for (auto& el : block.elements) {
                    next_bound = min(next_bound, el.d);
                }
                break; // Only need first non-empty block (blocks are sorted)
            }
        }
        // Check first non-empty block of D1
        for (auto& block : D1) {
            if (!block.elements.empty()) {
                for (auto& el : block.elements) {
                    next_bound = min(next_bound, el.d);
                }
                break; // Only need first non-empty block (blocks are sorted)
            }
        }
    }

    return {frontier_ids, next_bound};
}

bool BlockList::is_empty() { return locator.empty(); }
