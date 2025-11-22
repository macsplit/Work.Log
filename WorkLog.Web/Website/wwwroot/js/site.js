// Work Log - Main JavaScript file

document.addEventListener('DOMContentLoaded', function() {
    console.log('Work Log application loaded');
    renderSessionNotesMarkdown();
});

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
