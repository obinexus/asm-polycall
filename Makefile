# asm-polycall -- GNU Make with a POSIX shell (Linux, macOS, MSYS2 on Windows).
# The installed Polycall core (>= 1.1.0, binding ABI 1) is found with
# pkg-config: PKG_CONFIG_PATH=<prefix>/lib/pkgconfig.
CC ?= gcc
AR ?= ar
CLANG ?= clang
PKG_CONFIG ?= pkg-config

POLYCALL_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags polycall 2>/dev/null)
POLYCALL_LIBS ?= $(shell $(PKG_CONFIG) --libs polycall 2>/dev/null)
POLYCALL_LIBDIR ?= $(shell $(PKG_CONFIG) --variable=libdir polycall 2>/dev/null)

CPPFLAGS ?=
CPPFLAGS += -Iinclude $(POLYCALL_CFLAGS)
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

BUILD_DIR := build
LIB_DIR := lib
ADAPTER_OBJ := $(BUILD_DIR)/asm_polycall.o
STATIC_LIB := $(LIB_DIR)/libasm_polycall.a
TEST_BIN := $(BUILD_DIR)/asm_polycall_real_test
EXAMPLE_BIN := $(BUILD_DIR)/basic

PROBE_BIN := $(BUILD_DIR)/abi_probe

ifeq ($(OS),Windows_NT)
TEST_BIN := $(TEST_BIN).exe
EXAMPLE_BIN := $(EXAMPLE_BIN).exe
PROBE_BIN := $(PROBE_BIN).exe
FAKE_LIB := libpolycall.dll
FAKE_LDFLAGS := -shared
REAL_LIB ?= $(POLYCALL_LIBDIR)/../bin/libpolycall.dll
else
FAKE_LIB := libpolycall.so.1
FAKE_LDFLAGS := -shared -fPIC -Wl,-soname,libpolycall.so.1
REAL_LIB ?= $(POLYCALL_LIBDIR)/libpolycall.so.1
endif

# Targets the shims are assembled for by `make cross-check` (needs clang).
CROSS_TARGETS := x86_64-linux-gnu x86_64-w64-windows-gnu i686-linux-gnu \
	i686-w64-windows-gnu aarch64-linux-gnu aarch64-w64-windows-gnu \
	armv7-linux-gnueabihf x86_64-apple-darwin arm64-apple-darwin

.DEFAULT_GOAL := all

.PHONY: all
all: $(STATIC_LIB)

.PHONY: check-core
check-core:
	@test -n "$(POLYCALL_LIBS)" || { echo "asm-polycall: pkg-config cannot find polycall (>= 1.1.0); set PKG_CONFIG_PATH=<prefix>/lib/pkgconfig" >&2; exit 2; }

$(BUILD_DIR) $(LIB_DIR):
	@mkdir -p $@

$(ADAPTER_OBJ): src/asm_polycall.S | $(BUILD_DIR)
	$(CC) -c $< -o $@

$(STATIC_LIB): $(ADAPTER_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(TEST_BIN): tests/asm_polycall_real_test.c $(ADAPTER_OBJ) include/asm_polycall.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/asm_polycall_real_test.c $(ADAPTER_OBJ) -o $@ $(LDFLAGS) $(POLYCALL_LIBS) -pthread

# Real-core test incl. interop with the C CLI (exit 77 = SKIP without it).
.PHONY: test
test: check-core $(TEST_BIN)
	sh tests/run-real.sh $(TEST_BIN) .

# Loader behaviour: real library, no library, a library reporting ABI 2 and
# a 1.0 library without the ABI v1 symbols (the last two are test fixtures,
# tests/loader/fake_polycall.c). Linux and MinGW.
$(PROBE_BIN): tests/abi_probe.c $(ADAPTER_OBJ) include/asm_polycall.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/abi_probe.c $(ADAPTER_OBJ) -o $@ $(LDFLAGS) $(POLYCALL_LIBS)

$(BUILD_DIR)/fake-abi2/$(FAKE_LIB) $(BUILD_DIR)/fake-old/$(FAKE_LIB): tests/loader/fake_polycall.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -O2 $(FAKE_LDFLAGS) -DFAKE_$(if $(findstring fake-abi2,$@),ABI2,OLD) $< -o $@

.PHONY: test-loader
test-loader: check-core $(PROBE_BIN) $(BUILD_DIR)/fake-abi2/$(FAKE_LIB) $(BUILD_DIR)/fake-old/$(FAKE_LIB)
	sh tests/loader-errors.sh $(PROBE_BIN) $(REAL_LIB) $(BUILD_DIR)/fake-abi2/$(FAKE_LIB) $(BUILD_DIR)/fake-old/$(FAKE_LIB)

.PHONY: example
example: check-core $(ADAPTER_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/basic.c $(ADAPTER_OBJ) -o $(EXAMPLE_BIN) $(LDFLAGS) $(POLYCALL_LIBS)
	$(EXAMPLE_BIN)

# Assemble the shims for every supported target (no linking, no running).
.PHONY: cross-check
cross-check: | $(BUILD_DIR)
	@command -v $(CLANG) >/dev/null || { echo "SKIP: $(CLANG) not found"; exit 77; }
	@for t in $(CROSS_TARGETS); do \
		$(CLANG) --target=$$t -c src/asm_polycall.S -o $(BUILD_DIR)/cross-$$t.o || exit 1; \
		echo "assembled for $$t"; \
	done

.PHONY: verify-dry
verify-dry:
	sh scripts/verify-dry.sh

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR)
