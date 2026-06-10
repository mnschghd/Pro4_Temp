# API Reference

> HTTP API for the esp32s3-webdrive file server. Base: `http://192.168.4.1/`

---

## 1. Service the Web UI

```
GET /
```

Serves the embedded single-page application (`index.html`).

---

## 2. File Endpoints

### 2.1 List Directory

```
GET /api/files
GET /api/files/{path}
```

**Response `200 OK`**

```json
{
  "path": "/",
  "entries": [
    {
      "name": "uploads",
      "type": "directory",
      "size": null,
      "modified": "2025-01-15T10:30:00Z"
    },
    {
      "name": "readme.txt",
      "type": "file",
      "size": 1234,
      "modified": "2025-01-14T08:15:00Z"
    }
  ]
}
```

### 2.2 Download a File

```
GET /api/files/{path}?download
```

Streams file content as `application/octet-stream` with `Content-Disposition: attachment`.

**cURL**

```bash
curl -o file.txt http://192.168.4.1/api/files/notes.txt?download
```

### 2.3 Upload a File

```
POST /api/upload/{path}
```

Body must be raw binary content.

**cURL**

```bash
curl -X POST http://192.168.4.1/api/upload/document.pdf \
  --data-binary @document.pdf \
  -H "Content-Type: application/octet-stream"
```

**Response `201 Created`**

```json
{
  "status": "ok",
  "path": "/uploads/document.pdf",
  "size": 12345
}
```

### 2.4 Delete a File or Directory

```
DELETE /api/files/{path}
```

Deletes a file or an empty directory.

**Response `200 OK`**

```json
{
  "status": "deleted",
  "path": "/old-file.txt"
}
```

### 2.5 Create a Directory

```
POST /api/dir/{path}
```

Creates a directory (and any intermediate directories).

**Response `201 Created`**

```json
{
  "status": "created",
  "path": "/new-folder"
}
```

---

## 3. Status

### 3.1 Device Status

```
GET /api/status
```

```json
{
  "device": "ESP32-S3",
  "uptime_seconds": 3600,
  "wifi": {
    "mode": "AP",
    "ssid": "ESP32S3-WebDrive",
    "connected_clients": 1,
    "ip": "192.168.4.1"
  },
  "storage": {
    "total_bytes": 2097152,
    "used_bytes": 524288,
    "free_bytes": 1572864,
    "filesystem": "littlefs"
  },
  "memory": {
    "free_heap": 240000,
    "psram_size": 8388608,
    "free_psram": 6356992
  }
}
```

---

## 4. Error Format

```json
{
  "error": "Human-readable message",
  "code": "ERROR_CODE"
}
```

| Code | Meaning |
|------|---------|
| `PATH_TRAVERSAL` | `../` detected |
| `FILE_EXISTS` | File already exists |
| `FILE_TOO_LARGE` | Exceeds upload size limit |
| `STORAGE_FULL` | No space left |
| `FS_ERROR` | Filesystem error |
| `INVALID_PATH` | Invalid characters |

---

## 5. Summary

| Method | Route | Purpose |
|--------|-------|---------|
| `GET` | `/` | Web UI |
| `GET` | `/api/files` | List root directory |
| `GET` | `/api/files/*` | List subdirectory or download (with `?download`) |
| `POST` | `/api/upload/*` | Upload a file |
| `DELETE` | `/api/files/*` | Delete a file or directory |
| `POST` | `/api/dir/*` | Create a directory and parents |
| `GET` | `/api/status` | Device status |
