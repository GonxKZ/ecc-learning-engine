/**
 * ECScope Interactive Documentation - Main JavaScript
 * Handles navigation, theme switching, search, and core interactions
 */

class ECScope_Documentation {
    constructor() {
        this.currentSection = 'overview';
        this.theme = this.getStoredTheme() || 'light';
        this.searchIndex = [];
        this.navigationHistory = [];
        
        this.initializeComponents();
        this.setupEventListeners();
        this.loadContent();
    }

    initializeComponents() {
        // Set initial theme
        document.documentElement.setAttribute('data-theme', this.theme);
        this.updateThemeButton();
        
        // Hide loading overlay after initialization
        setTimeout(() => {
            const loading = document.getElementById('loading');
            if (loading) {
                loading.classList.add('hidden');
            }
        }, 1000);
        
        // Initialize search
        this.buildSearchIndex();
        
        // Set up intersection observer for navigation highlighting
        this.setupIntersectionObserver();
    }

    setupEventListeners() {
        // Theme toggle
        const themeToggle = document.getElementById('theme-toggle');
        if (themeToggle) {
            themeToggle.addEventListener('click', () => this.toggleTheme());
        }

        // Search functionality
        const searchInput = document.getElementById('search-input');
        if (searchInput) {
            searchInput.addEventListener('input', (e) => this.handleSearch(e.target.value));
            searchInput.addEventListener('keydown', (e) => this.handleSearchKeydown(e));
        }

        // Navigation links
        document.addEventListener('click', (e) => {
            if (e.target.matches('[data-navigate]')) {
                e.preventDefault();
                const target = e.target.getAttribute('data-navigate');
                this.navigateToSection(target);
            }

            if (e.target.matches('.nav-link, .sidebar-link')) {
                e.preventDefault();
                const href = e.target.getAttribute('href');
                if (href && href.startsWith('#')) {
                    this.navigateToSection(href.substring(1));
                }
            }

            // Copy button functionality
            if (e.target.matches('.copy-button') || e.target.closest('.copy-button')) {
                e.preventDefault();
                const button = e.target.closest('.copy-button');
                this.copyToClipboard(button);
            }

            // Tab functionality
            if (e.target.matches('.tab-button')) {
                this.switchTab(e.target);
            }

            // Action cards
            if (e.target.matches('.action-card') || e.target.closest('.action-card')) {
                const card = e.target.closest('.action-card');
                const target = card.getAttribute('data-navigate');
                if (target) {
                    this.navigateToSection(target);
                }
            }

            // Next step cards
            if (e.target.matches('.next-step-card') || e.target.closest('.next-step-card')) {
                const card = e.target.closest('.next-step-card');
                const target = card.getAttribute('data-navigate');
                if (target) {
                    this.navigateToSection(target);
                }
            }
        });

        // Handle browser back/forward
        window.addEventListener('popstate', (e) => {
            if (e.state && e.state.section) {
                this.navigateToSection(e.state.section, false);
            }
        });

        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            // Ctrl/Cmd + K for search
            if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
                e.preventDefault();
                const searchInput = document.getElementById('search-input');
                if (searchInput) {
                    searchInput.focus();
                }
            }

