/**
 * Paperless Office & Student Portal - Frontend Logic
 * Connects to C Winsock Backend (Port 8080) with Local Database (records.db)
 * Graceful fallback to browser localStorage if server is offline
 */

const API_BASE = "http://localhost:8080";
let allRecords = [];
let isBackendOnline = false;

// Initialize on page load
document.addEventListener("DOMContentLoaded", () => {
    detectNetworkHost();
    checkBackendAndLoad();
});

// Detect network host for other students sharing
function detectNetworkHost() {
    const host = window.location.hostname;
    const port = window.location.port || "8080";
    const networkElem = document.getElementById("networkUrl");
    if (host && host !== "localhost" && host !== "127.0.0.1" && host !== "") {
        networkElem.textContent = `http://${host}:${port}`;
    } else {
        networkElem.textContent = `http://10.213.0.200:8080`;
    }
}

// Copy Network URL to clipboard
function copyNetworkUrl() {
    const url = document.getElementById("networkUrl").textContent;
    navigator.clipboard.writeText(url).then(() => {
        alert("🌐 Link copied to clipboard!\nShare this with any other student on the same Wi-Fi:\n" + url);
    }).catch(() => {
        prompt("Copy this link to share with other students:", url);
    });
}

// Check Backend Connection and Load Documents
async function checkBackendAndLoad() {
    updateStatusBadge("connecting", "Connecting to C Backend...");

    try {
        const response = await fetch(`${API_BASE}/api/stats`, { cache: "no-store" });
        if (response.ok) {
            isBackendOnline = true;
            updateStatusBadge("online", "Connected to C Backend (records.db)");
            await loadDocuments();
            return;
        }
    } catch (e) {
        // Backend not running on 8080
    }

    // Fallback to localStorage if backend is not running
    isBackendOnline = false;
    updateStatusBadge("offline", "C Backend Offline (Using Local Storage)");
    loadFromLocalStorage();
}

// Update connection status badge
function updateStatusBadge(status, text) {
    const badge = document.getElementById("backendStatus");
    const statusText = document.getElementById("statusText");
    badge.className = `status-badge ${status}`;
    statusText.textContent = text;
}

// Fetch documents from C backend
async function loadDocuments() {
    if (!isBackendOnline) {
        loadFromLocalStorage();
        return;
    }

    try {
        const res = await fetch(`${API_BASE}/api/documents`, { cache: "no-store" });
        if (!res.ok) throw new Error("Failed to fetch documents");
        allRecords = await res.json();
        renderDocuments(allRecords);
        updateStats();
    } catch (err) {
        console.warn("Backend error, falling back to local storage:", err);
        isBackendOnline = false;
        updateStatusBadge("offline", "Backend Disconnected (Using Local Storage)");
        loadFromLocalStorage();
    }
}

// Load documents from browser localStorage
function loadFromLocalStorage() {
    const stored = localStorage.getItem("paperless_records");
    if (stored) {
        try {
            allRecords = JSON.parse(stored);
        } catch (e) {
            allRecords = [];
        }
    } else {
        // Initial sample seed if empty
        allRecords = [
            {
                id: 101,
                student_name: "Rahul Sharma",
                student_id: "R24CS101",
                doc_name: "Data Structures Lab Report",
                category: "Lab Report",
                doc_note: "Completed Binary Search Tree & AVL Tree implementations with test benchmarks.",
                created_at: "2026-11-18 10:15:00"
            },
            {
                id: 102,
                student_name: "Priya Nair",
                student_id: "R24EC204",
                doc_name: "Digital Electronics Notes",
                category: "Notes",
                doc_note: "Unit 3 Logic Gates and Karnaugh Map simplification cheat-sheet.",
                created_at: "2026-11-18 11:30:00"
            }
        ];
        localStorage.setItem("paperless_records", JSON.stringify(allRecords));
    }
    renderDocuments(allRecords);
    updateStats();
}

// Handle Form Submission (Save Document)
async function handleFormSubmit(event) {
    event.preventDefault();

    const student_name = document.getElementById("studentName").value.trim();
    const student_id = document.getElementById("studentId").value.trim() || "N/A";
    const doc_name = document.getElementById("docName").value.trim();
    const category = document.getElementById("category").value;
    const doc_note = document.getElementById("docNote").value.trim();

    if (!student_name || !doc_name || !doc_note) {
        alert("Please fill in Student Name, Document Title, and Document Content.");
        return;
    }

    const newRecord = {
        student_name,
        student_id,
        doc_name,
        category,
        doc_note
    };

    if (isBackendOnline) {
        try {
            const res = await fetch(`${API_BASE}/api/documents`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(newRecord)
            });

            if (res.ok) {
                resetForm();
                await loadDocuments();
                return;
            }
        } catch (e) {
            console.error("Save error:", e);
        }
    }

    // Fallback: save locally in browser
    const localId = Date.now() % 10000;
    const dateStr = new Date().toISOString().replace("T", " ").substring(0, 19);
    allRecords.unshift({
        id: localId,
        ...newRecord,
        created_at: dateStr
    });
    localStorage.setItem("paperless_records", JSON.stringify(allRecords));
    resetForm();
    renderDocuments(allRecords);
    updateStats();
}

