# Basic Makefile structure taken from this tutorial http://wiki.osdev.org/Makefile

libs=vulkan glfw3

DOCS_FILES = docs/manual/index.dox Doxyfile
AUXFILES = $(DOCS_FILES) $(TAGS_FILE)
SRC_DIR = src
INCL_DIR = include
PROJDIRS = $(SRC_DIR) $(INCL_DIR) 
SPLDIRS  = samples

TAGS = tags
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

CFLAGS += -I./include
CFLAGS += $(shell pkg-config --cflags $(libs)) 

$(info $(CFLAGS))

LDFLAGS = $(shell pkg-config --libs $(libs))

all: libmosaic.a samples docs tags

code: libmosaic.a samples

libmosaic.a: $(OBJFILES)
	@$(AR) r bin/libmosaic.a $?

clean:
	-@$(RM) $(wildcard $(OBJFILES) $(DEPFILES) $(SPLOBJFILES) $(SAMPLES) bin/libmosaic.a)
	-@$(RM) -r bin/objects

clean-all: clean clean-docs

todolist:
	# Seemingly broken at this moment
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

bin/objects/%.o: src/%.c Makefile | bin/objects
	$(info compiling $@)
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ 

samples: $(SAMPLES)

bin/%: bin/objects/%.o libmosaic.a
	@$(CC) -o $@ $< bin/libmosaic.a $(LDFLAGS)

bin/objects/%.o: samples/%.c Makefile | bin/objects
	@$(CC) $(CFLAGS) -c $< -o $@ 
	$(info compiling $@)

bin/objects: 
	mkdir -p bin/objects
	
docs: $(DOCS)

$(DOCS): $(DOCS_FILES) $(PUBL_HDR_FILES)
	$(info Building docs)
	@doxygen

clean-docs: 
	$(info Cleaning docs)
	@$(RM) -rf docs/html docs/latex 

# tags file is just called "tags", so this target can be called with "make tags"
$(TAGS): $(HDRFILES) $(SRCFILES)
	$(info Generating Tags)
	@ctags -R src/ include/
