rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SOURCES=$(call rwildcard,src,*.c)
TESTS=$(call rwildcard,test,*.c)
LINKER_FLAGS=$(patsubst %.c, build/%.lf, $(TESTS))
SRC_OBJECTS=$(patsubst %.c, build/%.o, $(SOURCES))
TEST_OBJECTS=$(patsubst %.c, build/%.o, $(TESTS))
DEPENDENCIES=$(patsubst %.c, build/%.d, $(SOURCES)$(TESTS))
TEST_BIN=tests
TEST_RUN=build/$(TEST_BIN).run

MEMORY_DEBUG_FLAGS=-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract
WARN_FLAGS=-Wall -Wextra -Werror -Wno-error=cpp -Wno-unused-function -Wunused-result -Wvla -Wshadow -Wstrict-prototypes -Wno-maybe-uninitialized -Wno-logical-not-parentheses
SANITIIZER_FLAGS=-fsanitize=undefined -fsanitize-address-use-after-scope -fstack-check -fno-stack-clash-protection
DEBUG_FLAGS=$(WARN_FLAGS) $(SANITIZER_FLAGS) $(MEMORY_DEBUG_FLAGS) -Og -ggdb3 -MMD -MP
OPTIMIZE_FLAGS=-march=x86-64-v3 -O2 -pipe -D NDEBUG
TEST_LINK_FLAGS=$(LINK_FLAGS) -lpthread
LINK_FLAGS=$(BASE_CFLAGS) -flto=4 -fwhole-program
TEST_FLAGS=$(CFLAGS) -Ibuild/src -Wno-shadow -Wno-unused-parameter
BASE_CFLAGS=-std=gnu11
CFLAGS=$(BASE_CFLAGS)

all: release

release: CFLAGS += $(OPTIMIZE_FLAGS)
release: dist doc

debug: CFLAGS += $(DEBUG_FLAGS)
debug: dist $(TEST_RUN)

check: $(TEST_RUN)

clean:
	rm -rf build bin dist

dist: dist/include/ctf/ctf.h dist/lib64/libctf.so dist/share/doc/ctf/ctf.pdf

install: dist
	install -D dist/include/ctf/ctf.h /usr/include/ctf/ctf.h
	install -D dist/lib64/libctf.so /usr/lib64/libctf.so

uninstall:
	rm -r /usr/include/ctf/ctf.h /usr/lib64/libctf.so

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.DELETE_ON_ERROR:
.PHONY: all release debug check clean dist install uninstall doc
$(VERBOSE).SILENT:
$(shell mkdir -p $(dir $(DEPENDENCIES)))
-include $(DEPENDENCIES)

build/doc/ctf.pdf: doc/ctf.tex
	mkdir -p $(@D)
	$(info DOC  $@)
	cd doc; latexmk -pdf ctf.tex > /dev/null

dist/share/doc/ctf/ctf.pdf: build/doc/ctf.pdf
	mkdir -p $(@D)
	$(info COPY $@)
	cp $^ $@

dist/include/ctf/ctf.h: build/src/ctf/ctf.h
	mkdir -p $(@D)
	$(info COPY $@)
	cp $^ $@

dist/lib64/libctf.so: build/src/ctf/ctf.c
	mkdir -p $(@D)
	$(info SO   $@)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^

$(TEST_RUN): bin/$(TEST_BIN)
	mkdir -p $(@D)
	$(info RUN  $<)
	./$< -j 2 --cleanup --sigsegv
	touch $@

bin/$(TEST_BIN): $(TEST_OBJECTS) build/src/ctf/ctf.o | build/test/$(TEST_BIN).lf
	mkdir -p $(@D)
	$(info LN   $@)
	$(CC) $(LINK_FLAGS) $(TEST_FLAGS) `cat $|` -o $@ $^

build/test/$(TEST_BIN).lf: $(TESTS)
	$(info FLG  $@)
	grep -h '^\s*\(CTF_\)\?MOCK(' $^ | sed 's/\s*\(CTF_\)\?MOCK([^,]\+,\s*\([^ ,]\+\)\s*,.*/,--wrap=\2/' | sort | uniq | tr -d '\n' | sed 's/^,/-Wl,/' > $@

build/test/%.c: test/%.c | build/src/ctf/ctf.h
	mkdir -p $(@D)
	$(info E    $@)
	$(CC) $(TEST_FLAGS) -E -o $@ $<

build/test/%.o: test/%.c | build/src/ctf/ctf.h
	mkdir -p $(@D)
	$(info CC   $@)
	$(CC) $(TEST_FLAGS) -c -o $@ $<

build/test/main.o: test/main.c | build/src/ctf/ctf.h
	mkdir -p $(@D)
	$(info CC   $@)
	$(CC) $(TEST_FLAGS) -c -o $@ $<

build/src/%.o: build/src/%.c build/src/%.h
	mkdir -p $(@D)
	$(info CC   $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/src/ctf/ctf.c: src/ctf/ctf.c $(call rwildcard, src/ctf, *.c)
	mkdir -p $(@D)
	$(info GEN  $@)
	m4 -Im4 -Isrc/ctf $< > $@

build/src/ctf/ctf.h: src/ctf/ctf.h $(call rwildcard, src/ctf, *.h)
	mkdir -p $(@D)
	$(info GEN  $@)
	m4 -Im4 -Isrc/ctf $< > $@

build/%: %
	mkdir -p $(@D)
	$(info GEN  $@)
	m4 -Im4 $< > $@
