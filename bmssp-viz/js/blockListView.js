/**
 * Block List View - Visualize the block list state (D0 and D1)
 */

class BlockListView {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.maxDisplay = CONSTANTS.UI.MAX_BLOCKLIST_DISPLAY;
    }

    /**
     * Render the block list
     * @param {Object} blockList - Block list with D0 and D1 arrays
     */
    render(blockList) {
        if (!this.container) return;

        const d0 = blockList.D0 || [];
        const d1 = blockList.D1 || [];

        if (d0.length === 0 && d1.length === 0) {
            this.container.innerHTML = '<div class="empty-state">Empty</div>';
            return;
        }

        let html = '';

        // Render D0
        if (d0.length > 0) {
            html += this._renderSection('D0', d0);
        }

        // Render D1
        if (d1.length > 0) {
            html += this._renderSection('D1', d1);
        }

        this.container.innerHTML = html;
    }

    _renderSection(name, items) {
        const displayItems = items.slice(0, this.maxDisplay);
        const hasMore = items.length > this.maxDisplay;

        const itemsHtml = displayItems.map((item, idx) => {
            const isFirst = idx === 0;
            return `
                <span class="blocklist-item ${isFirst ? 'highlight' : ''}">
                    (${item.n}, ${formatDistance(item.d)})
                </span>
            `;
        }).join('');

        const moreHtml = hasMore ? `<span class="blocklist-item">+${items.length - this.maxDisplay} more</span>` : '';

        return `
            <div class="blocklist-section">
                <div class="blocklist-header">${name} (${items.length} items)</div>
                <div class="blocklist-items">
                    ${itemsHtml}
                    ${moreHtml}
                </div>
            </div>
        `;
    }

    /**
     * Highlight specific items
     * @param {Array} nodeIds - Node IDs to highlight
     */
    highlightItems(nodeIds) {
        const nodeSet = new Set(nodeIds);
        const items = this.container.querySelectorAll('.blocklist-item');

        items.forEach(item => {
            // Extract node ID from text content like "(3, 5.0)"
            const match = item.textContent.match(/\((\d+),/);
            if (match) {
                const nodeId = parseInt(match[1], 10);
                if (nodeSet.has(nodeId)) {
                    item.classList.add('highlight');
                } else {
                    item.classList.remove('highlight');
                }
            }
        });
    }

    /**
     * Clear the view
     */
    clear() {
        if (this.container) {
            this.container.innerHTML = '<div class="empty-state">Empty</div>';
        }
    }
}
