#  MAPD - Memory Allocation Problem Detector

## Overview
MAPD is a memory analysis framework that wraps client applications via LD_PRELOAD, intercepts memory allocation calls, and forwards allocation events to a central analyzer via a UNIX socket. The analyzer parses, queues, and processes these messages, exposing live memory usage and fragmentation data to a GTK4-based GUI in real time.

## Features
- Full real-time memory allocation tracing (malloc, calloc, realloc, free)
- Live fragmentation monitoring
- Thread-safe global message queue
- Live interactive GUI built with GTK 4
- Realtime live log display of allocation events

## Prerequisites and Build

- Ubuntu 22.04 or later
- GTK 4.6+ development libraries
The full instructions can be found in the [installation document](docs/INSTALL.md).


## Project Components
A description and sketch of the architecture can be found in the [architecture description](docs/mapd-architecture.md).
The system consists of the following core modules:

### `memwrap/`

- LD_PRELOAD shared library (`memwrap.so`)
- Intercepts `malloc()`, `free()`
- Sends JSON-encoded memory events to the central analyzer over a Unix Domain Socket (`/tmp/mapd_socket`)

### `analyzer/`

- Multithreaded server listening for incoming client connections.
- Spawns one thread per client.
- Parses incoming JSON messages into structured `Message` objects.
- Enqueues messages into a thread-safe global message queue.
- Exposes `analyzer_init()` for embedded GUI startup.

#### fragmentation observation
- Background fragmentation monitoring thread.
- Periodically inspects fragmentation state.
- Sends event to analyzer in case of bad fragmentation.

### `message/`

- Implements the core thread-safe message queue.
- Handles serialization/deserialization of `Message` objects.
- Provides `enqueue_message()` and `dequeue_message()` for communication.

### `gui/`

- Full GTK 4.6+ GUI written in Model-View-Controller (MVC) pattern.
- Allows user to:
    - Select target application for wrapping.
    - Launch instrumented clients (max 5 concurrent).
    - Live monitor all memory events in a scrollable log view.
- Fully integrated with the backend analyzer running inside the GUI process.

## Scope
This project was completed as part of the "Operating Systems" semester course at University of Basel in addition to
the lectures. It provides the possibility to implement the newly learned material in a real world use case.

## Authors
Contributors include:
- Gioia Almer
- Yanick Spichty
- Max Reinert
-  Mike Müller

## License
This project is licensed under the MIT License.