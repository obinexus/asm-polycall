CC ?= gcc
AR ?= ar

CPPFLAGS ?=
CPPFLAGS += -Iinclude -Igenerated
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Wpedantic

BUILD_DIR := build
LIB_DIR := lib
ADAPTER_OBJ := $(BUILD_DIR)/asm_polycall.o
STATIC_LIB := $(LIB_DIR)/libasm_polycall.a
TEST_BIN := $(BUILD_DIR)/asm_polycall_adapter_test
EXAMPLE_BIN := $(BUILD_DIR)/basic

ifeq ($(OS),Windows_NT)
EXE_EXT := .exe
TEST_BIN := $(TEST_BIN)$(EXE_EXT)
EXAMPLE_BIN := $(EXAMPLE_BIN)$(EXE_EXT)
endif

.DEFAULT_GOAL := all

.PHONY: all
all: $(STATIC_LIB)

$(BUILD_DIR) $(LIB_DIR):
ifeq ($(OS),Windows_NT)
	@if not exist "$@" mkdir "$@"
else
	@mkdir -p $@
endif

$(ADAPTER_OBJ): src/asm_polycall.S | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(STATIC_LIB): $(ADAPTER_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(TEST_BIN): src/asm_polycall.S tests/polycall_ffi_mock.c tests/asm_polycall_adapter_test.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Itests $(CFLAGS) $^ -o $@

.PHONY: test
test: $(TEST_BIN)
	$(TEST_BIN)

.PHONY: example
example: $(ADAPTER_OBJ) | $(BUILD_DIR)
ifeq ($(OS),Windows_NT)
	@if "$(strip $(POLYCALL_LDFLAGS))"=="" (echo Set POLYCALL_LDFLAGS to the libpolycall v1.5 linker flags & exit /b 2)
else
	@test -n "$(POLYCALL_LDFLAGS)" || (echo "Set POLYCALL_LDFLAGS to the libpolycall v1.5 linker flags" && exit 2)
endif
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/basic.c $(ADAPTER_OBJ) \
		$(POLYCALL_LDFLAGS) -o $(EXAMPLE_BIN)
	$(EXAMPLE_BIN)

.PHONY: verify-dry
verify-dry:
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify-dry.ps1
else
	sh scripts/verify-dry.sh
endif

.PHONY: clean
clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	@if exist "$(LIB_DIR)" rmdir /s /q "$(LIB_DIR)"
else
	rm -rf $(BUILD_DIR) $(LIB_DIR)
endif

-include $(ADAPTER_OBJ:.o=.d)
