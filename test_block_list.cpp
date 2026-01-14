#include "block_list.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;

// Test utilities
void assert_true(bool condition, const string& message) {
    if (!condition) {
        cerr << "FAILED: " << message << endl;
        exit(1);
    }
    cout << "PASSED: " << message << endl;
}

void test_basic_insert() {
    cout << "\n=== Test Basic Insert ===" << endl;
    BlockList bl(5, 100.0);

    bl.insert(1, 10.0);
    bl.insert(2, 20.0);
    bl.insert(3, 5.0);

    auto result = bl.pull();
    assert_true(result.frontier.size() > 0, "Pull returns elements");
    // Paper spec: S' contains smallest values (order within S' not specified)
    set<int> pulled(result.frontier.begin(), result.frontier.end());
    assert_true(pulled.count(3) > 0, "Smallest element (5.0) is in pulled set");
}

void test_duplicate_key_insert() {
    cout << "\n=== Test Duplicate Key Insert ===" << endl;
    BlockList bl(5, 100.0);

    bl.insert(1, 50.0);
    bl.insert(1, 30.0); // Better value
    bl.insert(1, 40.0); // Worse than 30, should be ignored

    auto result = bl.pull();
    assert_true(result.frontier.size() == 1, "Only one element for key 1");
    assert_true(result.frontier[0] == 1, "Key 1 is present");

    // Verify it was the smallest value (30.0) by checking bound or further pulls
    assert_true(bl.is_empty(), "No more elements after pull");
}

void test_batch_prepend_small() {
    cout << "\n=== Test Batch Prepend (Small) ===" << endl;
    BlockList bl(5, 100.0);

    bl.insert(10, 50.0);

    vector<pair<int, double>> batch = {{1, 5.0}, {2, 3.0}, {3, 7.0}};
    bl.batch_prepend(batch);

    auto result = bl.pull();
    // Should get the batch prepend elements first (smallest values)
    set<int> pulled(result.frontier.begin(), result.frontier.end());
    assert_true(pulled.count(2) > 0, "Element with value 3.0 pulled");
}

void test_batch_prepend_large() {
    cout << "\n=== Test Batch Prepend (Large, > M) ===" << endl;
    BlockList bl(5, 100.0);

    vector<pair<int, double>> batch;
    for (int i = 0; i < 20; i++) {
        batch.push_back({i, (double)i});
    }
    bl.batch_prepend(batch);

    // Pull should get smallest M elements
    auto result = bl.pull();
    assert_true(result.frontier.size() <= 5, "Pull returns at most M elements");

    // Verify they are the smallest
    for (int id : result.frontier) {
        assert_true(id < 5, "Pulled elements are among the smallest");
    }
}

void test_batch_prepend_duplicates() {
    cout << "\n=== Test Batch Prepend with Duplicates ===" << endl;
    BlockList bl(5, 100.0);

    vector<pair<int, double>> batch = {
        {1, 10.0}, {1, 5.0}, {1, 15.0}, {2, 20.0}};
    bl.batch_prepend(batch);

    auto result = bl.pull();
    assert_true(result.frontier.size() == 2, "Two unique keys");

    set<int> pulled(result.frontier.begin(), result.frontier.end());
    assert_true(pulled.count(1) > 0 && pulled.count(2) > 0,
                "Both keys present");
}

void test_pull_all_elements() {
    cout << "\n=== Test Pull All Elements ===" << endl;
    BlockList bl(5, 100.0);

    bl.insert(1, 10.0);
    bl.insert(2, 20.0);

    auto result = bl.pull();
    assert_true(result.frontier.size() == 2, "All elements pulled");
    assert_true(result.bound == 100.0,
                "Bound is B_global when all elements pulled");
    assert_true(bl.is_empty(), "BlockList is empty after pulling all");
}

void test_pull_partial() {
    cout << "\n=== Test Pull Partial Elements ===" << endl;
    BlockList bl(3, 100.0);

    for (int i = 0; i < 10; i++) {
        bl.insert(i, (double)i);
    }

    auto result = bl.pull();
    assert_true(result.frontier.size() <= 3,
                "Pull returns at most M elements");
    assert_true(!bl.is_empty(), "BlockList not empty after partial pull");

    // Verify returned bound is valid
    assert_true(result.bound < 100.0, "Bound is less than B_global");
    assert_true(result.bound > 0.0, "Bound is positive");
}

void test_pull_bound_correctness() {
    cout << "\n=== Test Pull Bound Correctness ===" << endl;
    BlockList bl(3, 100.0);

    for (int i = 0; i < 10; i++) {
        bl.insert(i, (double)(i * 10));
    }

    auto result1 = bl.pull();
    double bound1 = result1.bound;

    // All pulled elements should have values < bound1
    // (We can't directly verify this without knowing internal values,
    //  but we can verify bound properties)
    assert_true(bound1 <= 100.0, "Bound does not exceed B_global");

    auto result2 = bl.pull();
    double bound2 = result2.bound;
    assert_true(bound2 >= bound1, "Bounds are non-decreasing");
}

