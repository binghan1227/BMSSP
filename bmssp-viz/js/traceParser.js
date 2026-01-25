/**
 * Trace Parser - Parse JSONL trace files
 *
 * Handles special values like 'inf' for infinity
 */

const TraceParser = {
    /**
     * Parse a JSONL trace file content
     * @param {string} content - The file content
     * @returns {Array} Array of parsed events
     */
    parse(content) {
        const lines = content.trim().split('\n');
        const events = [];

        lines.forEach((line, lineNum) => {
            const trimmed = line.trim();
            if (!trimmed) return;

            try {
                // Preprocess: replace 'inf' with a special marker for JSON parsing
                // The trace file uses bare 'inf' which is not valid JSON
                const preprocessed = trimmed
                    .replace(/:inf([,\}])/g, ':"__INFINITY__"$1')
                    .replace(/:inf$/g, ':"__INFINITY__"');

                const event = JSON.parse(preprocessed);

                // Post-process: convert infinity markers back to actual Infinity
                this._processInfinity(event);

                events.push(event);
            } catch (e) {
                console.warn(`Failed to parse line ${lineNum + 1}: ${e.message}`);
                console.warn(`Line content: ${trimmed.substring(0, 100)}`);
            }
        });

        // Sort by sequence number
        events.sort((a, b) => (a.seq || 0) - (b.seq || 0));

        return events;
    },

    /**
     * Recursively process infinity values in an object
     * @param {Object} obj - Object to process
     */
    _processInfinity(obj) {
        if (obj === null || typeof obj !== 'object') return;

        for (const key in obj) {
            if (obj[key] === '__INFINITY__') {
                obj[key] = Infinity;
            } else if (typeof obj[key] === 'object') {
                this._processInfinity(obj[key]);
            }
        }
    },

    /**
     * Validate parsed events
     * @param {Array} events - Parsed events
     * @returns {Object} Validation result
     */
    validate(events) {
        const errors = [];
        const warnings = [];

        if (!events || events.length === 0) {
            errors.push('No events found in trace file');
            return { isValid: false, errors, warnings };
        }

        // Check for required event types
        const eventTypes = new Set(events.map(e => e.event));
        if (!eventTypes.has(CONSTANTS.EVENTS.SOLVE_START)) {
            warnings.push('Missing SOLVE_START event');
        }

        // Check sequence numbers
        let lastSeq = -1;
        events.forEach((event, i) => {
            if (event.seq === undefined) {
                warnings.push(`Event ${i} missing sequence number`);
            } else if (event.seq <= lastSeq) {
                warnings.push(`Event ${i} has non-increasing sequence number`);
            }
            lastSeq = event.seq || lastSeq;
        });

        return {
            isValid: errors.length === 0,
            errors,
            warnings
        };
    },

    /**
     * Get event statistics
     * @param {Array} events - Parsed events
     * @returns {Object} Statistics about the trace
     */
    getStats(events) {
        const stats = {
            total: events.length,
            byType: {}
        };

        events.forEach(event => {
            const type = event.event || 'UNKNOWN';
            stats.byType[type] = (stats.byType[type] || 0) + 1;
        });

        return stats;
    },

    /**
     * Extract parameters from SOLVE_START event
     * @param {Array} events - Parsed events
     * @returns {Object|null} Parameters or null
     */
    getParams(events) {
        const startEvent = events.find(e => e.event === CONSTANTS.EVENTS.SOLVE_START);
        if (!startEvent) return null;

        return {
            n: startEvent.n,
            k: startEvent.k,
            t: startEvent.t,
            l: startEvent.l,
            source: startEvent.source
        };
    }
};
