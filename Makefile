rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SOURCES=$(call rwildcard,src,*.c)
TESTS=$(call rwildcard,test,*.c)
LINKER_FLAGS=$(patsubst %.c, build/%.lf, $(TESTS))
EXAMPLES=$(call rwildcard,example,*.c)
SRC_OBJECTS=$(patsubst %.c, build/%.o, $(SOURCES))
TEST_OBJECTS=$(patsubst %.c, build/%.o, $(TESTS))
EXAMPLE_OBJECTS=$(patsubst %.c, build/%.o, $(EXAMPLES))
EXAMPLE_BINARIES=$(patsubst %.c, bin/%, $(EXAMPLES))
DEPENDENCIES=$(patsubst %.c, build/%.d, $(SOURCES)$(TESTS)$(EXAMPLES))
TEST_BIN=tests
TEST_RUN=build/$(TEST_BIN).run

MEMORY_DEBUG_FLAGS=-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract
WARN_FLAGS=-Wall -Wextra -Werror -Wno-error=cpp -Wno-unused-function -Wunused-result -Wvla -Wshadow -Wstrict-prototypes -Wno-maybe-uninitialized
SANTIIZER_FLAGS=-fsanitize=undefined -fsanitize-address-use-after-scope -fstack-check -fno-stack-clash-protection
DEBUG_FLAGS=$(WARN_FLAGS) $(MEMORY_DEBUG_FLAGS) $(SANITIZER_FLAGS) -Og -ggdb3
OPTIMIZE_FLAGS=-march=native -O2 -pipe -D NDEBUG
LINK_FLAGS=$(BASE_CFLAGS) -s -flto=4 -fwhole-program -pthread
TEST_FLAGS=$(BASE_CFLAGS) -Isrc
BASE_CFLAGS=-std=gnu99 -MMD -MP
CFLAGS=$(BASE_CFLAGS)

all: release

release: CFLAGS += $(OPTIMIZE_FLAGS)
release: $(TEST_RUN) $(SRC_BINARIES)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TEST_RUN) $(SRC_BINARIES)

check: $(TEST_RUN)

clean:
	rm -r build bin

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.DELETE_ON_ERROR:
.PHONY: all release debug check clean
$(VERBOSE).SILENT:
$(shell mkdir -p $(dir $(DEPENDENCIES)))
-include $(DEPENDENCIES)

build/test/%.c: test/%.c
	$(info E   $@)
	$(CC) -E $(TEST_FLAGS) -o $@ $<

build/src/%.c: src/%.c
	$(info E   $@)
	$(CC) -E $(CFLAGS) -o $@ $<

build/src/ctf/ctf.o: src/ctf/ctf.c
	mkdir -p $(@D)
	$(info CC  $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_RUN): bin/$(TEST_BIN)
	mkdir -p $(@D)
	$(info RUN $<)
	./$<
	touch $@

bin/$(TEST_BIN): $(TEST_OBJECTS) build/src/ctf/ctf.o | build/test/$(TEST_BIN).lf
	mkdir -p $(@D)
	$(info LN  $@)
	$(CC) $(LINK_FLAGS) $(TEST_FLAGS) `cat $|` -o $@ $^

build/test/$(TEST_BIN).lf: $(TESTS)
	$(info FLG $@)
	grep -ho '__wrap_[^( 	]\+' $^ | sort | uniq | sed 's/__wrap_\(.\+\)/,--wrap=\1/' | tr -d '\n' | sed 's/^,/-Wl,/' > $@

build/test/%.o: test/%.c
	mkdir -p $(@D)
	$(info CC  $@)
	$(CC) $(TEST_FLAGS) -c -o $@ $<

build/src/%.o: src/%.c
	mkdir -p $(@D)
	$(info CC  $@)
	$(CC) $(CFLAGS) -c -o $@ $<
