/**
 * Event Processor - Process trace events and update visualization state
 */

class EventProcessor {
    constructor(graphRenderer, recursionStack, blockListView, priorityQueueView) {
        this.graphRenderer = graphRenderer;
        this.recursionStack = recursionStack;
        this.blockListView = blockListView;
        this.priorityQueueView = priorityQueueView;

        this.events = [];
        this.currentIndex = -1;
        this.snapshots = [];  // State snapshots for backward navigation

        // Algorithm state
        this.state = this._createInitialState();
    }

    _createInitialState() {
        return {
            // Current PQ contents (simulated)
            priorityQueue: [],
            // Block list: { D0: [], D1: [] }
            blockList: { D0: [], D1: [] },
            // Recursion stack
            recursionStack: [],
            // Set of settled nodes
            settledNodes: new Set(),
            // Current frontier nodes
            frontierNodes: new Set(),
            // Pivot nodes
            pivotNodes: new Set(),
            // Node distances
            distances: new Map(),
            // Algorithm parameters
            params: null
        };
    }

    /**
     * Initialize with events
     * @param {Array} events - Parsed trace events
     */
    init(events) {
        this.events = events;
        this.currentIndex = -1;
        this.snapshots = [];
        this.state = this._createInitialState();

        // Pre-compute snapshots for each event for backward navigation
        this._precomputeSnapshots();
    }

    _precomputeSnapshots() {
        // Save initial state
        this.snapshots = [this._cloneState()];

        // Process each event and save state
        for (let i = 0; i < this.events.length; i++) {
            this._applyEvent(this.events[i], false);
            this.snapshots.push(this._cloneState());
        }

        // Reset to initial state
        this.state = this._createInitialState();
        this.currentIndex = -1;
    }

    _cloneState() {
        return {
            priorityQueue: [...this.state.priorityQueue],
            blockList: {
                D0: [...this.state.blockList.D0],
                D1: [...this.state.blockList.D1]
            },
            recursionStack: this.state.recursionStack.map(frame => ({ ...frame, frontier: [...frame.frontier] })),
            settledNodes: new Set(this.state.settledNodes),
            frontierNodes: new Set(this.state.frontierNodes),
            pivotNodes: new Set(this.state.pivotNodes),
            distances: new Map(this.state.distances),
            params: this.state.params ? { ...this.state.params } : null
        };
    }

    /**
     * Get current event index
     */
    getCurrentIndex() {
        return this.currentIndex;
    }

    /**
     * Get total number of events
     */
    getTotalEvents() {
        return this.events.length;
    }

    /**
     * Get current event
     */
    getCurrentEvent() {
        if (this.currentIndex < 0 || this.currentIndex >= this.events.length) {
            return null;
        }
        return this.events[this.currentIndex];
    }

    /**
     * Step to next event
     * @returns {Object|null} The processed event or null
     */
    stepForward() {
        if (this.currentIndex >= this.events.length - 1) {
            return null;
        }

        this.currentIndex++;
        const event = this.events[this.currentIndex];
        this._applyEvent(event, true);
        this._updateViews();

        return event;
    }

    /**
     * Step to previous event
     * @returns {Object|null} The current event after stepping back
     */
    stepBackward() {
        if (this.currentIndex < 0) {
            return null;
        }

        // Restore state from snapshot
        const snapshot = this.snapshots[this.currentIndex];
        this.state = this._cloneStateFrom(snapshot);
        this.currentIndex--;

        this._restoreGraphState();
        this._updateViews();

        return this.currentIndex >= 0 ? this.events[this.currentIndex] : null;
    }

    _cloneStateFrom(snapshot) {
        return {
            priorityQueue: [...snapshot.priorityQueue],
            blockList: {
                D0: [...snapshot.blockList.D0],
                D1: [...snapshot.blockList.D1]
            },
            recursionStack: snapshot.recursionStack.map(frame => ({ ...frame, frontier: [...frame.frontier] })),
            settledNodes: new Set(snapshot.settledNodes),
            frontierNodes: new Set(snapshot.frontierNodes),
            pivotNodes: new Set(snapshot.pivotNodes),
            distances: new Map(snapshot.distances),
            params: snapshot.params ? { ...snapshot.params } : null
        };
    }

    /**
     * Jump to specific event index
     * @param {number} index - Target index
     */
    jumpTo(index) {
        if (index < -1 || index >= this.events.length) {
            return;
        }

        // Restore from snapshot
        const snapshotIndex = index + 1;  // snapshots[0] is initial state
        const snapshot = this.snapshots[snapshotIndex];
        this.state = this._cloneStateFrom(snapshot);
        this.currentIndex = index;

        this._restoreGraphState();
        this._updateViews();
    }

    /**
     * Reset to initial state
     */
    reset() {
        this.state = this._createInitialState();
        this.currentIndex = -1;

        if (this.graphRenderer) {
            this.graphRenderer.reset();
        }

        this._updateViews();
    }