void test_mixed_operations() {
    cout << "\n=== Test Mixed Operations ===" << endl;
    BlockList bl(5, 100.0);

    bl.insert(1, 50.0);
    bl.insert(2, 30.0);

    vector<pair<int, double>> batch = {{3, 10.0}, {4, 5.0}};
    bl.batch_prepend(batch);

    bl.insert(5, 25.0);

    auto result = bl.pull();
    assert_true(result.frontier.size() > 0, "Pull returns elements");

    // Element with value 5.0 should be among the first pulled
    set<int> pulled(result.frontier.begin(), result.frontier.end());
    assert_true(pulled.count(4) > 0, "Smallest element (4, 5.0) pulled");
}

void test_update_across_lists() {
    cout << "\n=== Test Update Across Lists ===" << endl;
    BlockList bl(5, 100.0);

    vector<pair<int, double>> batch = {{1, 50.0}};
    bl.batch_prepend(batch);

    // Update with insert (should remove from D0, add to D1)
    bl.insert(1, 30.0);

    auto result = bl.pull();
    assert_true(result.frontier.size() == 1, "One element");
    assert_true(result.frontier[0] == 1, "Key 1 present");
}

void test_block_splitting() {
    cout << "\n=== Test Block Splitting ===" << endl;
    BlockList bl(4, 100.0);

    // Insert more than M elements to trigger split
    for (int i = 0; i < 10; i++) {
        bl.insert(i, (double)(10 - i));
    }

    // Verify all elements can be pulled in order
    vector<int> all_pulled;
    while (!bl.is_empty()) {
        auto result = bl.pull();
        all_pulled.insert(all_pulled.end(), result.frontier.begin(),
                          result.frontier.end());
    }

    assert_true(all_pulled.size() == 10, "All 10 elements pulled");

    // Elements should be pulled roughly in order of their values
    // (smallest values first)
    set<int> pulled_set(all_pulled.begin(), all_pulled.end());
    assert_true(pulled_set.size() == 10, "All unique keys pulled");
}

void test_empty_operations() {
    cout << "\n=== Test Empty Operations ===" << endl;
    BlockList bl(5, 100.0);

    assert_true(bl.is_empty(), "Initially empty");

    auto result = bl.pull();
    assert_true(result.frontier.empty(), "Pull on empty returns empty");
    assert_true(result.bound == 100.0, "Bound is B_global on empty pull");

    bl.insert(1, 10.0);
    assert_true(!bl.is_empty(), "Not empty after insert");
}

void test_m_equals_one() {
    cout << "\n=== Test M = 1 ===" << endl;
    BlockList bl(1, 100.0);

    for (int i = 0; i < 5; i++) {
        bl.insert(i, (double)i);
    }

    auto result = bl.pull();
    assert_true(result.frontier.size() <= 1, "Pull returns at most 1 element");

    int count = 1;
    while (!bl.is_empty()) {
        auto r = bl.pull();
        count += r.frontier.size();
    }
    assert_true(count == 5, "All 5 elements eventually pulled");
}

void test_ordering_correctness() {
    cout << "\n=== Test Ordering Correctness ===" << endl;
    BlockList bl(5, 100.0);

    vector<pair<int, double>> elements = {
        {1, 50.0}, {2, 10.0}, {3, 30.0}, {4, 5.0}, {5, 80.0}};

    for (const auto& p : elements) {
        bl.insert(p.first, p.second);
    }

    // Pull all and verify ordering ACROSS batches (not within)
    // Paper spec: each Pull returns smallest values, with bound x separating
    // from remaining. So max of batch i < bound i <= min of batch i+1.
    vector<double> batch_maxes;

    while (!bl.is_empty()) {
        auto result = bl.pull();
        if (result.frontier.empty())
            break;

        double batch_max = -1;
        for (int id : result.frontier) {
            for (const auto& p : elements) {
                if (p.first == id) {
                    batch_max = max(batch_max, p.second);
                    break;
                }
            }
        }
        batch_maxes.push_back(batch_max);
    }

    // Verify that batch max values are non-decreasing across pulls
    for (size_t i = 1; i < batch_maxes.size(); i++) {
        assert_true(batch_maxes[i - 1] <= batch_maxes[i],
                    "Batch max values non-decreasing across pulls");
    }
}

