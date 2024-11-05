rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SOURCES=$(call rwildcard,src,*.c)
TESTS=$(call rwildcard,test,*.c)
EXAMPLES=$(call rwildcard,example,*.c)
SRC_OBJECTS=$(patsubst %.c, build/%.o, $(SOURCES))
TEST_OBJECTS=$(patsubst %.c, build/%.o, $(TESTS))
EXAMPLE_OBJECTS=$(patsubst %.c, build/%.o, $(EXAMPLES))
EXAMPLE_BINS=$(patsubst %.c, bin/%, $(EXAMPLE_OBJECTS))
DEPENDENCIES=$(patsubst %.c, build/%.d, $(SOURCES)$(TESTS)$(EXAMPLES))

PROJECT_NAME=ctf
TEST_BINARIES=bin/test/$(PROJECT_NAME)
SRC_BINARIES=

TEST_RUN=$(patsubst bin/%, build/%.run, $(TEST_BINARIES))
MEMORY_DEBUG_FLAGS=-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract
WARN_FLAGS=-Wall -Wextra -Werror -Wno-error=cpp -Wno-unused-function -Wunused-result -Wvla -Wshadow -Wstrict-prototypes
SANTIIZER_FLAGS=-fsanitize=undefined -fsanitize-address-use-after-scope -fstack-check -fno-stack-clash-protection
DEBUG_FLAGS=$(WARN_FLAGS) $(MEMORY_DEBUG_FLAGS) $(SANITIZER_FLAGS) -Og -ggdb3
OPTIMIZE_FLAGS=-march=native -O2 -s -pipe -flto=4 -D NDEBUG -fwhole-program
CFLAGS=-std=gnu99 -MMD -MP

all: debug

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

build/test/%.run: bin/test/%
	mkdir -p $(@D)
	$(info RUN $<)
	./$<
	touch $@

bin/test/$(PROJECT_NAME): $(TEST_OBJECTS) $(SRC_OBJECTS)
	mkdir -p $(@D)
	$(info LN  $@)
	$(CC) $(CFLAGS) -o $@ $^

build/src/$(PROJECT_NAME)/%.o: src/$(PROJECT_NAME)/%.c
	mkdir -p $(@D)
	$(info CC  $@)
	$(CC) -c $(CFLAGS) -DCTF_PARALLEL=4 -lpthread -o $@ $<

build/%.o: %.c
	mkdir -p $(@D)
	$(info CC  $@)
	$(CC) -Isrc -c $(CFLAGS) -o $@ $<
