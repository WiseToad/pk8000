PLATFORM    = $(shell gcc -dumpmachine)

include makefile.$(PLATFORM).inc

ifneq ($(filter-out %.cpp %.asm, $(SOURCES)), )
$(error Unknown source file type found)
endif

BUILD       = release

CXX         = g++
CXXFLAGS   += -Wall
CXXFLAGS.release = -O2 -DNDEBUG
CXXFLAGS.debug   = -g3

ASM         = fasm
ASMFLAGS   +=
ASMFLAGS.release =
ASMFLAGS.debug   =

LNK         = g++
LNKFLAGS   +=
LNKFLAGS.release = -s
LNKFLAGS.debug   =

ifeq ($(TARGETBASE), )
TARGETBASE  = target
endif

TARGETFILE  = $(TARGETBASE)/$(PLATFORM)/$(BUILD)/$(TARGET)
TARGETDIR   = $(dir $(TARGETFILE))

OBJS        = $(addprefix $(TARGETDIR)/, $(addsuffix .o, $(SOURCES)))
DEPS        = $(addprefix $(TARGETDIR)/, $(addsuffix .d, $(filter %.cpp, $(SOURCES))))

.PHONY: all clean distclean usage

ifneq ($(PHASE), build)
all: $(DEPS)
	@make PHASE=build
else
all: $(TARGETFILE)
endif

clean:
	rm -f $(DEPS) $(OBJS)

distclean:
	rm -f $(DEPS) $(OBJS) $(TARGETFILE)

usage:
	@echo 'Usage: make [all] [BUILD={release|debug}]'
	@echo '       make clean [BUILD={release|debug}]'
	@echo '       make distclean [BUILD={release|debug}]'
	@echo 'By default BUILD=release'

.PRECIOUS: $(TARGETDIR)/. $(TARGETDIR)%/.

$(TARGETDIR)/.:
	@mkdir -p $@

$(TARGETDIR)%/.:
	@mkdir -p $@

.SECONDEXPANSION:

$(TARGETFILE): $(OBJS)
	$(LNK) $(LNKFLAGS) $(LNKFLAGS.$(BUILD)) $(OBJS) $(LIBS) -o $@

$(TARGETDIR)/%.cpp.d: %.cpp | $$(@D)/.
	$(CXX) $(CXXFLAGS) $(CXXFLAGS.$(BUILD)) $(INCLUDE) -MM -MT $(TARGETDIR)/$<.o $< > $@

$(TARGETDIR)/%.cpp.o: %.cpp | $$(@D)/.
	$(CXX) $(CXXFLAGS) $(CXXFLAGS.$(BUILD)) $(INCLUDE) -c $< -o $@

$(TARGETDIR)/%.asm.o: %.asm | $$(@D)/.
	$(ASM) $(ASMFLAGS) $(ASMFLAGS.$(BUILD)) $< $@

ifeq ($(PHASE), build)
include $(DEPS)
endif
