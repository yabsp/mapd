# === Compiler & Flags ===
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -g -pthread -Ilib -Isrc

# === Source Files ===
ANALYZER_SRC = src/analyzer.c src/message.c lib/cJSON.c
WRAPPER_SRC = src/memwrap.c
TEST_ALLOC_SRC = tests/test_alloc.c

# === Targets ===
.PHONY: all clean test valgrind analyzer run-all

all: analyzer_bin libmemwrap.so test_alloc

analyzer_bin: $(ANALYZER_SRC)
	$(CC) $(CFLAGS) $^ -o analyzer

libmemwrap.so: $(WRAPPER_SRC)
	$(CC) -shared $(CFLAGS) $^ -o libmemwrap.so -ldl

test_alloc: $(TEST_ALLOC_SRC)
	$(CC) $(CFLAGS) $^ -o test_alloc

# Run analyzer manually
analyzer: analyzer_bin
	./analyzer

# Run test with wrapper injected
test: all
	LD_PRELOAD=./libmemwrap.so ./test_alloc

# Run valgrind memory analysis
valgrind: all
	LD_PRELOAD=./libmemwrap.so valgrind --leak-check=full --error-exitcode=1 ./test_alloc

# Run analyzer and test together in one command
run-all: all
	@echo "Starting analyzer..."; \
	./analyzer & \
	ANALYZER_PID=$$!; \
	sleep 1; \
	echo "Running test_alloc with LD_PRELOAD..."; \
	LD_PRELOAD=./libmemwrap.so ./test_alloc; \
	echo "Stopping analyzer..."; \
	kill $$ANALYZER_PID || true

# Clean build artifacts
clean:
	rm -f analyzer libmemwrap.so test_alloc