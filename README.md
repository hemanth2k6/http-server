# 🚀 Multi-Threaded HTTP/1.1 Server in C++

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen?style=flat-square)](https://github.com/hemanth2k6/http-server)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)](./LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey?style=flat-square)](https://github.com/hemanth2k6/http-server)

A production-grade, multi-threaded HTTP/1.1 server built from scratch in modern C++17. No frameworks, no bloat — just raw sockets, a hand-rolled thread pool, and a clean architecture designed to handle real concurrent load. Whether you're learning how web servers work under the hood or need a lightweight, embeddable HTTP server for your own projects, this is a solid place to start.

---

## ✨ Key Features

- **Full HTTP/1.1 Parsing** — Robust request and response parsing covering methods, headers, status codes, and body handling, built to spec.
- **Custom Thread Pool** — A hand-crafted thread pool manages a configurable number of worker threads, enabling genuine concurrent connection handling without the overhead of spawning a new thread per request.
- **Static File Serving** — Efficiently serves HTML, CSS, JS, images, and other static assets from a configured document root, with automatic MIME type detection for correct `Content-Type` headers.
- **Safe File I/O** — Path traversal prevention and safe static file reads are baked in, so you're not accidentally exposing the host filesystem.
- **High Throughput & Low Latency** — Designed from the ground up for performance: non-blocking where it matters, minimal allocations in the hot path, and zero unnecessary copies.
- **Configurable** — Port, thread count, and document root are all configurable via a simple config file or CLI arguments.

---

## 🛠 Getting Started

### Prerequisites

Make sure you have the following installed before building:

- `g++` (>= 9.0) or `clang++` (>= 10.0) with C++17 support
- `CMake` (>= 3.14)
- `make`

On Ubuntu/Debian:
```bash
sudo apt update && sudo apt install -y build-essential cmake
```

On macOS (with Homebrew):
```bash
brew install cmake
```

### Build

Clone the repository and build with CMake:

```bash
git clone https://github.com/hemanth2k6/http-server.git
cd http-server

mkdir build && cd build
cmake ..
make -j$(nproc)
```

The compiled binary will be placed at `build/httpserver`.

### Run

```bash
./httpserver
```

By default, the server reads from `config/server.conf`. You can also pass arguments directly:

```bash
./httpserver [port] [threads] [document_root]
```

**Examples:**

```bash
# Start on port 8080 with 4 threads, serving files from ../www
./httpserver 8080 4 ../www

# Start with all defaults (reads from config/server.conf)
./httpserver
```

---

## ⚙️ Configuration

The `config/server.conf` file controls the server's runtime behavior. Here's what it looks like:

```ini
# config/server.conf

port        = 8080
threads     = 4
document_root = ./www
```

| Option          | Default   | Description                                      |
|-----------------|-----------|--------------------------------------------------|
| `port`          | `8080`    | The TCP port to listen on                        |
| `threads`       | `4`       | Number of worker threads in the pool             |
| `document_root` | `./www`   | Root directory from which static files are served|

Command-line arguments always take precedence over the config file, so you can override individual settings without editing the file.

---

## 🧪 Usage & Testing

Once the server is running, you can verify it's working in a few ways.

### Using curl

A quick sanity check to fetch the root page:
```bash
curl -v http://localhost:8080/
```

Fetch a specific static file:
```bash
curl -v http://localhost:8080/styles.css
```

Test a non-existent route to confirm 404 handling:
```bash
curl -v http://localhost:8080/does-not-exist.html
```

Send a POST request:
```bash
curl -X POST -d "key=value" http://localhost:8080/endpoint
```

### Using a Browser

Just open your browser and navigate to:

```
http://localhost:8080/
```

You should see the `index.html` page served from the `www/` directory.

### Running Unit Tests

After building the project, the test binaries will also be available in `build/tests/`:

```bash
# Run the request parser tests
./tests/test_parser

# Run the integration/server tests
./tests/test_server
```

---

## 🏛 Architecture

The server follows a clean, layered design that separates concerns clearly. Here's the high-level flow:

```
Client Connection
      │
      ▼
 http_server.cpp         ← Accepts incoming TCP connections
      │
      ▼
 thread_pool.cpp         ← Dispatches each connection to a worker thread
      │
      ▼
 request_parser.cpp      ← Parses the raw HTTP/1.1 request bytes
      │
      ▼
 file_handler.cpp        ← Resolves the request to a file on disk (safely)
      │
      ▼
 response_builder.cpp    ← Builds the HTTP response with correct headers
      │
      ▼
 socket_utils.cpp        ← Writes the response back to the client socket
```

**Key design decisions:**

- The **thread pool** uses a work queue with a `std::mutex` and `std::condition_variable` so threads sleep efficiently when idle and wake up immediately when work arrives.
- The **file handler** resolves all paths relative to the configured document root and rejects any path containing `..` to prevent directory traversal attacks.
- The **response builder** dynamically selects the correct MIME type based on the file extension before writing the `Content-Type` header.

### Repository Structure

```
http-server/
├── CMakeLists.txt
├── config/
│   └── server.conf           ← Runtime configuration
├── src/
│   ├── main.cpp              ← Entry point, config loading
│   └── server/
│       ├── http_server.*     ← TCP accept loop
│       ├── request_parser.*  ← HTTP/1.1 request parsing
│       ├── response_builder.*← HTTP response construction
│       ├── file_handler.*    ← Static file serving & MIME types
│       └── thread_pool.*     ← Worker thread management
│   └── utils/
│       └── socket_utils.*    ← Low-level socket helpers
├── tests/
│   ├── test_parser.cpp       ← Unit tests for the parser
│   └── test_server.cpp       ← Integration tests
└── www/
    ├── index.html            ← Default served page
    └── styles.css
```

---

## 🤝 Contributing

Contributions are welcome! If you've spotted a bug, have a feature idea, or want to improve the docs, feel free to open an issue or submit a pull request.

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make your changes and add tests where appropriate
4. Commit with a clear message: `git commit -m "feat: add keep-alive connection support"`
5. Push to your branch: `git push origin feature/your-feature-name`
6. Open a Pull Request and describe what you changed and why

Please try to keep PRs focused — one feature or fix per PR makes review much easier. If you're planning something significant, open an issue first so we can discuss the approach before you invest the time.

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](./LICENSE) file for details. You're free to use, modify, and distribute this code in your own projects, commercial or otherwise.

---

<p align="center">Built with C++17 · Made by <a href="https://github.com/hemanth2k6">hemanth2k6</a></p>
