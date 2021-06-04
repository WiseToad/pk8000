TARGETBASE  = ../target

TARGET      = # specified separately in include files for each supported platform

SOURCES     = base/filesys.cpp base/stringf.cpp \
              emu/audio/comparator.cpp emu/audio/compressor.cpp \
              emu/controllers/keyboard.cpp emu/controllers/joysticks.cpp \
              emu/debug/dumper.cpp emu/debug/keylogger.cpp emu/debug/tracer.cpp \
              emu/bios.cpp emu/cpu.cpp emu/libretro.cpp emu/machine.cpp \
              emu/media.cpp emu/memory.cpp emu/timeline.cpp emu/video.cpp \
              filefmt/cas.cpp filefmt/wav.cpp \
              logging/filelog.cpp logging/logfilter.cpp logging/logging.cpp \
              streams/cvtstream.cpp streams/file.cpp streams/reverse.cpp

INCLUDE     = -Ibase -Iemu -Iemu/controllers -Iemu/debug -Ifilefmt -Ilogging -Istreams

LIBS        =

CXXFLAGS    = -std=c++17 -fPIC

LNKFLAGS    = -shared

include makefile.inc