// Delete Document
async function deleteDocument(id) {
    if (!confirm(`Are you sure you want to delete document #${id}?`)) return;

    if (isBackendOnline) {
        try {
            const res = await fetch(`${API_BASE}/api/delete`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ id })
            });

            if (res.ok) {
                await loadDocuments();
                return;
            }
        } catch (e) {
            console.error("Delete error:", e);
        }
    }

    // Local fallback delete
    allRecords = allRecords.filter(item => item.id !== id);
    localStorage.setItem("paperless_records", JSON.stringify(allRecords));
    renderDocuments(allRecords);
    updateStats();
}

// Reset / Seed Demo Records
async function seedDemoRecords() {
    if (!confirm("Reset database to sample student records?")) return;

    if (isBackendOnline) {
        try {
            await fetch(`${API_BASE}/api/reset`, { method: "POST" });
            await loadDocuments();
            return;
        } catch (e) {
            console.error("Reset error:", e);
        }
    }

    localStorage.removeItem("paperless_records");
    loadFromLocalStorage();
}

// Render Document Cards to DOM
function renderDocuments(list) {
    const container = document.getElementById("fileList");
    const emptyMsg = document.getElementById("emptyMessage");
    container.innerHTML = "";

    if (!list || list.length === 0) {
        emptyMsg.style.display = "block";
        return;
    }
    emptyMsg.style.display = "none";

    list.forEach(doc => {
        const card = document.createElement("div");
        card.className = "doc-card";

        const catTag = doc.category || "General";
        const rollDisplay = doc.student_id && doc.student_id !== "N/A"
            ? `<span class="student-roll">Roll: ${escapeHtml(doc.student_id)}</span>`
            : "";

        card.innerHTML = `
            <div>
                <div class="doc-header">
                    <span class="doc-id-badge">#${doc.id}</span>
                    <span class="category-tag">${escapeHtml(catTag)}</span>
                </div>
                <h3 class="doc-title">${escapeHtml(doc.doc_name)}</h3>
                <div class="student-author">
                    <span>👤 <strong>${escapeHtml(doc.student_name || "Anonymous")}</strong></span>
                    ${rollDisplay}
                </div>
                <div class="doc-body">${escapeHtml(doc.doc_note || "")}</div>
            </div>
            <div class="doc-footer">
                <span>🕒 ${escapeHtml(doc.created_at || "Just now")}</span>
                <button class="btn-delete" onclick="deleteDocument(${doc.id})">🗑️ Delete</button>
            </div>
        `;

        container.appendChild(card);
    });
}

// Filter and Search
function filterDocuments() {
    const query = document.getElementById("searchInput").value.toLowerCase().trim();
    const category = document.getElementById("categoryFilter").value;

    const filtered = allRecords.filter(doc => {
        const matchCategory = (category === "ALL") || (doc.category === category);
        const matchSearch = !query ||
            (doc.doc_name && doc.doc_name.toLowerCase().includes(query)) ||
            (doc.student_name && doc.student_name.toLowerCase().includes(query)) ||
            (doc.student_id && doc.student_id.toLowerCase().includes(query)) ||
            (doc.doc_note && doc.doc_note.toLowerCase().includes(query));

        return matchCategory && matchSearch;
    });

    renderDocuments(filtered);
}

// Update Stats in UI
function updateStats() {
    document.getElementById("statDocs").textContent = allRecords.length;

    // Unique students count
    const studentSet = new Set(allRecords.map(r => r.student_name).filter(Boolean));
    document.getElementById("statStudents").textContent = studentSet.size;

    document.getElementById("statStorage").textContent = isBackendOnline ? "records.db (C Engine)" : "localStorage (Browser)";
}

// Reset Form Inputs
function resetForm() {
    document.getElementById("studentName").value = "";
    document.getElementById("studentId").value = "";
    document.getElementById("docName").value = "";
    document.getElementById("docNote").value = "";
    document.getElementById("category").selectedIndex = 0;
}

// Helper: Escape HTML
function escapeHtml(str) {
    if (!str) return "";
    return String(str)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}
