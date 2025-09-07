// =============================================================================
// ECScope UI Components and Utilities
// =============================================================================

class UIComponents {
    constructor() {
        this.activeModals = new Set();
        this.toasts = [];
        this.maxToasts = 5;
        
        this.initializeComponents();
    }

    initializeComponents() {
        this.setupModalSystem();
        this.setupToastSystem();
        this.setupProgressBars();
        this.setupTabSystem();
        this.setupTooltips();
    }

    // =============================================================================
    // MODAL SYSTEM
    // =============================================================================

    setupModalSystem() {
        // Handle modal close on backdrop click
        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('modal')) {
                this.closeModal(e.target);
            }
        });

        // Handle modal close on escape key
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && this.activeModals.size > 0) {
                const lastModal = Array.from(this.activeModals).pop();
                this.closeModal(lastModal);
            }
        });
    }

    showModal(modalId, title = null, content = null) {
        const modal = document.getElementById(modalId);
        if (!modal) return;

        // Update title if provided
        if (title) {
            const titleElement = modal.querySelector('.modal-header h2');
            if (titleElement) {
                titleElement.textContent = title;
            }
        }

        // Update content if provided
        if (content) {
            const contentElement = modal.querySelector('.modal-body');
            if (contentElement) {
                contentElement.innerHTML = content;
            }
        }

        modal.classList.add('active');
        this.activeModals.add(modal);
        
        // Prevent body scroll
        document.body.style.overflow = 'hidden';
    }

    closeModal(modal) {
        if (typeof modal === 'string') {
            modal = document.getElementById(modal);
        }
        
        if (!modal) return;

        modal.classList.remove('active');
        this.activeModals.delete(modal);
        
        // Re-enable body scroll if no modals are open
        if (this.activeModals.size === 0) {
            document.body.style.overflow = '';
        }
    }

    // =============================================================================
    // TOAST SYSTEM
    // =============================================================================

    setupToastSystem() {
        // Create toast container if it doesn't exist
        if (!document.getElementById('toast-container')) {
            const container = document.createElement('div');
            container.id = 'toast-container';
            container.className = 'toast-container';
            container.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                z-index: 1050;
                display: flex;
                flex-direction: column;
                gap: 10px;
                pointer-events: none;
            `;
            document.body.appendChild(container);
        }
    }

    showToast(message, type = 'info', duration = 5000) {
        const container = document.getElementById('toast-container');
        if (!container) return;

        const toast = document.createElement('div');
        toast.className = `toast toast-${type}`;
        toast.style.cssText = `
            background: var(--surface-color);
            border: 1px solid var(--border-color);
            border-left: 4px solid var(--${type === 'success' ? 'success' : type === 'error' ? 'danger' : type === 'warning' ? 'warning' : 'info'}-color);
            border-radius: var(--border-radius);
            padding: 1rem;
            box-shadow: var(--shadow);
            max-width: 300px;
            pointer-events: auto;
            transform: translateX(100%);
            transition: transform 0.3s ease-in-out;
        `;

        const icon = this.getToastIcon(type);
        toast.innerHTML = `
            <div style="display: flex; align-items: flex-start; gap: 0.5rem;">
                <div style="color: var(--${type === 'success' ? 'success' : type === 'error' ? 'danger' : type === 'warning' ? 'warning' : 'info'}-color); margin-top: 2px;">
                    ${icon}
                </div>
                <div style="flex: 1;">
                    <div style="font-weight: 500; margin-bottom: 0.25rem; text-transform: capitalize;">
                        ${type}
                    </div>
                    <div style="font-size: 0.9rem; color: var(--text-muted);">
                        ${message}
                    </div>
                </div>
                <button class="toast-close" style="background: none; border: none; color: var(--text-muted); cursor: pointer; padding: 0; margin-left: 0.5rem;">
                    ×
                </button>
            </div>
        `;

        // Add close functionality
        toast.querySelector('.toast-close').addEventListener('click', () => {
            this.removeToast(toast);
        });

        container.appendChild(toast);
        this.toasts.push(toast);

        // Animate in
        setTimeout(() => {
            toast.style.transform = 'translateX(0)';
        }, 10);

        // Auto remove after duration
        if (duration > 0) {
            setTimeout(() => {
                this.removeToast(toast);
            }, duration);
        }

        // Remove excess toasts
        while (this.toasts.length > this.maxToasts) {
            this.removeToast(this.toasts[0]);
        }
    }

    removeToast(toast) {
        if (!toast || !toast.parentNode) return;

        toast.style.transform = 'translateX(100%)';
        
        setTimeout(() => {
            if (toast.parentNode) {
                toast.parentNode.removeChild(toast);
            }
            
            const index = this.toasts.indexOf(toast);
            if (index > -1) {
                this.toasts.splice(index, 1);
            }
        }, 300);
    }

    getToastIcon(type) {
        switch (type) {
            case 'success': return '✓';
            case 'error': return '✕';
            case 'warning': return '⚠';
            case 'info': 
            default: return 'ℹ';
        }
    }

    // =============================================================================
    // PROGRESS BAR SYSTEM
    // =============================================================================

    setupProgressBars() {
        // Animate progress bars when they come into view
        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const progressFill = entry.target.querySelector('.progress-fill');
                    if (progressFill) {
                        const targetWidth = progressFill.style.width;
                        progressFill.style.width = '0%';
                        setTimeout(() => {
                            progressFill.style.width = targetWidth;
                        }, 100);
                    }
                }
            });
        });

        document.querySelectorAll('.progress-bar').forEach(bar => {
            observer.observe(bar);
        });
    }

    updateProgressBar(selector, percentage, animated = true) {
        const progressBars = document.querySelectorAll(selector);
        progressBars.forEach(bar => {
            const fill = bar.querySelector('.progress-fill');
            if (fill) {
                if (animated) {
                    fill.style.transition = 'width 0.3s ease-in-out';
                }
                fill.style.width = `${Math.max(0, Math.min(100, percentage))}%`;
            }
        });
    }

    // =============================================================================
    // TAB SYSTEM
    // =============================================================================

    setupTabSystem() {
        document.addEventListener('click', (e) => {
            if (e.target.classList.contains('nav-item') && e.target.dataset.section) {
                this.switchTab(e.target.dataset.section);
            }
        });
    }

    switchTab(sectionId) {
        // Update navigation
        document.querySelectorAll('.nav-item').forEach(item => {
            item.classList.remove('active');
            if (item.dataset.section === sectionId) {
                item.classList.add('active');
            }
        });

        // Update sections
        document.querySelectorAll('.content-section').forEach(section => {
            section.classList.remove('active');
            if (section.id === `${sectionId}-section`) {
                section.classList.add('active');
            }
        });

        // Trigger section-specific initialization
        this.onSectionChanged(sectionId);
    }

    onSectionChanged(sectionId) {
        // Section-specific logic
        switch (sectionId) {
            case 'performance':
                if (window.performanceMonitor && !window.performanceMonitor.isMonitoring) {
                    window.performanceMonitor.start();
                }
                break;
            case 'physics':
                // Initialize physics canvas if needed
                break;
            case 'graphics':
                // Initialize graphics context if needed
                break;
        }
    }

    // =============================================================================
    // TOOLTIP SYSTEM
    // =============================================================================

    setupTooltips() {
        let currentTooltip = null;

        document.addEventListener('mouseenter', (e) => {
            const element = e.target.closest('[data-tooltip]');
            if (element) {
                this.showTooltip(element, element.dataset.tooltip);
            }
        }, true);

        document.addEventListener('mouseleave', (e) => {
            const element = e.target.closest('[data-tooltip]');
            if (element) {
                this.hideTooltip();
            }
        }, true);
    }

    showTooltip(element, text) {
        this.hideTooltip(); // Hide any existing tooltip

        const tooltip = document.createElement('div');
        tooltip.className = 'tooltip';
        tooltip.textContent = text;
        tooltip.style.cssText = `
            position: absolute;
            background: rgba(0, 0, 0, 0.8);
            color: white;
            padding: 0.5rem;
            border-radius: var(--border-radius);
            font-size: 0.8rem;
            z-index: 1060;
            max-width: 200px;
            pointer-events: none;
            opacity: 0;
            transition: opacity 0.2s ease-in-out;
        `;

        document.body.appendChild(tooltip);

        // Position tooltip
        const rect = element.getBoundingClientRect();
        const tooltipRect = tooltip.getBoundingClientRect();
        
        let left = rect.left + (rect.width / 2) - (tooltipRect.width / 2);
        let top = rect.top - tooltipRect.height - 8;

        // Adjust if tooltip would go off screen
        if (left < 8) left = 8;
        if (left + tooltipRect.width > window.innerWidth - 8) {
            left = window.innerWidth - tooltipRect.width - 8;
        }
        if (top < 8) {
            top = rect.bottom + 8;
        }

        tooltip.style.left = `${left}px`;
        tooltip.style.top = `${top}px`;

        // Fade in
        setTimeout(() => {
            tooltip.style.opacity = '1';
        }, 10);

        this.currentTooltip = tooltip;
    }

    hideTooltip() {
        if (this.currentTooltip) {
            this.currentTooltip.style.opacity = '0';
            setTimeout(() => {
                if (this.currentTooltip && this.currentTooltip.parentNode) {
                    this.currentTooltip.parentNode.removeChild(this.currentTooltip);
                }
                this.currentTooltip = null;
            }, 200);
        }
    }

    // =============================================================================
    // UTILITY METHODS
    // =============================================================================

    formatBytes(bytes, decimals = 2) {
        if (bytes === 0) return '0 Bytes';

        const k = 1024;
        const dm = decimals < 0 ? 0 : decimals;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];

        const i = Math.floor(Math.log(bytes) / Math.log(k));

        return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
    }

    formatNumber(num, decimals = 0) {
        return num.toLocaleString(undefined, {
            minimumFractionDigits: decimals,
            maximumFractionDigits: decimals
        });
    }

    formatDuration(ms) {
        if (ms < 1000) {
            return `${ms.toFixed(1)}ms`;
        } else if (ms < 60000) {
            return `${(ms / 1000).toFixed(1)}s`;
        } else {
            const minutes = Math.floor(ms / 60000);
            const seconds = ((ms % 60000) / 1000).toFixed(0);
            return `${minutes}:${seconds.padStart(2, '0')}`;
        }
    }

    debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }

    throttle(func, limit) {
        let inThrottle;
        return function executedFunction(...args) {
            if (!inThrottle) {
                func.apply(this, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    }

    // Copy text to clipboard
    async copyToClipboard(text) {
        try {
            await navigator.clipboard.writeText(text);
            this.showToast('Copied to clipboard', 'success', 2000);
        } catch (err) {
            // Fallback for older browsers
            const textArea = document.createElement('textarea');
            textArea.value = text;
            document.body.appendChild(textArea);
            textArea.select();
            document.execCommand('copy');
            document.body.removeChild(textArea);
            this.showToast('Copied to clipboard', 'success', 2000);
        }
    }

    // Download data as file
    downloadAsFile(data, filename, type = 'application/json') {
        const blob = new Blob([data], { type });
        const url = URL.createObjectURL(blob);
        
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        
        URL.revokeObjectURL(url);
    }

    // Get system information
    getSystemInfo() {
        return {
            userAgent: navigator.userAgent,
            platform: navigator.platform,
            language: navigator.language,
            hardwareConcurrency: navigator.hardwareConcurrency,
            memory: navigator.deviceMemory,
            connection: navigator.connection ? {
                effectiveType: navigator.connection.effectiveType,
                downlink: navigator.connection.downlink,
                rtt: navigator.connection.rtt
            } : null,
            screen: {
                width: screen.width,
                height: screen.height,
                colorDepth: screen.colorDepth,
                pixelDepth: screen.pixelDepth
            },
            viewport: {
                width: window.innerWidth,
                height: window.innerHeight
            }
        };
    }
}

// Global UI components instance
window.uiComponents = new UIComponents();

// Convenience methods
window.showToast = (message, type, duration) => window.uiComponents.showToast(message, type, duration);
window.showModal = (modalId, title, content) => window.uiComponents.showModal(modalId, title, content);
window.closeModal = (modal) => window.uiComponents.closeModal(modal);