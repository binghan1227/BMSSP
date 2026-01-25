/**
 * BMSSP Visualizer - Main Application
 */

class BMSSPVisualizer {
    constructor() {
        // State
        this.graph = null;
        this.events = null;
        this.isReady = false;

        // Components
        this.graphRenderer = null;
        this.eventProcessor = null;
        this.playbackControls = null;
        this.recursionStack = null;
        this.blockListView = null;
        this.priorityQueueView = null;

        // UI elements
        this.loadGraphBtn = document.getElementById('load-graph-btn');
        this.loadTraceBtn = document.getElementById('load-trace-btn');
        this.graphFileInput = document.getElementById('graph-file-input');
        this.traceFileInput = document.getElementById('trace-file-input');
        this.fileStatus = document.getElementById('file-status');
        this.dropZone = document.getElementById('drop-zone');
        this.graphSvgContainer = document.getElementById('graph-svg-container');
        this.eventDescription = document.getElementById('event-description');

        this._init();
    }

    _init() {
        // Initialize view components
        this.recursionStack = new RecursionStackView('stack-content');
        this.blockListView = new BlockListView('blocklist-content');
        this.priorityQueueView = new PriorityQueueView('pq-content');

        // Bind file input events
        this._bindFileEvents();

        // Bind window resize
        window.addEventListener('resize', () => this._onResize());

        this._updateStatus('Ready - Load graph and trace files');
    }

    _bindFileEvents() {
        // Load buttons
        if (this.loadGraphBtn) {
            this.loadGraphBtn.addEventListener('click', () => this.graphFileInput.click());
        }

        if (this.loadTraceBtn) {
            this.loadTraceBtn.addEventListener('click', () => this.traceFileInput.click());
        }

        // File inputs
        if (this.graphFileInput) {
            this.graphFileInput.addEventListener('change', (e) => this._handleGraphFile(e.target.files[0]));
        }

        if (this.traceFileInput) {
            this.traceFileInput.addEventListener('change', (e) => this._handleTraceFile(e.target.files[0]));
        }

        // Drag and drop
        if (this.dropZone) {
            this.dropZone.addEventListener('dragover', (e) => {
                e.preventDefault();
                this.dropZone.classList.add('drag-over');
            });

            this.dropZone.addEventListener('dragleave', () => {
                this.dropZone.classList.remove('drag-over');
            });

            this.dropZone.addEventListener('drop', (e) => {
                e.preventDefault();
                this.dropZone.classList.remove('drag-over');
                this._handleDroppedFiles(e.dataTransfer.files);
            });
        }
    }

    _handleDroppedFiles(files) {
        for (const file of files) {
            if (file.name.endsWith('.in') || file.name.endsWith('.txt')) {
                this._handleGraphFile(file);
            } else if (file.name.endsWith('.jsonl') || file.name.endsWith('.json')) {
                this._handleTraceFile(file);
            }
        }
    }

    _handleGraphFile(file) {
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (e) => {
            try {
                this.graph = GraphParser.parse(e.target.result);
                const validation = GraphParser.validate(this.graph);

                if (!validation.isValid) {
                    this._updateStatus(`Graph error: ${validation.errors.join(', ')}`);
                    return;
                }

                this._updateStatus(`Loaded graph: ${this.graph.n} nodes, ${this.graph.m} edges`);
                this._tryInitialize();
            } catch (err) {
                this._updateStatus(`Graph parse error: ${err.message}`);
                console.error(err);
            }
        };
        reader.readAsText(file);
    }

    _handleTraceFile(file) {
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (e) => {
            try {
                this.events = TraceParser.parse(e.target.result);
                const validation = TraceParser.validate(this.events);

                if (!validation.isValid) {
                    this._updateStatus(`Trace error: ${validation.errors.join(', ')}`);
                    return;
                }

                if (validation.warnings.length > 0) {
                    console.warn('Trace warnings:', validation.warnings);
                }

                const stats = TraceParser.getStats(this.events);
                this._updateStatus(`Loaded trace: ${stats.total} events`);
                this._tryInitialize();
            } catch (err) {
                this._updateStatus(`Trace parse error: ${err.message}`);
                console.error(err);
            }
        };
        reader.readAsText(file);
    }

    _tryInitialize() {
        if (!this.graph || !this.events) {
            return;
        }

        // Hide drop zone, show graph
        if (this.dropZone) {
            this.dropZone.classList.add('hidden');
        }
        if (this.graphSvgContainer) {
            this.graphSvgContainer.style.display = 'block';
        }

        // Initialize graph renderer
        this.graphRenderer = new GraphRenderer('#graph-svg');
        this.graphRenderer.render(this.graph);

        // Initialize event processor
        this.eventProcessor = new EventProcessor(
            this.graphRenderer,
            this.recursionStack,
            this.blockListView,
            this.priorityQueueView
        );
        this.eventProcessor.init(this.events);

        // Initialize playback controls
        this.playbackControls = new PlaybackControls(
            this.eventProcessor,
            (event) => this._onEventUpdate(event)
        );
        this.playbackControls.init();

        // Update params display
        const params = TraceParser.getParams(this.events);
        if (params) {
            this._updateParams(params);
        }

        // Fit graph to view after a short delay
        setTimeout(() => {
            this.graphRenderer.fitToView();
        }, 500);

        this.isReady = true;
        this._updateStatus('Ready - Use controls to step through events');
        this._updateEventDescription(null);
    }

    _onEventUpdate(event) {
        this._updateEventDescription(event);
    }

    _updateEventDescription(event) {
        if (!this.eventDescription) return;

        if (!event) {
            this.eventDescription.innerHTML = '<span class="event-type">Initial State</span> - No events processed yet';
            return;
        }

        const desc = this.eventProcessor.getEventDescription(event);
        this.eventDescription.innerHTML = `<span class="event-type">${event.event}</span> - ${desc}`;
    }

    _updateParams(params) {
        const setParam = (id, value) => {
            const el = document.getElementById(id);
            if (el) el.textContent = value !== undefined ? value : '-';
        };

        setParam('param-n', params.n);
        setParam('param-k', params.k);
        setParam('param-t', params.t);
        setParam('param-l', params.l);
        setParam('param-src', params.source);
    }

    _updateStatus(message) {
        if (this.fileStatus) {
            this.fileStatus.textContent = message;
        }
    }

    _onResize() {
        if (this.graphRenderer) {
            this.graphRenderer.resize();
        }
    }
}

// Initialize application when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new BMSSPVisualizer();
});