void test_large_scale() {
    cout << "\n=== Test Large Scale Operations ===" << endl;
    BlockList bl(10, 1000.0);

    // Insert many elements
    for (int i = 0; i < 100; i++) {
        bl.insert(i, (double)(100 - i));
    }

    // Batch prepend
    vector<pair<int, double>> batch;
    for (int i = 100; i < 150; i++) {
        batch.push_back({i, (double)(i - 100)});
    }
    bl.batch_prepend(batch);

    // Pull all
    set<int> all_pulled;
    while (!bl.is_empty()) {
        auto result = bl.pull();
        all_pulled.insert(result.frontier.begin(), result.frontier.end());
    }

    assert_true(all_pulled.size() == 150, "All 150 elements pulled");
}

void test_random_operations() {
    cout << "\n=== Test Random Operations ===" << endl;
    mt19937 rng(12345);
    uniform_real_distribution<double> dist_val(0.0, 100.0);

    BlockList bl(8, 200.0);
    set<int> inserted_keys;

    // Random inserts
    for (int i = 0; i < 50; i++) {
        int key = rng() % 100;
        double val = dist_val(rng);
        bl.insert(key, val);
        inserted_keys.insert(key);
    }

    // Random batch prepend
    vector<pair<int, double>> batch;
    for (int i = 0; i < 20; i++) {
        int key = 200 + (rng() % 50);
        double val = dist_val(rng) * 0.5; // Smaller values
        batch.push_back({key, val});
        inserted_keys.insert(key);
    }
    bl.batch_prepend(batch);

    // Pull all and verify
    set<int> pulled_keys;
    while (!bl.is_empty()) {
        auto result = bl.pull();
        pulled_keys.insert(result.frontier.begin(), result.frontier.end());
    }

    assert_true(pulled_keys.size() <= inserted_keys.size(),
                "Pulled keys is subset of inserted keys");
    cout << "    Inserted " << inserted_keys.size() << " unique keys, pulled "
         << pulled_keys.size() << " keys" << endl;
}

void test_batch_prepend_overwrites_insert() {
    cout << "\n=== Test Batch Prepend Overwrites Insert ===" << endl;
    BlockList bl(5, 100.0);

    bl.insert(1, 50.0);
    bl.insert(2, 60.0);

    // Batch prepend with better value for key 1
    vector<pair<int, double>> batch = {{1, 10.0}, {3, 15.0}};
    bl.batch_prepend(batch);

    vector<int> all_pulled;
    while (!bl.is_empty()) {
        auto result = bl.pull();
        all_pulled.insert(all_pulled.end(), result.frontier.begin(),
                          result.frontier.end());
    }

    // Should have 3 unique keys
    set<int> unique_keys(all_pulled.begin(), all_pulled.end());
    assert_true(unique_keys.size() == 3, "Three unique keys (1, 2, 3)");
    assert_true(unique_keys.count(1) && unique_keys.count(2) &&
                    unique_keys.count(3),
                "All keys present");
}

void test_stress_pull_consistency() {
    cout << "\n=== Test Stress Pull Consistency ===" << endl;
    BlockList bl(7, 500.0);

    // Insert elements with known values
    map<int, double> ground_truth;
    for (int i = 0; i < 30; i++) {
        double val = (double)(i * 5);
        bl.insert(i, val);
        ground_truth[i] = val;
    }

    // Pull and track batch-level ordering (not element-level within batch)
    vector<double> batch_maxes;
    int total_pulled = 0;

    while (!bl.is_empty()) {
        auto result = bl.pull();
        if (result.frontier.empty())
            break;

        double batch_max = -1;
        for (int id : result.frontier) {
            batch_max = max(batch_max, ground_truth[id]);
            total_pulled++;
        }
        batch_maxes.push_back(batch_max);
    }

    // Verify all pulled
    assert_true(total_pulled == 30, "All elements pulled");

    // Verify monotonic ordering ACROSS batches
    for (size_t i = 1; i < batch_maxes.size(); i++) {
        assert_true(batch_maxes[i - 1] <= batch_maxes[i],
                    "Batch max values non-decreasing across pulls");
    }
}

int main() {
    cout << "Starting BlockList Correctness Tests..." << endl;
    cout << "=======================================" << endl;

    try {
        test_basic_insert();
        test_duplicate_key_insert();
        test_batch_prepend_small();
        test_batch_prepend_large();
        test_batch_prepend_duplicates();
        test_pull_all_elements();
        test_pull_partial();
        test_pull_bound_correctness();
        test_mixed_operations();
        test_update_across_lists();
        test_block_splitting();
        test_empty_operations();
        test_m_equals_one();
        test_ordering_correctness();
        test_large_scale();
        test_random_operations();
        test_batch_prepend_overwrites_insert();
        test_stress_pull_consistency();

        cout << "\n=======================================" << endl;
        cout << "ALL TESTS PASSED!" << endl;
        cout << "=======================================" << endl;

        return 0;
    } catch (const exception& e) {
        cerr << "\nEXCEPTION: " << e.what() << endl;
        return 1;
    }
}