# === Compiler & Flags ===
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -g -pthread -Ilib -Isrc
OUTDIR = build

# === Source Files ===
ANALYZER_SRC = src/analyzer.c src/message.c lib/cJSON.c
WRAPPER_SRC = src/memwrap.c
TEST_ALLOC_SRC = tests/test_alloc.c

# === Targets ===
.PHONY: all clean test valgrind analyzer run-all

all: $(OUTDIR)/analyzer $(OUTDIR)/libmemwrap.so $(OUTDIR)/test_alloc

# Build analyzer binary
$(OUTDIR)/analyzer: $(ANALYZER_SRC)
	@mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Build shared library wrapper
$(OUTDIR)/libmemwrap.so: $(WRAPPER_SRC)
	@mkdir -p $(OUTDIR)
	$(CC) -shared $(CFLAGS) $^ -o $@ -ldl

# Build test_alloc
$(OUTDIR)/test_alloc: $(TEST_ALLOC_SRC)
	@mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Run analyzer manually
analyzer: $(OUTDIR)/analyzer
	$<

# Run test with wrapper injected
test: all
	LD_PRELOAD=$(OUTDIR)/libmemwrap.so $(OUTDIR)/test_alloc

# Run valgrind memory analysis
valgrind: all
	LD_PRELOAD=$(OUTDIR)/libmemwrap.so valgrind --leak-check=full --error-exitcode=1 $(OUTDIR)/test_alloc

# Run analyzer and test together in one command
run-all: all
	@echo "Starting analyzer..."; \
	$(OUTDIR)/analyzer & \
	ANALYZER_PID=$$!; \
	sleep 1; \
	echo "Running test_alloc with LD_PRELOAD..."; \
	LD_PRELOAD=$(OUTDIR)/libmemwrap.so $(OUTDIR)/test_alloc; \
	echo "Stopping analyzer..."; \
	kill $$ANALYZER_PID || true

# Clean build artifacts
clean:
	rm -rf $(OUTDIR)
