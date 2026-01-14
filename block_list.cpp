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

    sort(to_add.begin(), to_add.end(),
         [](const Element& a, const Element& b) { return a.d < b.d; });

    int block_size = (M + 1) / 2;
    int n = (int)to_add.size();
    for (int i = n; i > 0;) {
        int start = max(0, i - block_size);
        int end = i;
        Block b;
        b.upper_bound = 0;
        for (int k = start; k < end; ++k) {
            b.elements.push_back(to_add[k]);
        }

        D0.push_front(b);
        auto it = D0.begin();
        for (auto el_it = it->elements.begin(); el_it != it->elements.end();
             ++el_it) {
            locator[el_it->u] = {LIST_D0, it, el_it, el_it->d};
        }
        i = start;
    }
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
        // Pull everything; bound = B
        for (const auto& p : candidates)
            frontier_ids.push_back(p.second);
        next_bound = B_global;
    } else {
        // Linear-time selection of the M-th order statistic
        nth_element(candidates.begin(), candidates.begin() + M,
                    candidates.end(),
                    [](const pair<double, int>& a, const pair<double, int>& b) {
                        return a.first < b.first;
                    });
        double dM = candidates[M].first;
        next_bound = dM;

        // Select strictly smaller than dM (to ensure strict inequality)
        for (const auto& p : candidates) {
            if ((int)frontier_ids.size() >= M)
                break;
            if (p.first < dM)
                frontier_ids.push_back(p.second);
        }

        // Fallback: ensure progress by taking at least one if all are ties
        if (frontier_ids.empty()) {
            auto it = min_element(
                candidates.begin(), candidates.end(),
                [](const pair<double, int>& a, const pair<double, int>& b) {
                    return a.first < b.first;
                });
            frontier_ids.push_back(it->second);
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

    // If empty after removal, bound should be B
    if (locator.empty())
        next_bound = B_global;

    return {frontier_ids, next_bound};
}

bool BlockList::is_empty() { return locator.empty(); }
