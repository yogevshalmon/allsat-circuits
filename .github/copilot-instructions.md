# HALL (Haifa AllSAT) - Project Overview

Full documentation lives in [README.md](../README.md) (usage) and [DEVELOPING.md](../DEVELOPING.md) (architecture, conventions, how to extend the code). Read DEVELOPING.md before making non-trivial changes.

## Purpose
HALL solves the AllSAT problem for single-output combinational circuits: generates all ternary input assignments (0/1/X don't-care) that entail the circuit output evaluates to 1. It can also enumerate over a subset of the inputs (projected enumeration).

## Key Technologies
- **Language**: C++20 (requires g++ ≥10.1.0)
- **Build System**: CMake ≥3.10, Linux only
- **Input Format**: AIGER (ASCII or binary) combinational circuits, or built in memory via `AigBuilder`
- **SAT Solvers**: IntelSAT (default for "plain"), CaDiCaL/CryptoMiniSat/MergeSAT via IPASIR (default for "dual")

## Architecture
- **src/Aiger**: AIGER circuit parsing and in-memory representation (`IAigerView` interface)
- **src/AllSatAlgo**: AllSAT enumeration algorithms (TALE, MARS, DUTY, CORE, ROC, CARMA)
- **src/AllSatSolver**: SAT solver integration layer
- **src/CirSimulation**: ternary (don't-care maximizing) circuit simulation
- **src/Globals**: shared types, `AllSatConfig` (the single source of truth for configuration) and the presets
- **src/Api**: public library interface implementation
- **include/allsat**: public header for library usage (`AllSatLib.hpp`)
- **libs/**: external dependencies as submodules (intel_sat_solver, cadical, etc.)

## Main Artifacts
- **hall_tool**: standalone CLI executable for AllSAT enumeration
- **liballsat.a**: static library for C++ integration

## Solution Types
- **Disjoint**: No overlap between solutions (mode: mars-dis)
- **Non-disjoint**: Solutions may overlap (modes: tale, mars-nondis, duty, core, roc, carma)

## Build Commands
Always prefer building with CADICAL which is the default IPASIR solver and provides the best performance. To build with a different IPASIR solver, specify the `-DIPASIR_SAT_SOLVER` option to CMake.
```bash
git submodule init && git submodule update
cmake -S . -B build -DBUILD_TESTS=ON [-DIPASIR_SAT_SOLVER=CADICAL|CRYPTOMINISAT|MERGESAT]
cmake --build build -j$(nproc)
```

## Testing
- `tests/allsat_lib_tests.cpp`, built with `-DBUILD_TESTS=ON`, run `./build/allsat_lib_tests` from the repo root
- `standalone_test/` checks linking against `liballsat.a` from outside CMake
- Benchmarks in `benchmarks/` (ISCAS85, random circuits, etc.). HALL is a solver: measure before and after any change to the enumeration, generalization or simulation loops

## Conventions
- Configuration flows CLI/library -> `AllSatConfig` -> algorithms. Nothing below `src/main.cpp` and `src/Api/` parses command-line strings
- User-facing interfaces speak AIGER **indices**, internals speak AIGER **literals**
- Any output from library-reachable code must be guarded by `m_PrintInfo`
- Existing misspellings (`AllSatGloblas.hpp`, `GetAndGated`, `TVal::UnKown`) are kept for consistency

## Citations
- SAT'23: "AllSAT for Combinational Circuits" (Fried, Nadel, Shalmon)
- SAT'24: "Entailing Generalization Boosts Enumeration" (Fried, Nadel, Sebastiani, Shalmon), LIPIcs vol. 305, DOI 10.4230/LIPIcs.SAT.2024.13
