# Tests the platform-independent core against the CVector/CMatrix/CQuat library.
# Override the library location with:  make LIB=/path/to/library
LIB      ?= /Users/g.r.akhtar/Documents/Claude Overflow/C++ Vector Exercise
CXX      ?= c++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -pedantic
CORE      = ../ArrowsMetal/Core

core_tests: core_tests.cpp $(CORE)/Scene.cpp $(CORE)/ArrowMesh.cpp $(CORE)/*.h
	$(CXX) $(CXXFLAGS) -I"$(LIB)" -I$(CORE) core_tests.cpp $(CORE)/Scene.cpp $(CORE)/ArrowMesh.cpp -o core_tests

check: core_tests
	./core_tests

clean:
	rm -f core_tests

.PHONY: check clean