    /**
     * Apply an event to the state
     * @param {Object} event - The event to apply
     * @param {boolean} updateGraph - Whether to update graph visualization
     */
    _applyEvent(event, updateGraph) {
        const type = event.event;

        switch (type) {
            case CONSTANTS.EVENTS.SOLVE_START:
                this._handleSolveStart(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.RECURSION_ENTER:
                this._handleRecursionEnter(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.RECURSION_EXIT:
                this._handleRecursionExit(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.FIND_PIVOTS:
                this._handleFindPivots(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.BASE_CASE:
                this._handleBaseCase(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.BASE_PQ_POP:
                this._handleBasePQPop(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.BL_PULL:
                this._handleBLPull(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.BL_PREPEND:
                this._handleBLPrepend(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.BASE_RELAX:
                this._handleBaseRelax(event, updateGraph);
                break;

            case CONSTANTS.EVENTS.BL_INSERT:
                this._handleBLInsert(event, updateGraph);
                break;

            default:
                console.warn(`Unknown event type: ${type}`);
        }
    }

    _handleSolveStart(event, updateGraph) {
        this.state.params = {
            n: event.n,
            k: event.k,
            t: event.t,
            l: event.l,
            source: event.source
        };

        // Initialize source distance
        this.state.distances.set(event.source, 0);

        if (updateGraph && this.graphRenderer) {
            this.graphRenderer.setNodeState(event.source, CONSTANTS.NODE_STATES.SOURCE);
            this.graphRenderer.setNodeDistance(event.source, 0);
        }
    }

    _handleRecursionEnter(event, updateGraph) {
        const frame = {
            level: event.l,
            B: event.B,
            frontier: event.frontier || []
        };

        this.state.recursionStack.push(frame);

        // Mark frontier nodes
        if (event.frontier) {
            event.frontier.forEach(nodeId => {
                this.state.frontierNodes.add(nodeId);
                if (updateGraph && this.graphRenderer &&
                    !this.state.settledNodes.has(nodeId) &&
                    !this.state.pivotNodes.has(nodeId)) {
                    this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.FRONTIER);
                }
            });
        }
    }

    _handleRecursionExit(event, updateGraph) {
        this.state.recursionStack.pop();

        // Mark settled nodes
        if (event.u_set) {
            event.u_set.forEach(nodeId => {
                this.state.settledNodes.add(nodeId);
                this.state.frontierNodes.delete(nodeId);
                this.state.pivotNodes.delete(nodeId);

                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.SETTLED);
                    // Also update distance display from stored state
                    const dist = this.state.distances.get(nodeId);
                    if (dist !== undefined) {
                        this.graphRenderer.setNodeDistance(nodeId, dist);
                    }
                }
            });
        }
    }

    _handleFindPivots(event, updateGraph) {
        // Mark pivots
        if (event.pivots) {
            event.pivots.forEach(nodeId => {
                this.state.pivotNodes.add(nodeId);
                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.PIVOT);
                }
            });
        }

        // Handle all_layers - now contains {n, d} objects with distances
        if (event.all_layers) {
            event.all_layers.forEach(item => {
                // Support both old format (int) and new format ({n, d})
                const nodeId = typeof item === 'object' ? item.n : item;
                const dist = typeof item === 'object' ? item.d : undefined;

                // Update distance if available
                if (dist !== undefined) {
                    const currentDist = this.state.distances.get(nodeId);
                    if (currentDist === undefined || dist < currentDist) {
                        this.state.distances.set(nodeId, dist);
                        if (updateGraph && this.graphRenderer) {
                            this.graphRenderer.setNodeDistance(nodeId, dist);
                        }
                    }
                }

                // Mark as frontier if not already settled/pivot
                if (!this.state.pivotNodes.has(nodeId) && !this.state.settledNodes.has(nodeId)) {
                    this.state.frontierNodes.add(nodeId);
                    if (updateGraph && this.graphRenderer) {
                        this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.FRONTIER);
                    }
                }
            });
        }
    }

    _handleBaseCase(event, updateGraph) {
        // Initialize PQ for base case - add the base node
        const node = event.node;
        const dist = this.state.distances.get(node) || 0;

        this.state.priorityQueue = [{ node, cost: dist }];

        if (updateGraph && this.graphRenderer && !this.state.settledNodes.has(node)) {
            this.graphRenderer.setNodeState(node, CONSTANTS.NODE_STATES.IN_PQ);
        }
    }

    _handleBasePQPop(event, updateGraph) {
        const node = event.node;
        const cost = event.cost;

        // Remove from PQ
        this.state.priorityQueue = this.state.priorityQueue.filter(item => item.node !== node);

        // Update distance
        this.state.distances.set(node, cost);
        this.state.settledNodes.add(node);

        if (updateGraph && this.graphRenderer) {
            this.graphRenderer.setNodeState(node, CONSTANTS.NODE_STATES.SETTLED);
            this.graphRenderer.setNodeDistance(node, cost);
        }
    }

    _handleBLPull(event, updateGraph) {
        const nodes = event.nodes || [];

        // Remove nodes from block list
        nodes.forEach(nodeId => {
            this.state.blockList.D0 = this.state.blockList.D0.filter(item => item.n !== nodeId);
            this.state.blockList.D1 = this.state.blockList.D1.filter(item => item.n !== nodeId);
        });

        // Mark as frontier
        nodes.forEach(nodeId => {
            if (!this.state.settledNodes.has(nodeId)) {
                this.state.frontierNodes.add(nodeId);
                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.FRONTIER);
                }
            }
        });
    }

