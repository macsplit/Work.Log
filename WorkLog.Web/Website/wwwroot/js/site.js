// Work Log - Main JavaScript file

document.addEventListener('DOMContentLoaded', function() {
    initAjaxNavigation();
    initHierarchyScrollPersistence();
    renderSessionNotesMarkdown();
    initThemeSelector();
});

// Handle browser back/forward buttons
window.addEventListener('popstate', function(e) {
    if (e.state && e.state.ajax) {
        loadPageContent(location.href, false);
    }
});

// AJAX navigation to prevent dark mode flash
function initAjaxNavigation() {
    document.addEventListener('click', function(e) {
        // Find if click was on any internal link (not buttons, not form submits)
        const link = e.target.closest('a:not([target]):not([download])');
        if (!link) return;

        // Skip links inside forms or with special roles
        if (link.closest('form')) return;

        // Skip if modifier keys pressed (user wants new tab etc)
        if (e.ctrlKey || e.metaKey || e.shiftKey) return;

        const href = link.getAttribute('href');
        if (!href || href.startsWith('#') || href.startsWith('javascript:')) return;

        // Only handle same-origin navigation
        const url = new URL(href, location.origin);
        if (url.origin !== location.origin) return;

        e.preventDefault();
        loadPageContent(href, true);
    });
}

async function loadPageContent(url, pushState) {
    try {
        const response = await fetch(url);
        if (!response.ok) {
            // Fall back to regular navigation on error
            location.href = url;
            return;
        }

        const html = await response.text();
        const parser = new DOMParser();
        const doc = parser.parseFromString(html, 'text/html');

        // Check if both pages have the dashboard layout (hierarchy + content panels)
        const newHierarchy = doc.querySelector('.hierarchy-panel');
        const oldHierarchy = document.querySelector('.hierarchy-panel');
        const newContent = doc.querySelector('.content-panel');
        const oldContent = document.querySelector('.content-panel');

        if (newHierarchy && oldHierarchy && newContent && oldContent) {
            // Same layout - just update panels
            const scrollTop = oldHierarchy.scrollTop;
            oldHierarchy.innerHTML = newHierarchy.innerHTML;
            oldHierarchy.scrollTop = scrollTop;
            oldContent.innerHTML = newContent.innerHTML;
        } else {
            // Different layout - replace entire main content
            const newMain = doc.querySelector('.main-content');
            const oldMain = document.querySelector('.main-content');
            if (newMain && oldMain) {
                oldMain.innerHTML = newMain.innerHTML;
            }
        }

        // Re-render markdown in new content
        renderSessionNotesMarkdown();

        // Update page title
        const newTitle = doc.querySelector('title');
        if (newTitle) {
            document.title = newTitle.textContent;
        }

        // Update URL
        if (pushState) {
            history.pushState({ ajax: true }, '', url);
        }

        // Scroll to top for new pages
        if (!newHierarchy || !oldHierarchy) {
            window.scrollTo(0, 0);
        }
    } catch (error) {
        console.error('AJAX navigation failed:', error);
        location.href = url;
    }
}

// Keep the hierarchy panel scroll position while navigating between dates
function initHierarchyScrollPersistence() {
    const panel = document.querySelector('.hierarchy-panel');
    if (!panel || typeof sessionStorage === 'undefined') {
        return;
    }

    const storageKey = 'hierarchy-panel-scroll-top';
    const savedPosition = sessionStorage.getItem(storageKey);
    if (savedPosition !== null) {
        const numericPosition = parseInt(savedPosition, 10);
        if (!Number.isNaN(numericPosition)) {
            panel.scrollTop = numericPosition;
        }
    }

    const persistScroll = () => sessionStorage.setItem(storageKey, panel.scrollTop.toString());
    panel.addEventListener('scroll', persistScroll);
    panel.querySelectorAll('a').forEach((link) => link.addEventListener('click', persistScroll));
    window.addEventListener('beforeunload', persistScroll);
}

// Modal functions
function openModal(title, content) {
    document.getElementById('modal-title').textContent = title;
    document.getElementById('modal-content').innerHTML = content;
    document.getElementById('modal-backdrop').style.display = 'flex';
    document.body.style.overflow = 'hidden';
}

function closeModal() {
    document.getElementById('modal-backdrop').style.display = 'none';
    document.body.style.overflow = '';
}

// Close modal when clicking backdrop
document.addEventListener('click', function(e) {
    if (e.target.id === 'modal-backdrop') {
        closeModal();
    }
});

// Close modal with Escape key
document.addEventListener('keydown', function(e) {
    if (e.key === 'Escape') {
        closeModal();
    }
});

// Tag management modal
async function openTagManager() {
    try {
        const response = await fetch('/Tag/GetTagsJson');
        const tags = await response.json();

        let tagListHtml = '';
        if (tags.length === 0) {
            tagListHtml = '<p class="empty-message">No tags yet. Create one above to get started.</p>';
        } else {
            tagListHtml = '<div class="tag-list">';
            for (const tag of tags) {
                tagListHtml += `
                    <div class="tag-item" id="tag-item-${tag.id}">
                        <span class="tag-badge">${escapeHtml(tag.name)}</span>
                        <button type="button" class="btn btn-sm btn-danger" onclick="deleteTag(${tag.id}, '${escapeHtml(tag.name)}')">Delete</button>
                    </div>
                `;
            }
            tagListHtml += '</div>';
        }

        const content = `
            <p class="form-hint">Tags help you categorize your work sessions.</p>
            <div class="tag-create-form">
                <div class="tag-input-row">
                    <input type="text" id="new-tag-name" class="form-control" placeholder="Enter new tag name..." />
                    <button type="button" class="btn btn-primary" onclick="createTag()">Add Tag</button>
                </div>
            </div>
            <div id="tag-list-container">
                ${tagListHtml}
            </div>
        `;

        openModal('Manage Tags', content);

        // Focus the input and add enter key handler
        const input = document.getElementById('new-tag-name');
        input.focus();
        input.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                createTag();
            }
        });
    } catch (error) {
        console.error('Failed to load tags:', error);
        alert('Failed to load tags. Please try again.');
    }
}

