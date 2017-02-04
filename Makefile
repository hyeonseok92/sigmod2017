CC := g++
SRCDIR := src
BUILDDIR := build
TARGET := bin/runner
DBG_TARGET := bin/dbg_runner

SRCEXT := cc
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
DBG_OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.dbg.o))
CFLAGS := -Wall -O3 -D NDEBUG -std=c++11
DBGCFLAGS := -Wall -O0 -std=c++11 -g
LIB := -lpthread -L lib
INC := -I include

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $^"; $(CC) $(CFLAGS) $(INC) -c -o $@ $^

debug: $(DBG_TARGET)

$(DBG_TARGET): $(DBG_OBJECTS)
	@echo " Build with debug..."
	@echo " $(CC) $^ -o $(DBG_TARGET) $(LIB)"; $(CC) $^ -o $(DBG_TARGET) $(LIB)

$(BUILDDIR)/%.dbg.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(DBGCFLAGS) -g $(INC) -c -o $@ $^"; $(CC) $(DBGCFLAGS) $(INC) -c -o $@ $^

clean:
	@echo " Cleaning..."
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

target:
	@echo " Build Spike..."


.PHONY: clean