    _handleBLPrepend(event, updateGraph) {
        if (!event.elements || event.elements.length === 0) {
            return;
        }

        // Prepend to D0
        const newItems = event.elements.map(e => ({ n: e.n, d: e.d }));
        this.state.blockList.D0 = [...newItems, ...this.state.blockList.D0];

        // Update distances and node states
        event.elements.forEach(e => {
            const currentDist = this.state.distances.get(e.n);
            if (currentDist === undefined || e.d < currentDist) {
                this.state.distances.set(e.n, e.d);
                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeDistance(e.n, e.d);
                }
            }
            if (!this.state.settledNodes.has(e.n)) {
                this.state.frontierNodes.add(e.n);
                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeState(e.n, CONSTANTS.NODE_STATES.FRONTIER);
                }
            }
        });
    }

    _handleBaseRelax(event, updateGraph) {
        const toNode = event.to;
        const cost = event.cost;

        // Update distance
        const currentDist = this.state.distances.get(toNode);
        if (currentDist === undefined || cost < currentDist) {
            this.state.distances.set(toNode, cost);
            if (updateGraph && this.graphRenderer) {
                this.graphRenderer.setNodeDistance(toNode, cost);
            }
        }

        // Add to PQ state
        this.state.priorityQueue.push({ node: toNode, cost: cost });

        if (!this.state.settledNodes.has(toNode)) {
            if (updateGraph && this.graphRenderer) {
                this.graphRenderer.setNodeState(toNode, CONSTANTS.NODE_STATES.IN_PQ);
            }
        }
    }

    _handleBLInsert(event, updateGraph) {
        if (!event.elements || event.elements.length === 0) {
            return;
        }

        // Add to D1
        event.elements.forEach(e => {
            this.state.blockList.D1.push({ n: e.n, d: e.d });

            const currentDist = this.state.distances.get(e.n);
            if (currentDist === undefined || e.d < currentDist) {
                this.state.distances.set(e.n, e.d);
                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeDistance(e.n, e.d);
                }
            }
            if (!this.state.settledNodes.has(e.n)) {
                this.state.frontierNodes.add(e.n);
                if (updateGraph && this.graphRenderer) {
                    this.graphRenderer.setNodeState(e.n, CONSTANTS.NODE_STATES.FRONTIER);
                }
            }
        });
    }

    /**
     * Restore graph state from current state object
     */
    _restoreGraphState() {
        if (!this.graphRenderer) return;

        // Reset all nodes to unvisited first
        this.graphRenderer.reset();

        // Apply settled nodes
        this.state.settledNodes.forEach(nodeId => {
            this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.SETTLED, false);
        });

        // Apply frontier nodes
        this.state.frontierNodes.forEach(nodeId => {
            if (!this.state.settledNodes.has(nodeId)) {
                this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.FRONTIER, false);
            }
        });

        // Apply pivot nodes
        this.state.pivotNodes.forEach(nodeId => {
            if (!this.state.settledNodes.has(nodeId)) {
                this.graphRenderer.setNodeState(nodeId, CONSTANTS.NODE_STATES.PIVOT, false);
            }
        });

        // Apply distances
        this.state.distances.forEach((dist, nodeId) => {
            this.graphRenderer.setNodeDistance(nodeId, dist);
        });

        // Mark source only if it hasn't been assigned a more specific state
        if (this.state.params) {
            const source = this.state.params.source;
            if (!this.state.settledNodes.has(source) &&
                !this.state.frontierNodes.has(source) &&
                !this.state.pivotNodes.has(source)) {
                this.graphRenderer.setNodeState(source, CONSTANTS.NODE_STATES.SOURCE, false);
            }
        }
    }

    /**
     * Update all view components
     */
    _updateViews() {
        if (this.recursionStack) {
            this.recursionStack.render(this.state.recursionStack);
        }

        if (this.blockListView) {
            this.blockListView.render(this.state.blockList);
        }

        if (this.priorityQueueView) {
            this.priorityQueueView.render(this.state.priorityQueue);
        }
    }

    /**
     * Get event description for display
     * @param {Object} event - The event
     * @returns {string} Human-readable description
     */
    getEventDescription(event) {
        if (!event) return 'No event';

        const descFn = EVENT_DESCRIPTIONS[event.event];
        if (descFn) {
            return descFn(event);
        }

        return `Event: ${event.event}`;
    }

    /**
     * Get current algorithm parameters
     */
    getParams() {
        return this.state.params;
    }
}
