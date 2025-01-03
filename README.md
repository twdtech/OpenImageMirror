# OpenImageMirror

## Overview
OpenImageMirror is a lightweight, flexible ISO file management and publishing API server designed to help you organize, catalog, and serve ISO files across your network. This tool provides a robust backend for listing, categorizing, and accessing ISO images with minimal configuration.

## Features ✨
### Key Capabilities:

- 📂 Automatic ISO file discovery
- 🔍 Recursive directory scanning
- 🏷️ Automatic category generation
- 💾 File metadata tracking
- 🚀 RESTful API for ISO listing
- 📦 Caching mechanism
- 🔒 Configurable logging

## Requirements
- C compiler (gcc/clang)
- libmicrohttpd (for API server)
- json-c library
- POSIX-compliant system (linux with systemd at best for automatic service installation)

## Configuration
Create a config/config.json with the following structure:

```json
{
    "iso_directory": "/path/to/image/files",
    "recursive_scan": true,
    "scan_interval": 300,
    "api_port": 8080,
    "log_file_path": "/var/log/openimagemirror.log",
    "debug_mode": false,
    "cache_db_path": "/var/cache/openimagemirror.db"
}
```

## Installation
```bash
git clone https://github.com/twdtech/OpenImageMirror.git
cd OpenImageMirror
make
systemd start openimagemirror
```

# API Endpoints
## GET /mirror
- Retrieves list of all ISO files
- Returns JSON with file metadata
- Includes filename, full path, category, size, and modification time

## Architectural Highlights
- **Image Management:** Automatically scans and categorizes ISO files
- **Caching:** Reduces repeated disk scans
- **Flexible Configuration:** Easily adaptable to different environments

## Logging
Comprehensive logging with configurable verbosity levels. Logs are written to the specified log file, tracking initialization, scanning, and potential errors.

## Security Considerations
Configurable logging
Graceful signal handling

License
BSD 3 Clause

~TheWinDev