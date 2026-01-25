/**
 * Graph Renderer - D3.js based graph visualization
 */

class GraphRenderer {
    constructor(svgSelector) {
        this.svg = d3.select(svgSelector);
        this.graph = null;
        this.simulation = null;
        this.zoom = null;

        // SVG groups
        this.container = null;
        this.edgeGroup = null;
        this.nodeGroup = null;

        // D3 selections
        this.edges = null;
        this.edgeLabels = null;
        this.nodes = null;
        this.nodeLabels = null;
        this.distanceLabels = null;

        // State
        this.nodeStates = new Map();
        this.nodeDistances = new Map();
        this.highlightedEdges = new Set();

        this._init();
    }

    _init() {
        // Clear any existing content
        this.svg.selectAll('*').remove();

        // Get dimensions
        const rect = this.svg.node().getBoundingClientRect();
        this.width = rect.width || 800;
        this.height = rect.height || 600;

        // Define arrow marker
        const defs = this.svg.append('defs');

        defs.append('marker')
            .attr('id', 'arrowhead')
            .attr('viewBox', '0 -5 10 10')
            .attr('refX', 20)
            .attr('refY', 0)
            .attr('markerWidth', CONSTANTS.GRAPH.ARROW_SIZE)
            .attr('markerHeight', CONSTANTS.GRAPH.ARROW_SIZE)
            .attr('orient', 'auto')
            .append('path')
            .attr('d', 'M0,-5L10,0L0,5')
            .attr('class', 'arrow-marker');

        defs.append('marker')
            .attr('id', 'arrowhead-highlighted')
            .attr('viewBox', '0 -5 10 10')
            .attr('refX', 20)
            .attr('refY', 0)
            .attr('markerWidth', CONSTANTS.GRAPH.ARROW_SIZE)
            .attr('markerHeight', CONSTANTS.GRAPH.ARROW_SIZE)
            .attr('orient', 'auto')
            .append('path')
            .attr('d', 'M0,-5L10,0L0,5')
            .attr('fill', '#e94560');

        // Create container for zoom
        this.container = this.svg.append('g').attr('class', 'graph-container');

        // Create groups for edges and nodes
        this.edgeGroup = this.container.append('g').attr('class', 'edges');
        this.nodeGroup = this.container.append('g').attr('class', 'nodes');

        // Setup zoom behavior
        this.zoom = d3.zoom()
            .scaleExtent([0.1, 4])
            .on('zoom', (event) => {
                this.container.attr('transform', event.transform);
            });

        this.svg.call(this.zoom);
    }

    /**
     * Render the graph
     * @param {Object} graph - Parsed graph object
     */
    render(graph) {
        this.graph = graph;

        // Initialize state maps
        this.nodeStates.clear();
        this.nodeDistances.clear();
        this.highlightedEdges.clear();

        graph.nodes.forEach(node => {
            this.nodeStates.set(node.id, CONSTANTS.NODE_STATES.UNVISITED);
            this.nodeDistances.set(node.id, Infinity);
        });

        // Set source node
        this.nodeStates.set(graph.source, CONSTANTS.NODE_STATES.SOURCE);
        this.nodeDistances.set(graph.source, 0);

        // Determine node radius based on graph size
        const nodeRadius = graph.n > 30 ? CONSTANTS.GRAPH.NODE_RADIUS_SMALL : CONSTANTS.GRAPH.NODE_RADIUS;

        // Create simulation
        this.simulation = d3.forceSimulation(graph.nodes)
            .force('link', d3.forceLink(graph.edges)
                .id(d => d.id !== undefined ? d.id : d.index)
                .distance(CONSTANTS.GRAPH.LINK_DISTANCE))
            .force('charge', d3.forceManyBody()
                .strength(CONSTANTS.GRAPH.CHARGE_STRENGTH))
            .force('center', d3.forceCenter(this.width / 2, this.height / 2)
                .strength(CONSTANTS.GRAPH.CENTER_STRENGTH))
            .force('collision', d3.forceCollide()
                .radius(CONSTANTS.GRAPH.COLLISION_RADIUS));

        // Render edges
        this.edges = this.edgeGroup.selectAll('.edge')
            .data(graph.edges, (d, i) => `${d.source.id || d.source}-${d.target.id || d.target}-${i}`)
            .join(
                enter => {
                    const g = enter.append('g').attr('class', 'edge');

                    g.append('line')
                        .attr('class', 'edge-line')
                        .attr('marker-end', 'url(#arrowhead)');

                    g.append('text')
                        .attr('class', 'edge-weight')
                        .attr('dy', -5);

                    return g;
                }
            );

        // Render nodes
        this.nodes = this.nodeGroup.selectAll('.node')
            .data(graph.nodes, d => d.id)
            .join(
                enter => {
                    const g = enter.append('g')
                        .attr('class', 'node')
                        .call(this._drag());

                    g.append('circle')
                        .attr('class', 'node-circle')
                        .attr('r', nodeRadius);

                    g.append('text')
                        .attr('class', 'node-label')
                        .text(d => d.id);

                    g.append('text')
                        .attr('class', 'node-distance')
                        .attr('dy', nodeRadius + 12);

                    return g;
                }
            );

        // Update node colors
        this._updateNodeColors();
        this._updateDistanceLabels();

        // Simulation tick handler
        this.simulation.on('tick', () => this._tick(nodeRadius));

        // Initial positions (spread out)
        graph.nodes.forEach((node, i) => {
            const angle = (2 * Math.PI * i) / graph.n;
            const radius = Math.min(this.width, this.height) / 3;
            node.x = this.width / 2 + radius * Math.cos(angle);
            node.y = this.height / 2 + radius * Math.sin(angle);
        });

        // Run simulation for a bit then stop
        this.simulation.alpha(1).restart();
    }

