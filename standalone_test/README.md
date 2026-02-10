# Standalone Test Example

This directory contains a simple standalone example showing how to integrate the HALL AllSAT library into a C++ project using a Makefile.

## Prerequisites

First, build the HALL library from the project root:

```bash
cd ..
cmake -S . -B build
cd build
make allsat
```

## Build and Run

```bash
cd standalone_test
make
./simple_test
```

## Expected Output

```
1 2 
Solution 1: 1 2 
Total solutions: 1
```

This demonstrates enumerating the single solution (both inputs = 1) for a simple AND gate.

## Key Integration Points

1. **Include path**: `-I$(HALL_DIR)/include -I$(HALL_DIR)/src`
2. **Required libraries** (order matters):
   - `liballsat.a` - The main HALL library
   - `libcadical.a` - CaDiCaL SAT solver (IPASIR backend)
   - `libintel_sat_solver.a` - Intel SAT solver (default backend)

**Note**: All three libraries must be explicitly linked when using static libraries in a Makefile-based build. CMake handles this automatically through target dependencies.
