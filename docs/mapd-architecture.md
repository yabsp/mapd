# Memory Allocation Problem Detector (mapd) – System Architecture

## Project Goal

Design a dynamic memory problem detector for Linux user-space processes that can detect:

- Memory **leaks** *(via injection)*
- **Dangling pointers** (use-after-free) *(via injection)*
- **Buffer overflows** *(via injection)*
- **Memory fragmentation** *(via system observation)*
- **real-time GUI notifications**

---

## High-Level Architecture Overview

The system is composed of two main components:

1. **Injected Memory Wrapper (for injectable targets)**  
   - Injected using `LD_PRELOAD` (for C/C++ programs)
   - Intercepts `malloc`, `free`
   - Sends detailed memory events to a central analyzer

2. **System-Wide Analyzer Process**  
   - A persistent background process
   - Monitors general memory usage of **any user-space process**
   - Aggregates system metrics and tracks injected program events

---

## Communication Flow

```text
[ C Program (injected) ] ---+
                                |
[ Other user processes ] ------>+--> [ Analyzer Process ]
                                |         |-- Thread: Buddyinfo monitor
                                |         |-- Thread: GUI notifications
                                |         |-- Thread: Event listener (injected programs)
                                |         |-- Central message queue and reporter
```

---

## Components and Responsibilities

### 1. Injected Memory Wrapper (`libmemwrap.so`)

- Loaded via `LD_PRELOAD` environment variable into programs
- Tracks memory function calls and memory metadata
- Detects:
  - **Leaks**
  - **Dangling pointers**
  - **Buffer overflows**
- Communicates with analyzer over UNIX domain sockets.

---

### 2. Central Analyzer Process

Handles both injected and non-injected processes:

**Main Threads:**

| Thread Name       | Function                                                          |
|-------------------|-------------------------------------------------------------------|
| Buddyinfo Monitor | Monitors kernel-level fragmentation via `/proc/buddyinfo`        |
| GUI Thread        | Displays notifications (GTK)                |
| Event Listener    | Receives live memory events from wrappers (one thread per program) |
| Data Aggregator   | Logs, aggregates, reports system-wide memory behaviour           |

---

## Detection Capabilities Matrix

| Issue                         | Detectable Without Injection | 
|------------------------------|------------------------------|
| Memory leaks                 | No                         |
| Dangling pointers            | No                         |
| Buffer overflows             | No                         |
| Double frees                 | No                         |
| Memory fragmentation         | Yes (via `/proc/buddyinfo`) |

---

## IPC Protocol (Wrapper <-> Analyzer)

- **Protocol**: Custom message format (JSON)
- **Transport**: UNIX domain sockets
- **Events** (simplified):
  - `malloc size=64 addr=0x1234`
  - `free addr=0x1234`
  - `overflow detected at addr=0x5678`
  - `leak summary on exit`

## Development Plan

| Phase | Focus                                         | Est. Effort |
|-------|-----------------------------------------------|-------------|
| 1     | Memory wrapper with malloc/free logging       | 40h         |
| 2     | Analyzer prototype with socket server         | 40h         |
| 3     | Add buffer overflow and use-after-free checks | 60h         |
| 4     | Threaded analyzer with per-wrapper thread     | 50h         |
| 5     | Buddyinfo + /proc scanner                     | 60h         |
| 6     | GUI/Notification integration                  | 65h         |
| 7     | Testing distributed among the whole project   | 40h         |
| 8     | Final polishing + documentation               | 45h         |

---

##  Benefits of This Architecture

-  **Hybrid**: supports both injected and non-injected processes
-  **Modular**: clear separation of tracking and 
-  **Efficient**: no per-process analyzers needed
-  **Insightful**: capturing of both correctness and usage patterns possible
-  **Extendable**: supports GUI and time-series data.

---


