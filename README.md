# C++17 Asynchronous Reverse Proxy & Load Balancer

A high-performance, single-threaded asynchronous C++17 reverse proxy and load balancer utilizing raw sockets, edge-triggered `epoll`, a token-bucket rate limiter, passive/active health checks, and round-robin load balancing.

---

## Table of Contents
- [What & Why](#what--why)
- [Key Features](#key-features)
- [Architecture & How It Works](#architecture--how-it-works)
  - [Asynchronous Epoll Engine](#asynchronous-epoll-engine)
  - [Token Bucket Rate Limiter](#token-bucket-rate-limiter)
  - [Load Balancing & Health Checker](#load-balancing--health-checker)
- [Project Directory Structure](#project-directory-structure)
- [Prerequisites](#prerequisites)
- [Step-by-Step Usage Guide](#step-by-step-usage-guide)
  - [1. Setting Up Test Backends](#1-setting-up-test-backends)
  - [2. Building and Running locally](#2-building-and-running-locally)
  - [3. Running under ThreadSanitizer (TSan)](#3-running-under-threadsanitizer-tsan)
  - [4. Docker Containerization](#4-docker-containerization)
  - [5. Verification Tests](#5-verification-tests)
    - [Basic Proxying (Round-Robin)](#basic-proxying-round-robin)
    - [Rate Limiting (HTTP 429)](#rate-limiting-http-429)
    - [Health Check Failover & Recovery](#health-check-failover--recovery)
  - [6. Load Testing with `wrk`](#6-load-testing-with-wrk)

---

## What & Why

This project is a lightweight reverse proxy and load balancer written in modern C++17. In production environments, standard reverse proxies (like Nginx or HAProxy) add routing, rate-limiting, and high-availability layers. This project serves as a lightweight, clean-room implementation of these core features built from scratch.

By bypassing heavyweight frameworks and working directly with the Linux `epoll` system calls, this proxy achieves maximum throughput and low latency under high concurrency (C10k class) while maintaining a minimal memory footprint.

---

## Key Features

- **Asynchronous Event Loop**: Edge-triggered `epoll` design for asynchronous, non-blocking I/O operations.
- **Round-Robin Load Balancing**: Balances client requests evenly across all available healthy upstream backends.
- **Active Health Checker**: Background checking thread monitors upstream servers with standard HTTP health checks and updates routing tables dynamically.
- **Token-Bucket Rate Limiter**: IP-based rate limiting with configurable capacity (burst) and token refill rate.
- **Dockerized Deployment**: Fully containerized using lightweight multi-stage Alpine Linux builds.
- **TSan Safe**: Clean compile and execution flags ready for static analysis and ThreadSanitizer checks.

---

## Architecture & How It Works

### Asynchronous Epoll Engine
The core event loop is driven by the class `EpollManager` in [src/epoll_manager.hpp](file:///home/hemanth/my-projects/http%20server/src/epoll_manager.hpp). It uses Linux **edge-triggered (EPOLLET)** mode. 
1. Client connects to the proxy port.
2. The proxy accepts the connection, configures the socket to be non-blocking, and adds it to `epoll`.
3. When data is received, the proxy reads asynchronously until `EAGAIN` or `EWOULDBLOCK` is returned, processes the request headers via the parser, matches the client IP against the rate limiter, and delegates the request to the upstream backend.
4. Upstream backend responses are read asynchronously and pushed back to the client.

### Token Bucket Rate Limiter
Implemented in [src/rate_limiter.cpp](file:///home/hemanth/my-projects/http%20server/src/rate_limiter.cpp). It maintains a thread-safe registry of client IPs. Each IP has a bucket filled with tokens at a configured rate. Requests consuming tokens below the threshold are permitted, otherwise, they are quickly rejected with an `HTTP/1.1 429 Too Many Requests` response.

### Load Balancing & Health Checker
- **Load Balancer**: Chosen using a simple round-robin scheme. The balancer holds a reference to the backend list so health modifications are seen in real-time.
- **Health Checker**: Runs a dedicated thread that performs active health checks at a defined interval (e.g., sending `GET /health` requests to the backends). If a backend fails to respond, it is marked as unhealthy, and the load balancer dynamically skips it. Once it becomes responsive again, it is safely reintroduced to the pool.

---

## Project Directory Structure

```
.
├── CMakeLists.txt              # Build configuration for CMake
├── Dockerfile                  # Multi-stage Docker build config (Alpine based)
├── .dockerignore               # Patterns to ignore during Docker builds
├── .gitignore                  # Git untrack specifications
├── README.md                   # Project documentation
├── src/                        # C++ Source and Header Files
│   ├── connection.cpp          # Client and backend socket event handling
│   ├── connection.hpp
│   ├── epoll_manager.cpp       # Epoll registration wrapper
│   ├── epoll_manager.hpp
│   ├── health_checker.cpp      # Active background checker
│   ├── health_checker.hpp
│   ├── http_parser.cpp         # HTTP raw string request parser
│   ├── http_parser.hpp
│   ├── load_balancer.cpp       # Round-Robin routing selection
│   ├── load_balancer.hpp
│   ├── main.cpp                # CLI Argument parser and program entry point
│   ├── proxy_server.cpp        # Event loop and connection registry
│   ├── proxy_server.hpp
│   ├── rate_limiter.cpp        # Thread-safe Token-Bucket rate limiter
│   ├── rate_limiter.hpp
│   ├── socket_utils.cpp        # Socket flags and helpers
│   └── socket_utils.hpp
└── test_backends/              # Python test backends
    ├── backend8001/
    │   └── index.html          # Backend 1 response file
    └── backend8002/
        └── index.html          # Backend 2 response file
```

---

## Prerequisites

To compile and run this project, you need:
- A Linux kernel environment (e.g., WSL 2 or native Ubuntu/Alpine)
- `g++ 13` or newer (supporting C++17)
- `CMake` (version 3.14+)
- `Docker` (if running containerized)
- `wrk` (optional, for load testing)

---

## Step-by-Step Usage Guide

### 1. Setting Up Test Backends
Start two Python mock backends on ports `8001` and `8002` using the provided test files:
```bash
python3 -m http.server -d test_backends/backend8001 8001 &
python3 -m http.server -d test_backends/backend8002 8002 &
```
Verify they are running:
```bash
curl http://127.0.0.1:8001/
curl http://127.0.0.1:8002/
```

### 2. Building and Running Locally
To compile the binary in release mode:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```
Start the proxy locally:
```bash
./build/proxy -p 8080 -b 127.0.0.1:8001,127.0.0.1:8002 -r 100 -t 50 -h 2
```
*Note: Flags represent: `-p` (listen port), `-b` (comma-separated backends), `-r` (rate-limit tokens/sec), `-t` (burst capacity), `-h` (health check interval).*

### 3. Running under ThreadSanitizer (TSan)
To test for data races under heavy load, build with the TSan instrumentation flag:
```bash
cmake -B build -S . -DENABLE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```
Run using `setarch` to disable Address Space Layout Randomization (ASLR), preventing memory mapping conflicts with TSan:
```bash
setarch $(uname -m) -R ./build/proxy -p 8080 -b 127.0.0.1:8001,127.0.0.1:8002 -r 10000 -t 5000 -h 2
```

### 4. Docker Containerization
Build the Docker image:
```bash
docker build -t myproxy .
```
Start the container. If you are running Docker Desktop (which uses a virtual machine network), map the proxy port explicitly and point the backends to the host VM IP (e.g., `172.24.174.38`) instead of `127.0.0.1`:
```bash
docker run -d --name myproxy_container -p 8080:8080 myproxy -p 8080 -b 172.24.174.38:8001,172.24.174.38:8002 -r 5 -t 3 -h 2
```

---

### 5. Verification Tests

#### Basic Proxying (Round-Robin)
Perform curls and ensure responses alternate between your backends:
```bash
curl -s http://127.0.0.1:8080/
curl -s http://127.0.0.1:8080/
```
*Expected Output:*
```
Backend 8001 is online
Backend 8002 is online
```

#### Rate Limiting (HTTP 429)
Make requests rapidly in a short interval. When you hit the rate limit, the proxy immediately returns a `429 Too Many Requests` status code:
```bash
for i in {1..5}; do curl -s -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/; done
```
*Expected Output:*
```
200
200
200
429
429
```

#### Health Check Failover & Recovery
1. **Trigger Failover**: Terminate backend 8001.
   ```bash
   pgrep -f "8001" | xargs kill
   ```
2. **Observe redirection**: Wait for 6–7 seconds (three health checks). Perform curls and verify all requests are sent only to the remaining healthy backend `8002`:
   ```bash
   curl -s http://127.0.0.1:8080/
   curl -s http://127.0.0.1:8080/
   ```
   *Expected Output:*
   ```
   Backend 8002 is online
   Backend 8002 is online
   ```
3. **Trigger Recovery**: Restart backend 8001.
   ```bash
   python3 -m http.server -d test_backends/backend8001 8001 &
   ```
4. **Observe re-entry**: Wait for 7 seconds. Perform curls again and verify round-robin behavior has resumed:
   ```bash
   curl -s http://127.0.0.1:8080/
   curl -s http://127.0.0.1:8080/
   ```
   *Expected Output:*
   ```
   Backend 8001 is online
   Backend 8002 is online
   ```

---

### 6. Load Testing with `wrk`
Run a load test with 4 threads and 100 connections for 30 seconds:
```bash
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
```
Output displays throughput, latencies, and any network/socket errors encountered during the test.
