CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
SRC      := src/main.cpp

lifeline: $(SRC) $(wildcard include/*.h)
	$(CXX) $(CXXFLAGS) -Iinclude $(SRC) -o $@

test: lifeline
	sh tests/run_tests.sh

clean:
	rm -f lifeline
	rm -rf tests/tmp

.PHONY: test clean
