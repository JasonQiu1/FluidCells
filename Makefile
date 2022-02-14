CC := /usr/bin/gcc

MKFILEDIR := $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))

SOURCEDIR := $(MKFILEDIR)
BUILDDIR := $(MKFILEDIR)/build
INCLUDEDIR := $(MKFILEDIR)
BINARYDIR := $(MKFILEDIR)
BINARYNAME := fluidcells

LDFLAGS := -lncurses 
CFLAGS := -Wall -g

SOURCES := $(wildcard $(SOURCEDIR)/*.c)
OBJECTS := $(addprefix $(BUILDDIR)/,$(patsubst %.c, %.o, $(SOURCES:$(SOURCEDIR)/%=%)))

$(BINARYDIR)/$(BINARYNAME): $(BUILDDIR) $(OBJECTS)
	echo $(SOURCES)
	echo $(OBJECTS)
	$(CC) $(OBJECTS) -o $(BINARYNAME) $(CFLAGS) $(LDFLAGS) 

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c 
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS) -I$(INCLUDEDIR) 

$(BUILDDIR):
	@mkdir $(BUILDDIR) || true

clean:
	@rm $(wildcard $(BUILDDIR)/*) || true
	@rm $(BINARYDIR)/$(BINARYNAME) || true

run:
	$(BINARYDIR)/$(BINARYNAME)

debug:
	/usr/bin/gdb --tty=/dev/pty1 $(BINARYDIR)/$(BINARYNAME)

.PHONY: run clean debug
