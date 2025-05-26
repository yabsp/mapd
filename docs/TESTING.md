# Memory Testing Suite – Makefile Usage Guide

This document explains how to use the provided `Makefile` to build, run, and validate the memory analysis components of this project. It supports **automated tests**, **Valgrind verification**, and **manual test execution** using `LD_PRELOAD`.

---

## Build Targets

### `make all`
Builds everything:
- The `analyzer` binary (memory event collector)
- The `memwrap` shared library (`libmemwrap.so`)
- The `test_alloc` binary
- Creates the `build/` directory

---

## Running the Test Suite

### `make test-all`
Runs all memory-related test cases with:
- `memwrap.so` preloaded
- The analyzer running in the background
- Exit codes checked automatically
- Test output saved to `build/tmp/tmp.out`

Each test case is defined as:
- `--leak`
- `--overflow`
- `--dangling`
- `--double-free`
- `--fragmentation`
- `--simple`

Test behaviour is verified based on expected outcomes (e.g., crash or clean exit).

---

## Running Individual Tests

You can run specific test cases manually using:

```bash
make test-<name>
```

For example:
```bash
make test-leak
make test-overflow
make test-dangling
```

This:
- Starts the analyzer
- Runs `test_alloc` with the given argument
- Preloads `libmemwrap.so`
- Saves output to `build/tmp/test_<name>.out`

---

## Valgrind Functionality Check

### `make valgrind-test`

Runs a **smoke test** using `test_valgrind` to check if **Valgrind detects an intentional leak**.

Expected result: Valgrind fails and exits with an error — this is a **pass**.

---

## Analyzer Self-Test (Valgrind)

### `make valgrind-analyzer`

Runs the `analyzer` binary under Valgrind for 10 seconds and checks for memory leaks.

- Output is saved in `build/logs/valgrind-analyzer.log`
- Test passes if **no memory is reported as definitely lost**

---

## Full CI Test Target

### `make ci`

Runs the full test suite used in GitHub Actions:
- `make test-all`
- `make valgrind-test`
- `make valgrind-analyzer`

---

## Clean Up

### `make clean`

Removes the entire `build/` directory, including:
- All binaries
- Temporary logs
- Valgrind reports

---

## Notes

- All memory tests use `LD_PRELOAD=build/libmemwrap.so` to inject the custom memory tracker.
- Output logs can be inspected under `build/tmp/` for debugging.

### Testing without Make
- It is possible to test without Make.
- In such a case pass the test you want to run as argument to the test_alloc executable.
- Possible arguments are: `--overflow`, `--dangling`, `--double-free`, `--fragmentation`, `--simple`, `--all`
- In this case you will need a separate, running analyzer instance