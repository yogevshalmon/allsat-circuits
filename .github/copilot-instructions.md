# HALL (Haifa AllSAT) - Project Overview

## Purpose
HALL solves the AllSAT problem for single-output combinational circuits: generates all ternary input assignments (0/1/X don't-care) that entail the circuit output evaluates to 1.

## Key Technologies
- **Language**: C++20 (requires g++ ≥10.1.0)
- **Build System**: CMake ≥3.8
- **Input Format**: AIGER (ASCII or binary) combinational circuits
- **SAT Solvers**: IntelSAT (default for "plain"), CaDiCaL/CryptoMiniSat/MergeSAT via IPASIR (default for "dual")

## Architecture
- **src/Aiger**: AIGER circuit parsing and representation
- **src/AllSatAlgo**: AllSAT enumeration algorithms (TALE, MARS, DUTY, CORE, ROC, CARMA)
- **src/AllSatSolver**: SAT solver integration layer
- **src/CirSimulation**: Circuit simulation and ternary simulation
- **src/Api**: Public library interface (AllSatLib.hpp)
- **include/allsat**: Public headers for library usage
- **libs/**: External dependencies (intel_sat_solver, cadical, etc.)

## Main Artifacts
- **hall_tool**: Standalone CLI executable for AllSAT enumeration
- **liballsat.a**: Static library for C++ integration

## Solution Types
- **Disjoint**: No overlap between solutions (mode: mars-dis)
- **Non-disjoint**: Solutions may overlap (modes: tale, mars-nondis, duty, core, roc, carma)

## Build Commands
Always prefer building with CADICAL which is the default IPASIR solver and provides the best performance. To build with a different IPASIR solver, specify the `-DIPASIR_SAT_SOLVER` option to CMake.
```bash
git submodule init && git submodule update
cmake -S . -B build [-DIPASIR_SAT_SOLVER=CADICAL|CRYPTOMINISAT|MERGESAT]
cd build && make
```

## Testing
- Tests in `tests/` directory (enable with `-DBUILD_TESTS=ON`)
- Benchmarks in `benchmarks/` directory (ISCAS85, random circuits, etc.)

## Citations
- SAT'23: "AllSAT for Combinational Circuits" (Fried, Nadel, Shalmon)
- SAT'24: "Entailing Generalization Boosts Enumeration" (Fried, Nadel, Sebastiani, Shalmon)
