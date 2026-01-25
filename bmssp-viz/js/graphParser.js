/**
 * Graph Parser - Parse graph input files
 *
 * Format:
 * n m           // number of nodes, number of edges
 * u v w         // edge from u to v with weight w (repeated m times)
 * ...
 * source        // source node
 */

const GraphParser = {
    /**
     * Parse a graph file content
     * @param {string} content - The file content
     * @returns {Object} Parsed graph with nodes, edges, and source
     */
    parse(content) {
        const lines = content.trim().split('\n').map(l => l.trim()).filter(l => l.length > 0);

        if (lines.length < 1) {
            throw new Error('Invalid graph file: empty content');
        }

        // Parse first line: n m
        const firstLine = lines[0].split(/\s+/);
        const n = parseInt(firstLine[0], 10);
        const m = parseInt(firstLine[1], 10);

        if (isNaN(n) || isNaN(m)) {
            throw new Error('Invalid graph file: first line should be "n m"');
        }

        // Parse edges
        const edges = [];
        for (let i = 1; i <= m && i < lines.length; i++) {
            const parts = lines[i].split(/\s+/);
            if (parts.length >= 3) {
                const u = parseInt(parts[0], 10);
                const v = parseInt(parts[1], 10);
                const w = parseFloat(parts[2]);

                if (!isNaN(u) && !isNaN(v) && !isNaN(w)) {
                    edges.push({ source: u, target: v, weight: w });
                }
            }
        }

        // Parse source (last line)
        let source = 0;
        if (lines.length > m + 1) {
            const sourceLine = lines[m + 1].trim();
            const parsedSource = parseInt(sourceLine, 10);
            if (!isNaN(parsedSource)) {
                source = parsedSource;
            }
        }

        // Create nodes array
        const nodes = [];
        for (let i = 0; i < n; i++) {
            nodes.push({
                id: i,
                state: CONSTANTS.NODE_STATES.UNVISITED,
                distance: Infinity
            });
        }

        return {
            n,
            m,
            nodes,
            edges,
            source
        };
    },

    /**
     * Validate a parsed graph
     * @param {Object} graph - Parsed graph object
     * @returns {Object} Validation result with isValid and errors
     */
    validate(graph) {
        const errors = [];

        if (!graph.nodes || graph.nodes.length === 0) {
            errors.push('Graph has no nodes');
        }

        if (!graph.edges) {
            errors.push('Graph has no edges array');
        }

        // Check edge references
        if (graph.edges) {
            graph.edges.forEach((edge, i) => {
                if (edge.source < 0 || edge.source >= graph.n) {
                    errors.push(`Edge ${i}: invalid source node ${edge.source}`);
                }
                if (edge.target < 0 || edge.target >= graph.n) {
                    errors.push(`Edge ${i}: invalid target node ${edge.target}`);
                }
                if (edge.weight < 0) {
                    errors.push(`Edge ${i}: negative weight ${edge.weight}`);
                }
            });
        }

        // Check source
        if (graph.source < 0 || graph.source >= graph.n) {
            errors.push(`Invalid source node: ${graph.source}`);
        }

        return {
            isValid: errors.length === 0,
            errors
        };
    },

    /**
     * Get adjacency list representation
     * @param {Object} graph - Parsed graph
     * @returns {Array} Adjacency list
     */
    getAdjacencyList(graph) {
        const adj = Array.from({ length: graph.n }, () => []);

        graph.edges.forEach(edge => {
            adj[edge.source].push({
                target: edge.target,
                weight: edge.weight
            });
        });

        return adj;
    }
};