            // Escape to close modals
            if (e.key === 'Escape') {
                this.closeAllModals();
            }
        });

        // Responsive sidebar toggle
        this.setupResponsiveSidebar();
    }

    setupResponsiveSidebar() {
        // Add mobile menu button if needed
        if (window.innerWidth <= 768) {
            this.addMobileMenuButton();
        }

        window.addEventListener('resize', () => {
            if (window.innerWidth <= 768 && !document.querySelector('.mobile-menu-button')) {
                this.addMobileMenuButton();
            } else if (window.innerWidth > 768) {
                const mobileButton = document.querySelector('.mobile-menu-button');
                if (mobileButton) {
                    mobileButton.remove();
                }
                const sidebar = document.querySelector('.sidebar');
                if (sidebar) {
                    sidebar.classList.remove('open');
                }
            }
        });
    }

    addMobileMenuButton() {
        const header = document.querySelector('.header-container');
        if (!header || document.querySelector('.mobile-menu-button')) return;

        const mobileButton = document.createElement('button');
        mobileButton.className = 'mobile-menu-button';
        mobileButton.innerHTML = '<i class="fas fa-bars"></i>';
        mobileButton.addEventListener('click', this.toggleMobileSidebar.bind(this));

        header.insertBefore(mobileButton, header.firstChild);
    }

    toggleMobileSidebar() {
        const sidebar = document.querySelector('.sidebar');
        if (sidebar) {
            sidebar.classList.toggle('open');
        }
    }

    setupIntersectionObserver() {
        const sections = document.querySelectorAll('.content-section');
        const navLinks = document.querySelectorAll('.sidebar-link');

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting && entry.intersectionRatio > 0.3) {
                    const id = entry.target.id;
                    this.highlightNavigation(id);
                }
            });
        }, {
            threshold: [0, 0.3, 0.7],
            rootMargin: '-100px 0px -300px 0px'
        });

        sections.forEach(section => {
            observer.observe(section);
        });
    }

    highlightNavigation(activeId) {
        // Remove all active states
        document.querySelectorAll('.nav-link.active, .sidebar-link.active').forEach(link => {
            link.classList.remove('active');
        });

        // Add active state to matching links
        document.querySelectorAll(`[href="#${activeId}"]`).forEach(link => {
            link.classList.add('active');
        });

        this.currentSection = activeId;
    }

    navigateToSection(sectionId, updateHistory = true) {
        const section = document.getElementById(sectionId);
        if (!section) {
            console.warn(`Section '${sectionId}' not found`);
            return;
        }

        // Update URL and history
        if (updateHistory) {
            const url = new URL(window.location);
            url.hash = sectionId;
            window.history.pushState({ section: sectionId }, '', url);
        }

        // Scroll to section with offset for fixed header
        const headerHeight = document.querySelector('.header').offsetHeight;
        const sectionTop = section.offsetTop - headerHeight - 20;

        window.scrollTo({
            top: sectionTop,
            behavior: 'smooth'
        });

        // Close mobile sidebar if open
        const sidebar = document.querySelector('.sidebar');
        if (sidebar && sidebar.classList.contains('open')) {
            sidebar.classList.remove('open');
        }

        // Load section content if needed
        this.loadSectionContent(sectionId);
    }

    async loadSectionContent(sectionId) {
        // Dynamic content loading for tutorials and API docs
        const section = document.getElementById(sectionId);
        if (!section || section.dataset.loaded) return;

        const contentMap = {
            'tutorial-basic-ecs': 'tutorials/basic-ecs.html',
            'tutorial-memory-lab': 'tutorials/memory-lab.html',
            'tutorial-physics': 'tutorials/physics.html',
            'tutorial-rendering': 'tutorials/rendering.html',
            'tutorial-advanced': 'tutorials/advanced.html',
            'api-core': 'api/core.html',
            'api-ecs': 'api/ecs.html',
            'api-physics': 'api/physics.html',
            'api-rendering': 'api/rendering.html',
            'api-memory': 'api/memory.html',
            'benchmarks': 'performance/benchmarks.html'
        };

        const contentFile = contentMap[sectionId];
        if (contentFile) {
            try {
                const response = await fetch(contentFile);
                if (response.ok) {
                    const html = await response.text();
                    section.innerHTML = html;
                    section.dataset.loaded = 'true';
                    
                    // Initialize any JavaScript components in the loaded content
                    this.initializeSectionComponents(section);
                }
            } catch (error) {
                console.warn(`Failed to load content for ${sectionId}:`, error);
                this.loadFallbackContent(section, sectionId);
            }
        }
    }

    loadFallbackContent(section, sectionId) {
        // Generate fallback content based on section type
        const fallbackContent = this.generateFallbackContent(sectionId);
        if (fallbackContent) {
            section.innerHTML = fallbackContent;
            section.dataset.loaded = 'true';
        }
    }

    generateFallbackContent(sectionId) {
        const contentGenerators = {
            'tutorial-basic-ecs': () => this.generateBasicECSTutorial(),
            'tutorial-memory-lab': () => this.generateMemoryLabTutorial(),
            'api-core': () => this.generateCoreAPIDoc(),
            'benchmarks': () => this.generateBenchmarkInterface()
        };

        const generator = contentGenerators[sectionId];
        return generator ? generator() : null;
    }

    initializeSectionComponents(section) {
        // Initialize code highlighting
        if (typeof Prism !== 'undefined') {
            Prism.highlightAllUnder(section);
        }

        // Initialize interactive components
        const codeRunners = section.querySelectorAll('.code-runner');
        codeRunners.forEach(runner => this.initializeCodeRunner(runner));

        const sliders = section.querySelectorAll('.interactive-slider');
        sliders.forEach(slider => this.initializeSlider(slider));

        const charts = section.querySelectorAll('.performance-chart');
        charts.forEach(chart => this.initializeChart(chart));
    }

    // Theme Management
    toggleTheme() {
        this.theme = this.theme === 'light' ? 'dark' : 'light';
        document.documentElement.setAttribute('data-theme', this.theme);
        this.updateThemeButton();
        this.storeTheme(this.theme);
    }

    updateThemeButton() {
        const button = document.getElementById('theme-toggle');
        if (button) {
            const icon = button.querySelector('i');
            if (icon) {
                icon.className = this.theme === 'light' ? 'fas fa-moon' : 'fas fa-sun';
            }
            button.title = `Switch to ${this.theme === 'light' ? 'dark' : 'light'} theme`;
        }
    }

    getStoredTheme() {
        return localStorage.getItem('ecscope-docs-theme');
    }

    storeTheme(theme) {
        localStorage.setItem('ecscope-docs-theme', theme);
    }

    // Search Functionality
    buildSearchIndex() {
        const sections = document.querySelectorAll('.content-section');
        this.searchIndex = [];

        sections.forEach(section => {
            const id = section.id;
            const title = section.querySelector('h1, h2')?.textContent || '';
            const content = section.textContent || '';
            
            this.searchIndex.push({
                id,
                title,
                content: content.toLowerCase(),
                element: section
            });
        });
    }

    handleSearch(query) {
        if (query.length < 2) {
            this.hideSearchResults();
            return;
        }

        const results = this.searchIndex.filter(item => 
            item.title.toLowerCase().includes(query.toLowerCase()) ||
            item.content.includes(query.toLowerCase())
        ).slice(0, 8);

        this.showSearchResults(results, query);
    }

    showSearchResults(results, query) {
        let resultsContainer = document.getElementById('search-results');
        
        if (!resultsContainer) {
            resultsContainer = document.createElement('div');
            resultsContainer.id = 'search-results';
            resultsContainer.className = 'search-results';
            
            const searchContainer = document.querySelector('.search-container');
            searchContainer.appendChild(resultsContainer);
        }

        if (results.length === 0) {
            resultsContainer.innerHTML = '<div class="search-no-results">No results found</div>';
        } else {
            resultsContainer.innerHTML = results.map(result => `
                <div class="search-result" data-navigate="${result.id}">
                    <div class="search-result-title">${result.title}</div>
                    <div class="search-result-preview">${this.generatePreview(result.content, query)}</div>
                </div>
            `).join('');
        }

        resultsContainer.classList.add('visible');
    }

    hideSearchResults() {
        const resultsContainer = document.getElementById('search-results');
        if (resultsContainer) {
            resultsContainer.classList.remove('visible');
        }
    }

    generatePreview(content, query) {
        const queryIndex = content.toLowerCase().indexOf(query.toLowerCase());
        if (queryIndex === -1) return content.substring(0, 100) + '...';

        const start = Math.max(0, queryIndex - 50);
        const end = Math.min(content.length, queryIndex + query.length + 50);
        let preview = content.substring(start, end);

        if (start > 0) preview = '...' + preview;
        if (end < content.length) preview = preview + '...';

        return preview;
    }

    handleSearchKeydown(e) {
        if (e.key === 'Escape') {
            this.hideSearchResults();
            e.target.blur();
        } else if (e.key === 'ArrowDown') {
            e.preventDefault();
            this.focusSearchResult(0);
        }
    }

    focusSearchResult(index) {
        const results = document.querySelectorAll('.search-result');
        if (results[index]) {
            results[index].focus();
        }
    }

    // Utility Functions
    copyToClipboard(button) {
        const code = button.getAttribute('data-copy') || 
                    button.closest('.code-example')?.querySelector('code')?.textContent ||
                    '';

        if (navigator.clipboard) {
            navigator.clipboard.writeText(code).then(() => {
                this.showCopyFeedback(button);
            }).catch(err => {
                console.warn('Failed to copy:', err);
                this.fallbackCopy(code, button);
            });
        } else {
            this.fallbackCopy(code, button);
        }
    }

    fallbackCopy(text, button) {
        const textArea = document.createElement('textarea');
        textArea.value = text;
        document.body.appendChild(textArea);
        textArea.select();
        
        try {
            document.execCommand('copy');
            this.showCopyFeedback(button);
        } catch (err) {
            console.warn('Fallback copy failed:', err);
        }
        
        document.body.removeChild(textArea);
    }

    showCopyFeedback(button) {
        const originalContent = button.innerHTML;
        button.innerHTML = '<i class="fas fa-check"></i>';
        button.classList.add('copied');
        
        setTimeout(() => {
            button.innerHTML = originalContent;
            button.classList.remove('copied');
        }, 1500);
    }

    switchTab(tabButton) {
        const container = tabButton.closest('.tab-container');
        if (!container) return;

        // Remove active states
        container.querySelectorAll('.tab-button').forEach(btn => 
            btn.classList.remove('active'));
        container.querySelectorAll('.tab-content').forEach(content => 
            content.classList.remove('active'));

        // Add active state to clicked tab
        tabButton.classList.add('active');
        
        const targetId = tabButton.getAttribute('data-tab');
        const targetContent = container.querySelector(`#${targetId}`);
        if (targetContent) {
            targetContent.classList.add('active');
        }
    }

    closeAllModals() {
        document.querySelectorAll('.modal.active').forEach(modal => {
            modal.classList.remove('active');
        });
    }

    // Load initial content
    async loadContent() {
        // Check for URL hash and navigate to section
        const hash = window.location.hash;
        if (hash && hash.length > 1) {
            const sectionId = hash.substring(1);
            setTimeout(() => {
                this.navigateToSection(sectionId, false);
            }, 500);
        }

        // Load any dynamic content needed for the initial view
        await this.loadCriticalContent();
    }

    async loadCriticalContent() {
        // Load essential content that should be available immediately
        // This could include API definitions, code examples, etc.
        
        try {
            // Example: Load API definitions
            const apiResponse = await fetch('data/api-definitions.json');
            if (apiResponse.ok) {
                this.apiDefinitions = await apiResponse.json();
            }
        } catch (error) {
            console.warn('Failed to load critical content:', error);
        }
    }
}

// Initialize the documentation system when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.ecscopeDoc = new ECScope_Documentation();
});

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ECScope_Documentation;
}