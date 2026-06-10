/* ---------- state ---------- */
const STATE = {
  path: '/',
  entries: [],
};

/* ---------- API helpers ---------- */
const API = {
  files:  '/api/files',
  upload: '/api/upload',
  dir:    '/api/dir',
  status: '/api/status',
};

async function apiJson(url, opts = {}) {
  const r = await fetch(url, opts);
  if (!r.ok) {
    const body = await r.json().catch(() => ({ error: r.statusText }));
    throw new Error(body.error || `HTTP ${r.status}`);
  }
  return r.json();
}

/* ---------- toast ---------- */
let toastTimer;

function showToast(msg, type) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.className = type === 'error' ? 'error' : '';
  el.classList.remove('hidden');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.add('hidden'), 3000);
}

/* ---------- connection status ---------- */
function setConnected(ok) {
  const el = document.getElementById('conn-status');
  el.textContent = ok ? 'Connected' : 'Disconnected';
  el.className = 'status-dot ' + (ok ? 'connect' : 'disconnect');
}

/* ---------- breadcrumb ---------- */
function renderBreadcrumb(path) {
  const nav = document.getElementById('breadcrumb');
  nav.innerHTML = '';

  const parts = path.split('/').filter(Boolean);
  let acc = '';

  // Root link
  const rootLink = document.createElement('a');
  rootLink.href = '#';
  rootLink.textContent = 'root';
  rootLink.addEventListener('click', e => { e.preventDefault(); navigate('/'); });
  nav.appendChild(rootLink);

  for (const p of parts) {
    const sep = document.createElement('span');
    sep.className = 'sep';
    sep.textContent = ' / ';
    nav.appendChild(sep);

    acc += '/' + p;
    const link = document.createElement('a');
    link.href = '#';
    link.textContent = p;
    link.dataset.path = acc;
    link.addEventListener('click', e => { e.preventDefault(); navigate(acc); });
    nav.appendChild(link);
  }
}

/* ---------- file list ---------- */
function renderFiles(data) {
  const tbody = document.getElementById('file-body');
  const empty = document.getElementById('empty-state');
  tbody.innerHTML = '';

  if (!data.entries || data.entries.length === 0) {
    empty.classList.remove('hidden');
    document.getElementById('file-table').classList.add('hidden');
    return;
  }

  empty.classList.add('hidden');
  document.getElementById('file-table').classList.remove('hidden');

  for (const e of data.entries) {
    const tr = document.createElement('tr');

    // Name
    const tdName = document.createElement('td');
    const div = document.createElement('div');
    div.className = 'entry-name';

    const icon = document.createElement('span');
    icon.className = 'file-icon ' + (e.type === 'directory' ? 'dir' : 'file');
    icon.textContent = e.type === 'directory' ? '📁' : '📄';
    div.appendChild(icon);

    const nameLink = document.createElement('a');
    nameLink.href = '#';
    nameLink.textContent = e.name;
    if (e.type === 'directory') {
      nameLink.addEventListener('click', ev => {
        ev.preventDefault();
        const p = STATE.path === '/' ? '/' + e.name : STATE.path + '/' + e.name;
        navigate(p);
      });
    } else {
      nameLink.style.cursor = 'default';
      nameLink.addEventListener('click', ev => {
        ev.preventDefault();
        downloadFile(STATE.path === '/' ? '/' + e.name : STATE.path + '/' + e.name);
      });
    }
    div.appendChild(nameLink);
    tdName.appendChild(div);
    tr.appendChild(tdName);

    // Size
    const tdSize = document.createElement('td');
    tdSize.className = 'size-num';
    tdSize.textContent = e.type === 'directory' ? '—' : formatSize(e.size);
    tr.appendChild(tdSize);

    // Actions
    const tdAct = document.createElement('td');
    // Download button (files only)
    if (e.type === 'file') {
      const dlBtn = document.createElement('button');
      dlBtn.className = 'action-btn dl';
      dlBtn.textContent = 'Download';
      dlBtn.addEventListener('click', () => {
        const p = STATE.path === '/' ? '/' + e.name : STATE.path + '/' + e.name;
        downloadFile(p);
      });
      tdAct.appendChild(dlBtn);
    }
    // Delete button
    const delBtn = document.createElement('button');
    delBtn.className = 'action-btn del';
    delBtn.textContent = 'Delete';
    delBtn.addEventListener('click', () => {
      const p = STATE.path === '/' ? '/' + e.name : STATE.path + '/' + e.name;
      deleteFile(p);
    });
    tdAct.appendChild(delBtn);
    tdAct.style.textAlign = 'right';
    tr.appendChild(tdAct);

    tbody.appendChild(tr);
  }
}