async function createTag() {
    const input = document.getElementById('new-tag-name');
    const name = input.value.trim();

    if (!name) {
        return;
    }

    try {
        const formData = new FormData();
        formData.append('name', name);

        // Get anti-forgery token from the page
        const token = document.querySelector('input[name="__RequestVerificationToken"]')?.value;
        if (token) {
            formData.append('__RequestVerificationToken', token);
        }

        const response = await fetch('/Tag/Create', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            input.value = '';
            // Refresh the tag list in modal and dropdown
            await refreshTagList();
            refreshTagDropdown();
        } else {
            alert('Failed to create tag. It may already exist.');
        }
    } catch (error) {
        console.error('Failed to create tag:', error);
        alert('Failed to create tag. Please try again.');
    }
}

async function deleteTag(id, name) {
    if (!confirm(`Delete tag "${name}"? Sessions using this tag will have their tag removed.`)) {
        return;
    }

    try {
        const formData = new FormData();
        formData.append('id', id);

        const token = document.querySelector('input[name="__RequestVerificationToken"]')?.value;
        if (token) {
            formData.append('__RequestVerificationToken', token);
        }

        const response = await fetch(`/Tag/Delete/${id}`, {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            // Remove from modal list
            const tagItem = document.getElementById(`tag-item-${id}`);
            if (tagItem) {
                tagItem.remove();
            }

            // Check if list is empty
            const tagList = document.querySelector('#tag-list-container .tag-list');
            if (tagList && tagList.children.length === 0) {
                document.getElementById('tag-list-container').innerHTML =
                    '<p class="empty-message">No tags yet. Create one above to get started.</p>';
            }

            refreshTagDropdown();
        } else {
            alert('Failed to delete tag. Please try again.');
        }
    } catch (error) {
        console.error('Failed to delete tag:', error);
        alert('Failed to delete tag. Please try again.');
    }
}

async function refreshTagList() {
    try {
        const response = await fetch('/Tag/GetTagsJson');
        const tags = await response.json();

        let tagListHtml = '';
        if (tags.length === 0) {
            tagListHtml = '<p class="empty-message">No tags yet. Create one above to get started.</p>';
        } else {
            tagListHtml = '<div class="tag-list">';
            for (const tag of tags) {
                tagListHtml += `
                    <div class="tag-item" id="tag-item-${tag.id}">
                        <span class="tag-badge">${escapeHtml(tag.name)}</span>
                        <button type="button" class="btn btn-sm btn-danger" onclick="deleteTag(${tag.id}, '${escapeHtml(tag.name)}')">Delete</button>
                    </div>
                `;
            }
            tagListHtml += '</div>';
        }

        document.getElementById('tag-list-container').innerHTML = tagListHtml;
    } catch (error) {
        console.error('Failed to refresh tag list:', error);
    }
}

async function refreshTagDropdown() {
    const dropdown = document.getElementById('TagId');
    if (!dropdown) return;

    const currentValue = dropdown.value;

    try {
        const response = await fetch('/Tag/GetTagsJson');
        const tags = await response.json();

        // Clear and rebuild options
        dropdown.innerHTML = '<option value="">No tag</option>';
        for (const tag of tags) {
            const option = document.createElement('option');
            option.value = tag.id;
            option.textContent = tag.name;
            if (tag.id.toString() === currentValue) {
                option.selected = true;
            }
            dropdown.appendChild(option);
        }
    } catch (error) {
        console.error('Failed to refresh tag dropdown:', error);
    }
}

function renderSessionNotesMarkdown() {
    if (!window.markdownit || !window.hljs) {
        console.warn('Markdown rendering skipped because supporting libraries are missing.');
        return;
    }

    const md = window.markdownit({
        html: false,
        linkify: true,
        breaks: true,
        highlight: function(str, lang) {
            try {
                if (lang && hljs.getLanguage(lang)) {
                    return `<pre class="hljs"><code>${hljs.highlight(str, { language: lang, ignoreIllegals: true }).value}</code></pre>`;
                }

                return `<pre class="hljs"><code>${hljs.highlightAuto(str).value}</code></pre>`;
            } catch (error) {
                const safe = md.utils.escapeHtml(str);
                return `<pre class="hljs"><code>${safe}</code></pre>`;
            }
        }
    });

    document.querySelectorAll('.session-notes-content').forEach((el) => {
        const raw = el.textContent || '';
        if (!raw.trim()) {
            return;
        }

        const rendered = md.render(raw);
        el.innerHTML = rendered;
    });
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Theme management
function initThemeSelector() {
    const selector = document.getElementById('theme-selector');
    if (!selector) return;

    // Set the selected option based on the current theme
    const currentTheme = document.body.getAttribute('data-theme') || '';
    selector.value = currentTheme;
}

async function changeTheme(theme) {
    // Apply theme immediately
    if (theme) {
        document.body.setAttribute('data-theme', theme);
    } else {
        document.body.removeAttribute('data-theme');
    }

    // Save theme preference to server
    try {
        const formData = new FormData();
        formData.append('theme', theme || '');

        const token = document.querySelector('input[name="__RequestVerificationToken"]')?.value;
        if (token) {
            formData.append('__RequestVerificationToken', token);
        }

        await fetch('/Auth/SetTheme', {
            method: 'POST',
            body: formData
        });
    } catch (error) {
        console.error('Failed to save theme preference:', error);
    }
}
