# Basic Makefile structure taken from this tutorial http://wiki.osdev.org/Makefile

INC=-I${VULKAN_SDK}/include -I./include 
LIB=-lvulkan

DOCS_FILES = docs/manual/index.dox Doxyfile
AUXFILES = $(DOCS_FILES)
SRC_DIR = src
INCL_DIR = include
PROJDIRS = $(SRC_DIR) $(INCL_DIR) 
SPLDIRS  = samples

DOCS = docs/html/index.html

SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")

PRIV_HDR_FILES = $(shell find $(SRC_DIR) -type f -name "*.h")
PUBL_HDR_FILES = $(shell find $(INCL_DIR) -type f -name "*.h")

HDRFILES = $(PRIV_HDR_FILES) $(PUBL_HDR_FILES)

OBJFILES = $(patsubst src/%,bin/objects/%,$(patsubst %.c,%.o,$(SRCFILES)))
DEPFILES = $(patsubst %.o,%.d,$(OBJFILES))

SPLFILES = $(shell find $(SPLDIRS) -type f -name "*.c")
SPLOBJFILES = $(patsubst samples/%,bin/objects/%,$(patsubst %.c,%.o,$(SPLFILES)))
SAMPLES = $(patsubst samples/%,bin/%,$(patsubst %.c,%,$(SPLFILES)))


.PHONY: all clean todolist samples

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wconversion -Wstrict-prototypes
CFLAGS := -g -std=gnu99 $(WARNINGS)

all: libmosaic.a samples

libmosaic.a: $(OBJFILES)
	@ar r bin/libmosaic.a $?

clean:
	-@$(RM) $(wildcard $(OBJFILES) $(DEPFILES) $(SPLOBJFILES) $(SAMPLES) bin/libmosaic.a)
	-@$(RM) -r bin/objects

todolist:
	# Seemingly broken at this moment
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

bin/objects/%.o: src/%.c Makefile | bin/objects
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ $(INC)
	$(info compiling $@)

samples: $(SAMPLES)

bin/%: bin/objects/%.o libmosaic.a
	@$(CC) -o $@ $< bin/libmosaic.a $(LIB)

bin/objects/%.o: samples/%.c Makefile | bin/objects
	@$(CC) $(CFLAGS) -c $< -o $@ $(INC)
	$(info compiling $@)

bin/objects: 
	mkdir -p bin/objects
	
docs: $(DOCS)

$(DOCS): $(DOCS_FILES) $(PUBL_HDR_FILES)
	$(info Building docs)
	@doxygen

clean-docs: 
	$(info Cleaning docs)
	@rm -rf docs/html docs/latex 