    _tick(nodeRadius) {
        // Update edges
        this.edges.each(function(d) {
            const g = d3.select(this);
            const sourceX = d.source.x;
            const sourceY = d.source.y;
            const targetX = d.target.x;
            const targetY = d.target.y;

            g.select('.edge-line')
                .attr('x1', sourceX)
                .attr('y1', sourceY)
                .attr('x2', targetX)
                .attr('y2', targetY);

            g.select('.edge-weight')
                .attr('x', (sourceX + targetX) / 2)
                .attr('y', (sourceY + targetY) / 2)
                .text(d.weight);
        });

        // Update nodes
        this.nodes.attr('transform', d => `translate(${d.x},${d.y})`);
    }

    _drag() {
        return d3.drag()
            .on('start', (event, d) => {
                if (!event.active) this.simulation.alphaTarget(0.3).restart();
                d.fx = d.x;
                d.fy = d.y;
            })
            .on('drag', (event, d) => {
                d.fx = event.x;
                d.fy = event.y;
            })
            .on('end', (event, d) => {
                if (!event.active) this.simulation.alphaTarget(0);
                d.fx = null;
                d.fy = null;
            });
    }

    /**
     * Update node state
     * @param {number} nodeId - Node ID
     * @param {string} state - New state
     * @param {boolean} animate - Whether to animate
     */
    setNodeState(nodeId, state, animate = true) {
        this.nodeStates.set(nodeId, state);

        const node = this.nodes.filter(d => d.id === nodeId);
        const color = CONSTANTS.NODE_COLORS[state] || CONSTANTS.NODE_COLORS.unvisited;

        if (animate) {
            node.select('.node-circle')
                .transition()
                .duration(CONSTANTS.ANIMATION.TRANSITION_DURATION)
                .attr('fill', color);

            // Add pulse effect for important state changes
            if (state === CONSTANTS.NODE_STATES.PIVOT || state === CONSTANTS.NODE_STATES.SETTLED) {
                node.select('.node-circle')
                    .classed('node-pulse', true);
                setTimeout(() => {
                    node.select('.node-circle').classed('node-pulse', false);
                }, CONSTANTS.ANIMATION.PULSE_DURATION);
            }
        } else {
            node.select('.node-circle').attr('fill', color);
        }
    }

    /**
     * Update node distance
     * @param {number} nodeId - Node ID
     * @param {number} distance - New distance
     */
    setNodeDistance(nodeId, distance) {
        this.nodeDistances.set(nodeId, distance);
        this._updateDistanceLabel(nodeId);
    }

    /**
     * Highlight multiple nodes
     * @param {Array} nodeIds - Array of node IDs
     * @param {string} state - State to set
     */
    highlightNodes(nodeIds, state) {
        nodeIds.forEach(id => this.setNodeState(id, state));
    }

    /**
     * Update all node colors based on current state
     */
    _updateNodeColors() {
        this.nodes.each((d, i, nodes) => {
            const state = this.nodeStates.get(d.id) || CONSTANTS.NODE_STATES.UNVISITED;
            const color = CONSTANTS.NODE_COLORS[state];
            d3.select(nodes[i]).select('.node-circle').attr('fill', color);
        });
    }

    /**
     * Update all distance labels
     */
    _updateDistanceLabels() {
        this.nodes.each((d, i, nodes) => {
            const dist = this.nodeDistances.get(d.id);
            d3.select(nodes[i]).select('.node-distance')
                .text(formatDistance(dist));
        });
    }

