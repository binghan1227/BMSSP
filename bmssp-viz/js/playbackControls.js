/**
 * Playback Controls - Handle play/pause/step controls
 */

class PlaybackControls {
    constructor(eventProcessor, onUpdate) {
        this.eventProcessor = eventProcessor;
        this.onUpdate = onUpdate;

        // UI elements
        this.btnFirst = document.getElementById('btn-first');
        this.btnPrev = document.getElementById('btn-prev');
        this.btnPlay = document.getElementById('btn-play');
        this.btnNext = document.getElementById('btn-next');
        this.btnLast = document.getElementById('btn-last');
        this.slider = document.getElementById('progress-slider');
        this.speedSelect = document.getElementById('speed-select');
        this.currentEventSpan = document.getElementById('current-event');
        this.totalEventsSpan = document.getElementById('total-events');

        // State
        this.isPlaying = false;
        this.playInterval = null;
        this.speed = 1;

        this._bindEvents();
    }

    _bindEvents() {
        if (this.btnFirst) {
            this.btnFirst.addEventListener('click', () => this.goToFirst());
        }

        if (this.btnPrev) {
            this.btnPrev.addEventListener('click', () => this.stepBackward());
        }

        if (this.btnPlay) {
            this.btnPlay.addEventListener('click', () => this.togglePlay());
        }

        if (this.btnNext) {
            this.btnNext.addEventListener('click', () => this.stepForward());
        }

        if (this.btnLast) {
            this.btnLast.addEventListener('click', () => this.goToLast());
        }

        if (this.slider) {
            this.slider.addEventListener('input', (e) => this.onSliderChange(e));
        }

        if (this.speedSelect) {
            this.speedSelect.addEventListener('change', (e) => this.onSpeedChange(e));
        }

        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => this._handleKeyboard(e));
    }

    _handleKeyboard(e) {
        // Ignore if focus is on an input
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') {
            return;
        }

        switch (e.key) {
            case ' ':
            case 'Space':
                e.preventDefault();
                this.togglePlay();
                break;
            case 'ArrowRight':
                e.preventDefault();
                this.stepForward();
                break;
            case 'ArrowLeft':
                e.preventDefault();
                this.stepBackward();
                break;
            case 'Home':
                e.preventDefault();
                this.goToFirst();
                break;
            case 'End':
                e.preventDefault();
                this.goToLast();
                break;
        }
    }

    /**
     * Initialize controls with event count
     */
    init() {
        const total = this.eventProcessor.getTotalEvents();
        this.updateDisplay();

        if (this.slider) {
            this.slider.max = Math.max(0, total - 1);
            this.slider.value = 0;
        }

        if (this.totalEventsSpan) {
            this.totalEventsSpan.textContent = total;
        }

        this._updateButtonStates();
    }

    /**
     * Step forward one event
     */
    stepForward() {
        if (this.eventProcessor.getCurrentIndex() >= this.eventProcessor.getTotalEvents() - 1) {
            this.pause();
            return null;
        }

        const event = this.eventProcessor.stepForward();
        this.updateDisplay();
        this._updateButtonStates();

        if (this.onUpdate) {
            this.onUpdate(event);
        }

        return event;
    }

    /**
     * Step backward one event
     */
    stepBackward() {
        this.pause();

        const event = this.eventProcessor.stepBackward();
        this.updateDisplay();
        this._updateButtonStates();

        if (this.onUpdate) {
            this.onUpdate(event);
        }

        return event;
    }

    /**
     * Go to first event (reset)
     */
    goToFirst() {
        this.pause();
        this.eventProcessor.reset();
        this.updateDisplay();
        this._updateButtonStates();

        if (this.onUpdate) {
            this.onUpdate(null);
        }
    }

    /**
     * Go to last event
     */
    goToLast() {
        this.pause();
        const total = this.eventProcessor.getTotalEvents();
        this.eventProcessor.jumpTo(total - 1);
        this.updateDisplay();
        this._updateButtonStates();

        if (this.onUpdate) {
            this.onUpdate(this.eventProcessor.getCurrentEvent());
        }
    }

    /**
     * Jump to specific event index
     */
    jumpTo(index) {
        this.pause();
        this.eventProcessor.jumpTo(index);
        this.updateDisplay();
        this._updateButtonStates();

        if (this.onUpdate) {
            this.onUpdate(this.eventProcessor.getCurrentEvent());
        }
    }

    /**
     * Toggle play/pause
     */
    togglePlay() {
        if (this.isPlaying) {
            this.pause();
        } else {
            this.play();
        }
    }

    /**
     * Start playback
     */
    play() {
        if (this.isPlaying) return;

        // If at end, restart
        if (this.eventProcessor.getCurrentIndex() >= this.eventProcessor.getTotalEvents() - 1) {
            this.eventProcessor.reset();
            this.updateDisplay();
        }

        this.isPlaying = true;
        this._updatePlayButton();

        const interval = CONSTANTS.SPEEDS[this.speed] || CONSTANTS.ANIMATION.DEFAULT_SPEED;

        this.playInterval = setInterval(() => {
            const event = this.stepForward();
            if (!event) {
                this.pause();
            }
        }, interval);
    }

    /**
     * Pause playback
     */
    pause() {
        if (!this.isPlaying) return;

        this.isPlaying = false;
        this._updatePlayButton();

        if (this.playInterval) {
            clearInterval(this.playInterval);
            this.playInterval = null;
        }
    }

    /**
     * Handle slider change
     */
    onSliderChange(e) {
        this.pause();
        const index = parseInt(e.target.value, 10);
        this.eventProcessor.jumpTo(index);
        this.updateDisplay();
        this._updateButtonStates();

        if (this.onUpdate) {
            this.onUpdate(this.eventProcessor.getCurrentEvent());
        }
    }

    /**
     * Handle speed change
     */
    onSpeedChange(e) {
        this.speed = parseFloat(e.target.value);

        // If playing, restart with new speed
        if (this.isPlaying) {
            this.pause();
            this.play();
        }
    }

    /**
     * Update display (slider position, event counter)
     */
    updateDisplay() {
        const current = this.eventProcessor.getCurrentIndex();
        const total = this.eventProcessor.getTotalEvents();

        if (this.slider) {
            this.slider.value = Math.max(0, current);
            this.slider.max = Math.max(0, total - 1);
        }

        if (this.currentEventSpan) {
            this.currentEventSpan.textContent = current + 1;
        }

        if (this.totalEventsSpan) {
            this.totalEventsSpan.textContent = total;
        }
    }

    _updatePlayButton() {
        if (this.btnPlay) {
            this.btnPlay.innerHTML = this.isPlaying ? '&#10074;&#10074;' : '&#9654;';
            this.btnPlay.title = this.isPlaying ? 'Pause' : 'Play';
        }
    }

    _updateButtonStates() {
        const current = this.eventProcessor.getCurrentIndex();
        const total = this.eventProcessor.getTotalEvents();

        const atStart = current < 0;
        const atEnd = current >= total - 1;

        if (this.btnFirst) this.btnFirst.disabled = atStart;
        if (this.btnPrev) this.btnPrev.disabled = atStart;
        if (this.btnNext) this.btnNext.disabled = atEnd;
        if (this.btnLast) this.btnLast.disabled = atEnd;
    }

    /**
     * Clean up
     */
    destroy() {
        this.pause();
        document.removeEventListener('keydown', this._handleKeyboard);
    }
}
