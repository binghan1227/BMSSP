#include "block_list.h"
#include <algorithm>
#include <limits>

using namespace std;

BlockList::BlockList(int m_val, double b_val) : M(m_val) {
    if (M < 1)
        M = 1;
    Block b;
    b.upper_bound = b_val;
    D1.push_back(b);
    D1_Index.insert({b_val, D1.begin()});
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
                auto range = D1_Index.equal_range(info.block_it->upper_bound);
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second == info.block_it) {
                        D1_Index.erase(it);
                        break;
                    }
                }
                D1.erase(info.block_it);
            } else {
                D0.erase(info.block_it);
            }
        }
        locator.erase(loc_it);
    }

    if (D1.empty()) {
        Block b;
        b.upper_bound = std::numeric_limits<double>::infinity();
        D1.push_back(b);
        D1_Index.insert({b.upper_bound, D1.begin()});
    }

    auto idx_it = D1_Index.lower_bound(d);
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

    // TODO: Need a O(M) time algorithm
    sort(els.begin(), els.end(),
         [](const Element& a, const Element& b) { return a.d < b.d; });

    int mid = n / 2;
    double split_ub = els[mid - 1].d;
    double old_ub = block_it->upper_bound;

    // Remove old index entry
    auto range = D1_Index.equal_range(old_ub);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == block_it) {
            D1_Index.erase(it);
            break;
        }
    }

    // Update first block (reused)
    block_it->elements.clear();
    block_it->upper_bound = split_ub;
    for (int i = 0; i < mid; ++i) {
        block_it->elements.push_back(els[i]);
        locator[els[i].u] = {LIST_D1, block_it, --block_it->elements.end(),
                             els[i].d};
    }
    D1_Index.insert({split_ub, block_it});

    // Create second block
    Block new_b;
    new_b.upper_bound = old_ub;
    for (int i = mid; i < n; ++i) {
        new_b.elements.push_back(els[i]);
    }

    auto new_block_it = D1.insert(next(block_it), new_b);
    D1_Index.insert({old_ub, new_block_it});

    for (auto it = new_block_it->elements.begin();
         it != new_block_it->elements.end(); ++it) {
        locator[it->u] = {LIST_D1, new_block_it, it, it->d};
    }
}

void BlockList::batch_prepend(const vector<pair<int, double>>& elements) {
    vector<Element> sorted_els;
    sorted_els.reserve(elements.size());
    for (auto& p : elements)
        sorted_els.push_back({p.first, p.second});
    sort(sorted_els.begin(), sorted_els.end(),
         [](const Element& a, const Element& b) { return a.d < b.d; });

    vector<Element> to_add;
    to_add.reserve(sorted_els.size());

    for (const auto& el : sorted_els) {
        auto loc_it = locator.find(el.u);
        if (loc_it != locator.end()) {
            if (el.d >= loc_it->second.dist)
                continue;
            auto& info = loc_it->second;
            info.block_it->elements.erase(info.elem_it);
            if (info.block_it->elements.empty()) {
                if (info.type == LIST_D1) {
                    auto range =
                        D1_Index.equal_range(info.block_it->upper_bound);
                    for (auto it = range.first; it != range.second; ++it) {
                        if (it->second == info.block_it) {
                            D1_Index.erase(it);
                            break;
                        }
                    }
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

    int block_size = (M + 1) / 2;
    int n = to_add.size();
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
        return {{}, next_bound};

    sort(candidates.begin(), candidates.end());

    int count = min((int)candidates.size(), M);
    for (int i = 0; i < count; ++i) {
        frontier_ids.push_back(candidates[i].second);
    }

    if (candidates.size() > M) {
        next_bound = candidates[M].first;
    } else {
        if (count < candidates.size())
            next_bound = candidates[count].first;
        // else next_bound = infinity
    }

    for (int u : frontier_ids) {
        auto loc_it = locator.find(u);
        if (loc_it != locator.end()) {
            auto& info = loc_it->second;
            info.block_it->elements.erase(info.elem_it);

            if (info.block_it->elements.empty()) {
                if (info.type == LIST_D1) {
                    auto range =
                        D1_Index.equal_range(info.block_it->upper_bound);
                    for (auto it = range.first; it != range.second; ++it) {
                        if (it->second == info.block_it) {
                            D1_Index.erase(it);
                            break;
                        }
                    }
                    D1.erase(info.block_it);
                } else {
                    D0.erase(info.block_it);
                }
            }
            locator.erase(loc_it);
        }
    }

    return {frontier_ids, next_bound};
}

bool BlockList::is_empty() { return locator.empty(); }
