CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -D_FILE_OFFSET_BITS=64

all: spectrum_update universe_issue universe_update

spectrum_update: spectrum_update.cpp state_common.h
	$(CXX) $(CXXFLAGS) -o $@ spectrum_update.cpp

universe_issue: universe_issue.cpp state_common.h
	$(CXX) $(CXXFLAGS) -o $@ universe_issue.cpp

universe_update: universe_update.cpp state_common.h
	$(CXX) $(CXXFLAGS) -o $@ universe_update.cpp

clean:
	rm -f spectrum_update universe_issue universe_update

.PHONY: all clean
