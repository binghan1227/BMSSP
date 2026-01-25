#ifndef BMSSP_TRACE_H
#define BMSSP_TRACE_H

#ifdef BMSSP_TRACE
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>

inline std::ofstream& trace_stream() {
    static std::ofstream f("bmssp_trace.jsonl");
    return f;
}
inline int& trace_seq() { static int s = 0; return s; }

// Format vector of ints as JSON array (full list)
inline std::string vec_json(const std::vector<int>& v) {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) ss << ",";
        ss << v[i];
    }
    ss << "]";
    return ss.str();
}

// Format vector of (node, dist) pairs as JSON array
inline std::string pairs_json(const std::vector<std::pair<int,double>>& v) {
    std::ostringstream ss; ss << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) ss << ",";
        ss << "{\"n\":" << v[i].first << ",\"d\":" << v[i].second << "}";
    }
    ss << "]"; return ss.str();
}

#define TRACE(ev, ...) trace_stream() << "{\"seq\":" << trace_seq()++ \
    << ",\"event\":\"" << ev << "\"" __VA_ARGS__ << "}\n"
#define TF(n, v) << ",\"" << n << "\":" << v
#define TFS(n, v) << ",\"" << n << "\":\"" << v << "\""
#else
#define TRACE(ev, ...) ((void)0)
#define TF(n, v)
#define TFS(n, v)
#endif
#endif
