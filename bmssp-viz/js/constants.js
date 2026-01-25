/**
 * BMSSP Visualizer Constants
 */

const CONSTANTS = {
    // Node colors for different states
    NODE_COLORS: {
        unvisited: '#666666',   // Gray
        frontier:  '#ffa500',   // Orange
        pivot:     '#ff00ff',   // Magenta
        inPQ:      '#87ceeb',   // Sky blue
        settled:   '#228b22',   // Forest green
        source:    '#00ff00'    // Lime green
    },

    // Node states
    NODE_STATES: {
        UNVISITED: 'unvisited',
        FRONTIER: 'frontier',
        PIVOT: 'pivot',
        IN_PQ: 'inPQ',
        SETTLED: 'settled',
        SOURCE: 'source'
    },

    // Event types
    EVENTS: {
        SOLVE_START: 'SOLVE_START',
        RECURSION_ENTER: 'RECURSION_ENTER',
        RECURSION_EXIT: 'RECURSION_EXIT',
        FIND_PIVOTS: 'FIND_PIVOTS',
        BASE_CASE: 'BASE_CASE',
        BASE_PQ_POP: 'BASE_PQ_POP',
        BL_PULL: 'BL_PULL',
        BL_PREPEND: 'BL_PREPEND'
    },

    // Graph rendering settings
    GRAPH: {
        NODE_RADIUS: 18,
        NODE_RADIUS_SMALL: 14,  // For larger graphs
        LINK_DISTANCE: 100,
        CHARGE_STRENGTH: -400,
        CENTER_STRENGTH: 0.1,
        COLLISION_RADIUS: 25,
        ARROW_SIZE: 8
    },

    // Animation settings
    ANIMATION: {
        TRANSITION_DURATION: 300,
        PULSE_DURATION: 500,
        DEFAULT_SPEED: 1000  // ms per event at 1x speed
    },

    // Playback speeds
    SPEEDS: {
        0.25: 4000,
        0.5: 2000,
        1: 1000,
        2: 500,
        4: 250
    },

    // UI settings
    UI: {
        MAX_PQ_DISPLAY: 20,
        MAX_BLOCKLIST_DISPLAY: 50
    }
};

// Event descriptions for display
const EVENT_DESCRIPTIONS = {
    SOLVE_START: (data) => `Starting BMSSP solve from source ${data.source} with n=${data.n}, k=${data.k}, t=${data.t}, l=${data.l}`,
    RECURSION_ENTER: (data) => `Entering recursion level ${data.l} with B=${formatBound(data.B)}, frontier: [${data.frontier.join(', ')}]`,
    RECURSION_EXIT: (data) => `Exiting recursion level ${data.l}, settled nodes: [${data.u_set.slice(0, 5).join(', ')}${data.u_set.length > 5 ? '...' : ''}]`,
    FIND_PIVOTS: (data) => `Found pivots: [${data.pivots.join(', ')}], exploring layers: [${data.all_layers.slice(0, 5).join(', ')}${data.all_layers.length > 5 ? '...' : ''}]`,
    BASE_CASE: (data) => `Base case for node ${data.node} with B=${formatBound(data.B)}`,
    BASE_PQ_POP: (data) => `PQ pop: node ${data.node} with cost ${data.cost.toFixed(2)}`,
    BL_PULL: (data) => `Pulling from block list: [${data.nodes.join(', ')}] with bound ${formatBound(data.bound)}`,
    BL_PREPEND: (data) => {
        if (data.elements.length === 0) return 'No elements to prepend to block list';
        const items = data.elements.slice(0, 3).map(e => `(${e.n}, ${e.d.toFixed(1)})`).join(', ');
        const more = data.elements.length > 3 ? ` +${data.elements.length - 3} more` : '';
        return `Prepending to block list: [${items}${more}]`;
    }
};

/**
 * Format bound value (handle infinity)
 */
function formatBound(value) {
    if (value === Infinity || value === 'inf' || value === null) {
        return '\u221E';  // Infinity symbol
    }
    if (typeof value === 'number') {
        return value.toFixed(1);
    }
    return String(value);
}

/**
 * Format distance for display
 */
function formatDistance(dist) {
    if (dist === Infinity || dist === null || dist === undefined) {
        return '\u221E';
    }
    if (Number.isInteger(dist)) {
        return String(dist);
    }
    return dist.toFixed(1);
}