    /**
     * Update single distance label
     */
    _updateDistanceLabel(nodeId) {
        this.nodes.filter(d => d.id === nodeId)
            .select('.node-distance')
            .text(formatDistance(this.nodeDistances.get(nodeId)));
    }

    /**
     * Highlight an edge
     * @param {number} source - Source node ID
     * @param {number} target - Target node ID
     * @param {boolean} highlight - Whether to highlight or unhighlight
     */
    highlightEdge(source, target, highlight = true) {
        const key = `${source}-${target}`;

        if (highlight) {
            this.highlightedEdges.add(key);
        } else {
            this.highlightedEdges.delete(key);
        }

        this.edges.each(function(d) {
            const src = d.source.id !== undefined ? d.source.id : d.source;
            const tgt = d.target.id !== undefined ? d.target.id : d.target;
            const edgeKey = `${src}-${tgt}`;

            const line = d3.select(this).select('.edge-line');

            if (highlight && edgeKey === key) {
                line.classed('highlighted', true)
                    .attr('marker-end', 'url(#arrowhead-highlighted)');
            } else if (!highlight && edgeKey === key) {
                line.classed('highlighted', false)
                    .attr('marker-end', 'url(#arrowhead)');
            }
        });
    }

    /**
     * Clear all edge highlights
     */
    clearEdgeHighlights() {
        this.highlightedEdges.clear();
        this.edges.selectAll('.edge-line')
            .classed('highlighted', false)
            .attr('marker-end', 'url(#arrowhead)');
    }

    /**
     * Reset graph to initial state
     */
    reset() {
        if (!this.graph) return;

        this.nodeStates.clear();
        this.nodeDistances.clear();
        this.highlightedEdges.clear();

        this.graph.nodes.forEach(node => {
            this.nodeStates.set(node.id, CONSTANTS.NODE_STATES.UNVISITED);
            this.nodeDistances.set(node.id, Infinity);
        });

        this.nodeStates.set(this.graph.source, CONSTANTS.NODE_STATES.SOURCE);
        this.nodeDistances.set(this.graph.source, 0);

        this._updateNodeColors();
        this._updateDistanceLabels();
        this.clearEdgeHighlights();
    }

    /**
     * Get current state for snapshot
     */
    getState() {
        return {
            nodeStates: new Map(this.nodeStates),
            nodeDistances: new Map(this.nodeDistances),
            highlightedEdges: new Set(this.highlightedEdges)
        };
    }

    /**
     * Restore state from snapshot
     */
    restoreState(snapshot) {
        this.nodeStates = new Map(snapshot.nodeStates);
        this.nodeDistances = new Map(snapshot.nodeDistances);
        this.highlightedEdges = new Set(snapshot.highlightedEdges);

        this._updateNodeColors();
        this._updateDistanceLabels();

        // Update edge highlights
        this.edges.each((d, i, edges) => {
            const src = d.source.id !== undefined ? d.source.id : d.source;
            const tgt = d.target.id !== undefined ? d.target.id : d.target;
            const key = `${src}-${tgt}`;
            const line = d3.select(edges[i]).select('.edge-line');

            if (this.highlightedEdges.has(key)) {
                line.classed('highlighted', true)
                    .attr('marker-end', 'url(#arrowhead-highlighted)');
            } else {
                line.classed('highlighted', false)
                    .attr('marker-end', 'url(#arrowhead)');
            }
        });
    }

    /**
     * Fit graph to view
     */
    fitToView() {
        if (!this.graph || this.graph.nodes.length === 0) return;

        const bounds = this.container.node().getBBox();
        const padding = 40;

        const dx = bounds.width + padding * 2;
        const dy = bounds.height + padding * 2;
        const x = bounds.x - padding;
        const y = bounds.y - padding;

        const scale = Math.min(
            0.9 * this.width / dx,
            0.9 * this.height / dy,
            2
        );

        const tx = this.width / 2 - scale * (x + dx / 2);
        const ty = this.height / 2 - scale * (y + dy / 2);

        this.svg.transition()
            .duration(500)
            .call(this.zoom.transform, d3.zoomIdentity.translate(tx, ty).scale(scale));
    }

    /**
     * Update dimensions on resize
     */
    resize() {
        const rect = this.svg.node().getBoundingClientRect();
        this.width = rect.width || 800;
        this.height = rect.height || 600;

        if (this.simulation) {
            this.simulation.force('center', d3.forceCenter(this.width / 2, this.height / 2));
            this.simulation.alpha(0.3).restart();
        }
    }
}
