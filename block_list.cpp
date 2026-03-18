#include "block_list.h"
#include <algorithm>
#include <climits>
#include <limits>

using namespace std;

BlockList::BlockList(int m_val, double b_val) : M(m_val), B_global(b_val) {
    if (M < 1)
        M = 1;
    locator.reserve(M * 2);
    Block b;
    b.upper_bound = B_global;
    b.id = next_block_id++;
    D1.push_back(b);
    D1_Index.insert({{b.upper_bound, b.id}, D1.begin()});
}

void BlockList::erase_element(list<Block>::iterator block_it, int elem_idx) {
    auto& elems = block_it->elements;
    int last = (int)elems.size() - 1;
    if (elem_idx != last) {
        int swapped_u = elems[last].u;
        locator[swapped_u].elem_idx = elem_idx;
        swap(elems[elem_idx], elems[last]);
    }
    elems.pop_back();
}

void BlockList::insert(int u, double d) {
    auto loc_it = locator.find(u);
    if (loc_it != locator.end()) {
        if (d >= loc_it->second.dist)
            return;

        auto& info = loc_it->second;
        erase_element(info.block_it, info.elem_idx);

        if (info.block_it->elements.empty()) {
            if (info.type == LIST_D1) {
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
    int idx = (int)target_block->elements.size() - 1;
    locator[u] = {target_block, d, idx, LIST_D1};

    if ((int)target_block->elements.size() > M) {
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
    nth_element(els.begin(), els.begin() + mid, els.end(),
                [](const Element& a, const Element& b) { return a.d < b.d; });

    double left_max = els[0].d;
    for (int i = 1; i < mid; ++i)
        left_max = max(left_max, els[i].d);

    double old_ub = block_it->upper_bound;
    int old_id = block_it->id;

    D1_Index.erase({old_ub, old_id});

    block_it->elements.clear();
    block_it->upper_bound = left_max;
    for (int i = 0; i < mid; ++i) {
        block_it->elements.push_back(els[i]);
        locator[els[i].u] = {block_it, els[i].d, i, LIST_D1};
    }
    D1_Index.insert({{block_it->upper_bound, block_it->id}, block_it});

    Block new_b;
    new_b.upper_bound = old_ub;
    new_b.id = next_block_id++;
    for (int i = mid; i < n; ++i) {
        new_b.elements.push_back(els[i]);
    }

    auto new_block_it = D1.insert(next(block_it), new_b);
    D1_Index.insert(
        {{new_block_it->upper_bound, new_block_it->id}, new_block_it});

    for (int i = 0; i < (int)new_block_it->elements.size(); ++i) {
        auto& el = new_block_it->elements[i];
        locator[el.u] = {new_block_it, el.d, i, LIST_D1};
    }
}

void BlockList::partition_into_blocks_d0(vector<Element>& arr, int start,
                                          int end, list<Block>& blocks) {
    int size = end - start;
    int threshold = (M + 1) / 2; // ceil(M/2)

    if (size <= threshold) {
        Block b;
        b.upper_bound = 0;
        b.id = 0;
        for (int i = start; i < end; ++i) {
            b.elements.push_back(arr[i]);
        }
        blocks.push_back(b);
        return;
    }

    int mid = start + size / 2;
    nth_element(arr.begin() + start, arr.begin() + mid, arr.begin() + end,
                [](const Element& a, const Element& b) { return a.d < b.d; });

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
            erase_element(info.block_it, info.elem_idx);
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
        Block b;
        b.upper_bound = 0;
        for (const auto& el : to_add) {
            b.elements.push_back(el);
        }
        D0.push_front(b);
        auto it = D0.begin();
        for (int i = 0; i < (int)it->elements.size(); ++i) {
            auto& el = it->elements[i];
            locator[el.u] = {it, el.d, i, LIST_D0};
        }
        return;
    }

    // Use recursive median partitioning
    list<Block> new_blocks;
    partition_into_blocks_d0(to_add, 0, (int)to_add.size(), new_blocks);

    for (auto block_it = new_blocks.begin(); block_it != new_blocks.end();
         ++block_it) {
        for (int i = 0; i < (int)block_it->elements.size(); ++i) {
            auto& el = block_it->elements[i];
            locator[el.u] = {block_it, el.d, i, LIST_D0};
        }
    }

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
        for (const auto& p : candidates)
            frontier_ids.push_back(p.second);
    } else {
        nth_element(candidates.begin(), candidates.begin() + M,
                    candidates.end(),
                    [](const pair<double, int>& a, const pair<double, int>& b) {
                        return a.first < b.first;
                    });
        double dM = candidates[M].first;

        for (int i = 0; i < M; ++i) {
            if (candidates[i].first < dM)
                frontier_ids.push_back(candidates[i].second);
        }

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
            erase_element(info.block_it, info.elem_idx);

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
        for (auto& block : D0) {
            if (!block.elements.empty()) {
                for (auto& el : block.elements) {
                    next_bound = min(next_bound, el.d);
                }
                break;
            }
        }
        for (auto& block : D1) {
            if (!block.elements.empty()) {
                for (auto& el : block.elements) {
                    next_bound = min(next_bound, el.d);
                }
                break;
            }
        }
    }

    return {frontier_ids, next_bound};
}

bool BlockList::is_empty() { return locator.empty(); }
