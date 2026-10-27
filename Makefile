# Basic Makefile structure taken from this tutorial http://wiki.osdev.org/Makefile

libs=vulkan glfw3

DOCS_FILES = docs/manual/index.dox Doxyfile
AUXFILES = $(DOCS_FILES) $(TAGS_FILE)
SRC_DIR = src
INCL_DIR = include
PROJDIRS = $(SRC_DIR) $(INCL_DIR) 
SPLDIRS  = samples

GLSLC=glslc

TAGS = tags
DOCS = docs/html/index.html

SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")

PRIV_HDR_FILES = $(shell find $(SRC_DIR) -type f -name "*.h")
PUBL_HDR_FILES = $(shell find $(INCL_DIR) -type f -name "*.h")

HDRFILES = $(PRIV_HDR_FILES) $(PUBL_HDR_FILES)

OBJFILES = $(patsubst src/%,bin/objects/%,$(patsubst %.c,%.o,$(SRCFILES)))
DEPFILES = $(patsubst %.o,%.d,$(OBJFILES))
OBJDIRS := $(sort $(dir $(OBJFILES)))

SPLFILES = $(shell find $(SPLDIRS) -type f -name "*.c")
SPLOBJFILES = $(patsubst samples/%,bin/samples/%,$(patsubst %.c,%.o,$(SPLFILES)))
SPLOBJDIRS = $(dir $(SPLOBJFILES))
SAMPLES = $(patsubst samples/%/main.c,bin/%, $(SPLFILES))
SPLSHADERS = $(shell find $(SPLDIRS) -type f -name "*.vert" -o -name "*.frag")
SPLSPVFILES = $(patsubst samples/%,bin/samples/%,$(patsubst %,%.spv,$(SPLSHADERS)))

.PHONY: all clean todolist samples 

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wconversion -Wstrict-prototypes

CFLAGS := -g -std=gnu99 $(WARNINGS)

CFLAGS += -I./include -I./src
CFLAGS += $(shell pkg-config --cflags $(libs)) 

LDFLAGS = $(shell pkg-config --libs $(libs))
LDFLAGS += -lm

all: libmosaic.a samples docs tags

code: libmosaic.a samples

shaders: $(SPLSPVFILES)

libmosaic.a: $(OBJFILES)
	@$(AR) r bin/libmosaic.a $?

clean:
	-@$(RM) $(wildcard $(OBJFILES) $(DEPFILES) $(SPLOBJFILES) $(SAMPLES) bin/libmosaic.a)
	-@$(RM) -r bin/objects
	-@$(RM) -r bin/samples

clean-all: clean clean-docs

todolist:
	# Seemingly broken at this moment
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

# Source files
bin/objects/%.o: src/%.c Makefile | $(OBJDIRS) 
	$(info compiling library object: $@)
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ 

samples: $(SAMPLES) $(SPLSPVFILES)

# Link samples 
bin/%: $(SPLOBJFILES) libmosaic.a
	@$(CC) -o $@ $< bin/libmosaic.a $(LDFLAGS)

# Sample objects 
bin/samples/%.o: samples/%.c Makefile | $(SPLOBJDIRS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@ 
	$(info compiling sample object: $@)

# Sample shaders
bin/samples/%.spv: samples/% Makefile | $(SPLOBJDIRS)
	$(info compiling shader: $<)
	@$(GLSLC) $< -o $@


$(OBJDIRS): 
	@mkdir -p $@ 

$(SPLOBJDIRS):
	@mkdir -p $@
	
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
