/**
 * Priority Queue View - Visualize the PQ state
 */

class PriorityQueueView {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.maxDisplay = CONSTANTS.UI.MAX_PQ_DISPLAY;
        this.lastPopped = null;
    }

    /**
     * Render the priority queue
     * @param {Array} pq - Priority queue as array of {node, cost}
     */
    render(pq) {
        if (!this.container) return;

        if (!pq || pq.length === 0) {
            // Show last popped if available
            if (this.lastPopped !== null) {
                this.container.innerHTML = `
                    <div class="pq-item popped">
                        <span class="pq-node">Node ${this.lastPopped.node}</span>
                        <span class="pq-cost">${formatDistance(this.lastPopped.cost)} (popped)</span>
                    </div>
                `;
            } else {
                this.container.innerHTML = '<div class="empty-state">Empty</div>';
            }
            return;
        }

        // Sort by cost
        const sorted = [...pq].sort((a, b) => a.cost - b.cost);
        const displayItems = sorted.slice(0, this.maxDisplay);
        const hasMore = sorted.length > this.maxDisplay;

        let html = displayItems.map((item, idx) => {
            const isFirst = idx === 0;
            return `
                <div class="pq-item ${isFirst ? '' : ''}">
                    <span class="pq-node">Node ${item.node}</span>
                    <span class="pq-cost">${formatDistance(item.cost)}</span>
                </div>
            `;
        }).join('');

        if (hasMore) {
            html += `<div class="pq-item">... +${sorted.length - this.maxDisplay} more</div>`;
        }

        this.container.innerHTML = html;
    }

    /**
     * Mark an item as popped
     * @param {number} node - Node ID
     * @param {number} cost - Cost
     */
    markPopped(node, cost) {
        this.lastPopped = { node, cost };
    }

    /**
     * Clear the last popped state
     */
    clearPopped() {
        this.lastPopped = null;
    }

    /**
     * Clear the view
     */
    clear() {
        this.lastPopped = null;
        if (this.container) {
            this.container.innerHTML = '<div class="empty-state">Empty</div>';
        }
    }
}
