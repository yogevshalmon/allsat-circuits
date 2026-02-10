HALL_DIR = ..
CXX = g++
CXXFLAGS = -std=c++20 -I$(HALL_DIR)/include -I$(HALL_DIR)/src

# All required libraries - static linking requires explicit listing of dependencies
LIBS = $(HALL_DIR)/build/liballsat.a \
       $(HALL_DIR)/libs/sat/cadical/build/libcadical.a \
       $(HALL_DIR)/libs/intel_sat_solver/libintel_sat_solver.a

simple_test: simple_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f simple_test

.PHONY: clean
