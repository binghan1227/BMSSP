#ifndef BMSSP_H
#define BMSSP_H

#include "types.h"
#include <vector>

using namespace std;

// Solves Single-Source Shortest Path using the BMSSP algorithm
vector<double> solve_sssp(int n, const vector<vector<Edge>>& adj, int start);

#endif // BMSSP_H
