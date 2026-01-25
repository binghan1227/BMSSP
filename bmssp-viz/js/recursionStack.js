/**
 * Recursion Stack View - Visualize the call stack
 */

class RecursionStackView {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
    }

    /**
     * Render the recursion stack
     * @param {Array} stack - Array of stack frames
     */
    render(stack) {
        if (!this.container) return;

        if (!stack || stack.length === 0) {
            this.container.innerHTML = '<div class="empty-state">No active recursion</div>';
            return;
        }

        // Render from top to bottom (most recent first)
        const html = stack.slice().reverse().map((frame, idx) => {
            const isTop = idx === 0;
            const level = frame.level;
            const B = formatBound(frame.B);
            const frontier = frame.frontier || [];
            const frontierStr = frontier.length > 5
                ? `[${frontier.slice(0, 5).join(', ')}...]`
                : `[${frontier.join(', ')}]`;

            return `
                <div class="stack-frame ${isTop ? 'active' : ''}">
                    <div class="stack-frame-header">
                        Level ${level} (B=${B})
                    </div>
                    <div class="stack-frame-details">
                        Frontier: ${frontierStr}
                    </div>
                </div>
            `;
        }).join('');

        this.container.innerHTML = html;
    }

    /**
     * Clear the view
     */
    clear() {
        if (this.container) {
            this.container.innerHTML = '<div class="empty-state">No active recursion</div>';
        }
    }
}
