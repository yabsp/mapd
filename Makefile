# === Compiler & Flags ===
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -g -pthread -Ilib -Isrc

# === Build directory and Valgrind test location
OUTDIR = build
VALGRIND_TEST_BIN = $(OUTDIR)/test_valgrind

# === Source Files ===
ANALYZER_SRC = src/analyzer.c src/message.c lib/cJSON.c
WRAPPER_SRC = src/memwrap.c
TEST_ALLOC_SRC = tests/test_alloc.c
TEST_VALGRIND_SRC = tests/test_valgrind.c

# === Arguments for test_alloc
TEST_ARGS := --leak --overflow --dangling --double-free --fragmentation --simple

# === phony, i.e. marked as instruction and not as target ===
.PHONY: all clean test-all valgrind-test analyzer ci test-%

# all binaries as prerequisite
all: $(OUTDIR)/analyzer $(OUTDIR)/libmemwrap.so $(OUTDIR)/test_alloc

# prerequisite for github ci.yaml
ci: test-all valgrind-analyzer

# === BUILD FILES ===
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

# Build test_valgrind
$(VALGRIND_TEST_BIN): $(TEST_VALGRIND_SRC)
	@mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Run analyzer
analyzer: $(OUTDIR)/analyzer
	$(OUTDIR)/analyzer

# GNU Make define directive, shell script used above for automated test-all
test-all: all
	@mkdir -p $(OUTDIR)/tmp
	@for arg in $(TEST_ARGS); do \
		echo "\n[Make] Running test_alloc $$arg"; \
		$(OUTDIR)/analyzer & \
		ANALYZER_PID=$$!; \
		sleep 0.5; \
		LD_PRELOAD=$(OUTDIR)/libmemwrap.so $(OUTDIR)/test_alloc $$arg > $(OUTDIR)/tmp/tmp.out 2>&1; \
		STATUS=$$?; \
		kill $$ANALYZER_PID || true; \
		if [ "$$arg" = "--simple" ] || [ "$$arg" = "--leak" ] || [ "$$arg" = "--fragmentation" ] || [ "$$arg" = "--double-free" ]; then \
			if [ $$STATUS -eq 0 ]; then \
				echo "[Make] $$arg passed as expected"; \
			else \
				echo "[Make] $$arg failed unexpectedly (exit $$STATUS)"; cat $(OUTDIR)/tmp/tmp.out; exit 1; \
			fi; \
		elif [ "$$arg" = "--dangling" ] || [ "$$arg" = "--overflow" ]; then \
			if [ $$STATUS -ne 0 ]; then \
				echo "[Make] $$arg crashed as expected (exit $$STATUS)"; \
			else \
				echo "[Make] $$arg did NOT crash as expected"; cat $(OUTDIR)/tmp/tmp.out; exit 1; \
			fi; \
		else \
			echo "[Make] Unknown test $$arg"; exit 1; \
		fi; \
	done

# Individual test targets: make test-leak, test-dangling, etc.
test-%: all
	@echo "\n[Make] Running test_alloc --$* (manual mode)"
	@mkdir -p $(OUTDIR)/tmp
	@LD_PRELOAD=$(OUTDIR)/libmemwrap.so $(OUTDIR)/test_alloc --$*
	@echo "[Make] Press Ctrl+C to stop or kill manually"

# Run valgrind on analyzer -> check issues with analyzer
valgrind-analyzer: $(OUTDIR)/analyzer
	@echo "[Make] Running analyzer under Valgrind (10s timeout)..."
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR)/logs
	@timeout 10s valgrind --leak-check=full --show-leak-kinds=all \
		--log-file=$(OUTDIR)/logs/valgrind-analyzer.log \
		$(OUTDIR)/analyzer &
	@sleep 12
	@echo "[Make] Valgrind output for analyzer:"
	@cat $(OUTDIR)/logs/valgrind-analyzer.log
	@if ! grep "definitely lost: [1-9][0-9]* bytes" $(OUTDIR)/logs/valgrind-analyzer.log; then \
    		echo "[Make] No leaks detected in analyzer"; \
    	else \
    		echo "[Make] Memory leaks found in analyzer"; \
    		exit 1; \
    	fi

# Smoke Test: Run valgrind on test_valgrind — expected to FAIL = PASS, automated use
valgrind-test: $(VALGRIND_TEST_BIN)
	@echo "Running intentional leak test (Valgrind should detect a leak)..."
	@if valgrind --leak-check=full --error-exitcode=1 $(VALGRIND_TEST_BIN); then \
		echo "Leak not detected — test failed"; \
		exit 1; \
	else \
		echo "Leak detected — Valgrind is working as expected"; \
		exit 0; \
	fi

# Clean build artifacts
clean:
	rm -rf $(OUTDIR)