function formatSize(bytes) {
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
  return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
}

/* ---------- navigation ---------- */
async function navigate(path) {
  STATE.path = path;

  try {
    const url = path === '/' ? API.files : `${API.files}${path}`;
    const data = await apiJson(url);
    STATE.entries = data.entries;
    renderBreadcrumb(path);
    renderFiles(data);
    setConnected(true);
  } catch (err) {
    showToast('Failed to load: ' + err.message, 'error');
    setConnected(false);
  }
}

/* ---------- upload ---------- */
function uploadFile(file, destPath) {
  const url = `${API.upload}${destPath}`;
  const xhr = new XMLHttpRequest();
  xhr.open('POST', url);
  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      // Upload progress could show in a more sophisticated UI
    }
  };
  xhr.onload = () => {
    if (xhr.status === 201) {
      showToast(`Uploaded: ${file.name}`);
      navigate(STATE.path);
    } else {
      let msg = 'Upload failed';
      try { const j = JSON.parse(xhr.responseText); msg = j.error; } catch {}
      showToast(msg, 'error');
    }
  };
  xhr.onerror = () => showToast('Upload error', 'error');
  xhr.send(file);
}

/* ---------- download ---------- */
function downloadFile(path) {
  const url = `${API.files}${path}?download=1`;
  const a = document.createElement('a');
  a.href = url;
  a.download = path.split('/').pop();
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}

/* ---------- delete ---------- */
async function deleteFile(path) {
  if (!confirm(`Delete "${path.split('/').pop()}"?`)) return;
  try {
    await apiJson(`${API.files}${path}`, { method: 'DELETE' });
    showToast('Deleted');
    navigate(STATE.path);
  } catch (err) {
    showToast('Delete failed: ' + err.message, 'error');
  }
}

/* ---------- create directory ---------- */
async function createDir(name) {
  const path = STATE.path === '/' ? '/' + name : STATE.path + '/' + name;
  try {
    await apiJson(`${API.dir}${path}`, { method: 'POST' });
    showToast('Folder created');
    navigate(STATE.path);
  } catch (err) {
    showToast('Failed: ' + err.message, 'error');
  }
}

/* ---------- status ---------- */
async function loadStatus() {
  try {
    const data = await apiJson(API.status);
    const el = document.getElementById('status-text');
    const free = formatSize(data.storage.free);
    const total = formatSize(data.storage.total);
    const heap = formatSize(data.memory.free_heap);
    el.textContent = `Uptime: ${data.uptime}s · Storage: ${free} free / ${total} · Heap: ${heap}`;
    setConnected(true);
  } catch {
    // Don't spam errors — status is best-effort
  }
}

/* ---------- drag & drop ---------- */
const dropOverlay  = document.getElementById('drop-overlay');
const uploadZone   = document.getElementById('upload-zone');
const fileInput    = document.getElementById('file-input');

function onDrop(e) {
  e.preventDefault();
  dropOverlay.classList.add('hidden');
  uploadZone.classList.remove('dragover');

  const files = e.dataTransfer.files;
  for (const f of files) {
    const dest = STATE.path === '/' ? '/' + f.name : STATE.path + '/' + f.name;
    uploadFile(f, dest);
  }
}

document.addEventListener('dragenter', (e) => {
  e.preventDefault();
  dropOverlay.classList.remove('hidden');
});
document.addEventListener('dragover', (e) => e.preventDefault());
document.addEventListener('dragleave', (e) => {
  if (e.relatedTarget === null) {
    dropOverlay.classList.add('hidden');
  }
});
document.addEventListener('drop', onDrop);

// Click on upload zone opens file browser
uploadZone.addEventListener('click', () => fileInput.click());
fileInput.addEventListener('change', () => {
  for (const f of fileInput.files) {
    const dest = STATE.path === '/' ? '/' + f.name : STATE.path + '/' + f.name;
    uploadFile(f, dest);
  }
  fileInput.value = '';
});

/* ---------- new folder dialog ---------- */
const dialog = document.getElementById('dialog-new-folder');
document.getElementById('btn-new-folder').addEventListener('click', () => {
  document.getElementById('new-folder-name').value = '';
  dialog.showModal();
});
dialog.addEventListener('close', () => {
  if (dialog.returnValue === 'ok') {
    const name = document.getElementById('new-folder-name').value.trim();
    if (name) createDir(name);
  }
});

/* ---------- init ---------- */
navigate('/');
loadStatus();
setInterval(loadStatus, 10000);
