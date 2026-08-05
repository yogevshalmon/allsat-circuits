# Developing HALL

This document is for people who want to work **on** HALL rather than just use it. For the user-facing documentation (what HALL does, the command-line interface, the algorithms, how to reproduce the published experiments) see [README.md](README.md).

## Contents

- [Getting set up](#getting-set-up)
- [Build options](#build-options)
- [Repository layout](#repository-layout)
- [Core concepts](#core-concepts)
- [How a run is wired together](#how-a-run-is-wired-together)
- [Adding a new option](#adding-a-new-option)
- [Adding a new mode / preset](#adding-a-new-mode--preset)
- [Projected enumeration](#projected-enumeration)
- [The library interface](#the-library-interface)
- [Tests](#tests)
- [Performance work](#performance-work)
- [Submodules](#submodules)
- [Continuous integration](#continuous-integration)
- [Known rough edges](#known-rough-edges)

## Getting set up

HALL is developed and tested on Linux, with g++ (>= 10.1.0) and CMake (>= 3.10).

```bash
git clone --recurse-submodules <repo-url>
cd allsat-circuits
# if you cloned without --recurse-submodules:
git submodule init && git submodule update

cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

./build/hall_tool benchmarks/AND.aag /general/print_enumer 1
./build/allsat_lib_tests
```

The first build also builds the SAT solvers under `libs/`, so it takes a few minutes. Subsequent builds are incremental, but note that the `intel_sat_solver` and `ipasir_sat_solver` CMake targets are custom targets that always re-run their `make`, so their own makefiles decide what is rebuilt.

## Build options

All of these are passed to CMake, for example `cmake -S . -B build -DUSE_DEBUG=ON`.

| Option | Default | Meaning |
| --- | --- | --- |
| `IPASIR_SAT_SOLVER` | `CADICAL` | IPASIR backend, one of `CADICAL`, `CRYPTOMINISAT`, `MERGESAT`. Anything else disables the IPASIR solver entirely and leaves only IntelSAT. |
| `BUILD_TESTS` | `OFF` | Build `allsat_lib_tests`. |
| `USE_DEBUG` | `OFF` | Debug build (`-g -DDEBUG`), and builds the debug flavour of IntelSAT. |
| `USE_64b_INDEX_SOLVER` | `OFF` | Compile IntelSAT with 64 bit indices (`-DSAT_SOLVER_INDEX_64`). |
| `USE_COMPRESS_SOLVER` | `OFF` | Compile IntelSAT in compressed mode (`-DSAT_SOLVER_COMPRESS`). |

CaDiCaL is the default and the best performing backend, benchmark numbers should be produced with it.

Build targets: `hall_tool` (CLI), `allsat` (`liballsat.a`), `allsat_lib_tests` (with `BUILD_TESTS=ON`).

## Repository layout

```
include/allsat/AllSatLib.hpp   public library header (the only supported public API)
src/Api/AllSatLib.cpp          implementation of the public API, maps options -> AllSatConfig
src/main.cpp                   the hall_tool CLI: argument parsing -> AllSatConfig
src/Globals/                   shared types, the config struct and the presets
src/Aiger/                     AIG representation: the IAigerView interface and its two implementations
src/AllSatAlgo/                enumeration algorithms
src/AllSatSolver/             SAT solver abstraction layer
src/CirSimulation/            ternary (don't-care maximizing) circuit simulation
src/Utilities/                small helpers (command-line parsing, string helpers)
tests/                         library-level tests
standalone_test/               minimal example of linking against liballsat.a from a Makefile
benchmarks/                    AIGER benchmarks, including the families used in the papers
libs/                          submodules: intel_sat_solver, cadical, cryptominisat, mergesat, lorina
```

## Core concepts

**AIGER literals vs indices.** An `AIGLIT` is an AIGER literal: even means the positive variable, odd means its negation. An `AIGINDEX` is the variable index, `lit >> 1`. Inputs always have even literals. `AIGLitToAIGIndex` / `AIGIndexToAIGLit` in `src/Globals/AllSatGloblas.hpp` convert between them. The user-facing interfaces (printing, `/general/projection_vars`) speak **indices**, while the internals mostly speak **literals** — this is the single most common source of confusion in this codebase.

**Ternary values.** `TVal` (`src/Globals/TernaryVal.hpp`) is `True` / `False` / `DontCare` / `UnKown`. An `INPUT_ASSIGNMENT` is a `std::vector<std::pair<AIGLIT, TVal>>`, always over input literals. Don't-care entries are simply not printed.

**Encodings.** `CirEncoding` selects how the circuit is encoded into CNF:
- `TSEITIN_ENC` - standard Tseitin encoding, one SAT variable per AIG index.
- `DUALRAIL_ENC` - each AIG index gets a *pair* of SAT variables (a "DRVAR"), so that the solver itself can assign don't-cares. `GetPos` / `GetNeg` in `src/Globals/AllSatSolverGloblas.hpp` map a `DRVAR` to its two SAT literals.

**Plain vs dual instance.** The "plain" solver holds the circuit and the accumulated blocking clauses, and produces candidate models. The "dual" solver holds the *negated* circuit and is used to generalize a model: a model is generalized by asking the dual solver for an unsat core of the model, which is a sub-cube that still entails the output. By default IntelSAT is used for the plain instance and the IPASIR solver (CaDiCaL) for the dual instance, which is what `use_ipasir_for_plain` / `use_ipasir_for_dual` control.

## How a run is wired together

Both entry points converge on the same struct:

```
hall_tool CLI (src/main.cpp)            library (src/Api/AllSatLib.cpp)
        |  parse argv                            |  EnumerateOptions
        |  ApplyPreset(config, preset)           |  ApplyPreset + per-option overrides
        v                                        v
                        AllSatConfig  (src/Globals/AllSatConfig.hpp)
                                    |
                                    v
        AllSatAlgoTseitinEnc / AllSatAlgoDualRailEnc  (constructed from the config)
                                    |
                                    v
                    AllSatAlgoBlockingBase::InitializeFromAiger(IAigerView)
                                    |
                                    v
        BeginEnumeration()  then  NextModel() until Exhausted / Timeout
```

`AllSatConfig` is the single source of truth for algorithm configuration. Nothing below `src/main.cpp` and `src/Api/` reads command-line strings — `InputParser` is CLI-only.

The enumeration loop itself lives in `AllSatAlgoBlockingBase::NextModel`:

1. Ask the plain solver for a model, read off the input assignment.
2. Generalize it (`GeneralizeModel`, implemented per encoding): optional ternary simulation (`CirSim`) and/or an unsat core from the dual solver (`GetUnSATCore`, optionally with literal dropping).
3. If everything is don't-care, report a tautology.
4. Update the statistics, block the generalized cube in the plain solver (`BlockModel`), and solve again so the next call has a pending status ready.

`FindAllEnumer` (used by the CLI) is just a loop over `NextModel`, so the CLI and the library exercise exactly the same code path.

## Adding a new option

An algorithm option must be added in these places, in this order:

1. `src/Globals/AllSatConfig.hpp` - add the field with its default.
2. The algorithm/solver class - read it in the constructor's initializer list and store it in a `const` member.
3. `src/main.cpp` - add an `ApplyBoolOverride` / `ApplyUIntOverride` call for the `/alg/...` parameter, and a line in `PrintUsage`.
4. `include/allsat/AllSatLib.hpp` - add an `std::optional<T>` field to `EnumerateOptions` (optional so that "not set" is distinguishable from "set to the default", which is what makes preset overriding work).
5. `src/Api/AllSatLib.cpp` - copy it across in `ResolveOptions`.
6. `AllSatAlgoBlockingBase::PrintInitialInformation` (or the relevant subclass) - report it, guarded by `m_PrintInfo`.

Anything printed by library-reachable code must be guarded by `m_PrintInfo`, otherwise library users get stray output on stdout.

## Adding a new mode / preset

Modes are the CLI-visible names, presets are the library-visible enum, and they must stay in sync:

1. Add the mode name constant and add it to `MODES` in `src/Globals/AllSatAlgoGlobals.hpp`.
2. Add the enumerator to `EnumerateOptions::Preset` in `include/allsat/AllSatLib.hpp`.
3. Add the `case` that sets the config fields in `ApplyPreset` (`src/Globals/AllSatPresets.hpp`).
4. Map the mode name to the preset in `TryGetPresetForMode` (`src/main.cpp`).
5. Document it in the README's algorithm list.

`ApplyPreset` switches on the enum without a `default:` case on purpose, so adding an enumerator produces a compiler warning at the place that needs updating.

## Projected enumeration

Projection makes HALL enumerate over a subset of the inputs. The implementation is deliberately localized:

- `AllSatAlgoBase::InitializeProjection` validates the requested indices against the circuit inputs and fills `m_ProjectionInputs` / `m_ProjectionSet` (a set of **literals**, for O(1) lookup). It returns `false` on an invalid index and the caller turns that into an exception.
- `CirSim` takes an optional projection set and refuses to turn non-projection inputs into don't-cares, so the generalized cube keeps the concrete values that are needed to entail the output.
- `AllSatSolverBase::GetUnSATCore` takes the same set, and only tries to *drop* projection literals. Non-projection literals are kept in the core: they never take part in the blocking clause, and keeping them gives the drop check a better chance of removing projection literals. To keep the cheap swap-with-back removal valid, the core is first partitioned so the non-droppable literals sit at the front.
- `BlockModel` blocks `FilterToProjection(model)`, so only projection literals reach the blocking clause. This is what makes the enumeration project: any assignment consistent with the blocked sub-cube is already covered by the reported solution.
- The statistics (`GetNumOfDCFromProjectedAssignment`, average cardinality, model count) are computed over the projection size rather than the input size.

Correctness argument in one line: the generalized cube `c` entails the output, so every assignment extending `c` satisfies the circuit — hence every assignment to the projection inputs consistent with `c` restricted to the projection has a witness, and blocking it removes only covered points.

Consequently, if the projected cube is *empty* while the full cube is not, the projected problem is a tautology: every assignment to the projection inputs has some witness.

## The library interface

`include/allsat/AllSatLib.hpp` is the only supported public header. Things to keep in mind when changing it:

- It is deliberately thin: `EnumerateOptions` (a plain struct of `std::optional`s), `AigBuilder`, and `Enumerator` (a pimpl over `AllSatAlgoBlockingBase`).
- It currently pulls in a few `src/` headers (`AigerMemory.hpp`, `AllSatGloblas.hpp`, `TernaryVal.hpp`), which is why consumers need `src` on the include path in addition to `include`. `target_include_directories(allsat PUBLIC ...)` exports both. If you ever want a truly standalone header, that is the coupling to break.
- `AigBuilder` builds an `AigerMemory`, which implements `IAigerView`. `AigerParser` implements the same interface, so file-based and in-memory circuits are interchangeable everywhere below the API.
- Do not throw across the API for expected outcomes — timeouts and exhaustion are statuses, not exceptions.
- **A reported model never contains `TVal::DontCare`.** Internally the encodings disagree: the dual-rail ones put explicit don't-care entries in the assignment, the Tseitin ones just omit the input. `Enumerator::Next` normalizes that away, because an input that is absent from a cube *is* don't-care, so an explicit entry carries no information. If you add a code path that hands assignments to library users, keep that normalization.
- Configurations that cannot work must be rejected where the message can still be useful. `litDropConflictLimit` is the worked example: no SAT backend implements `SetConflictLimit`, so `AllSatAlgoBlockingBase`'s constructor rejects a non-zero limit rather than letting it surface as "Function not implemented" halfway through an enumeration.

`standalone_test/` is the smoke test for the "someone links against us from outside CMake" path, and its Makefile documents the required link order (`liballsat.a`, then the SAT solver archives).

## Tests

`tests/allsat_lib_tests.cpp` is a single self-contained executable with no test framework — each test is a function that throws on failure, `main` runs them in order and returns non-zero on the first failure.

```bash
cmake -S . -B build -DBUILD_TESTS=ON && cmake --build build -j$(nproc)
./build/allsat_lib_tests            # run from the repo root, one test locates a benchmark relatively
```

Current coverage: a basic AND gate, tautology detection, preset overriding, non-consecutive input indices (a regression test — inputs and AND gates share one index space, so input #3 is not necessarily index 3), projected enumeration, and timeout reporting.

When adding a test, prefer small hand-built circuits via `AigBuilder`, and add it to the list in `main`.

## Performance work

HALL is a solver — a "harmless refactor" in the enumeration or generalization loop can easily cost a factor of two. Before and after any change to `AllSatAlgoBlockingBase`, `AllSatSolverBase::GetUnSATCore`, `CirSim`, or a submodule bump, measure.

A workable protocol:

1. Build the before and after versions into two separate build directories.
2. Run a fixed set of benchmarks under a fixed timeout, for example a few `benchmarks/iscas85/*` and `benchmarks/islis_benchmarks/*` circuits across the `roc`, `core`, `tale`, `carma` and `mars-nondis` modes.
3. Compare *number of assignments* for the runs that hit the timeout (throughput at equal time) and *cpu time* for the runs that complete.

Both numbers are printed by the tool as `c Number of assignments:` and `c cpu time :`. Beware that assignment counts differ legitimately between modes and between SAT solver versions (a different model order means a different set of cubes), so compare like-for-like and look at the order of magnitude rather than exact equality.

## Submodules

| Submodule | What it is |
| --- | --- |
| `libs/intel_sat_solver` | IntelSAT (Topor), the default plain-instance solver. Built via its own `make libr` / `make libd`. |
| `libs/sat/cadical` | CaDiCaL, the default IPASIR (dual-instance) solver. Built via `./configure && make -C build libcadical.a`. |
| `libs/sat/cryptominisat`, `libs/sat/mergesat` | Alternative IPASIR backends, only built when selected. |
| `libs/lorina` | AIGER parser used by `AigerParser`. |

Both SAT solvers are built **in place**, inside the submodule working tree, so a build leaves untracked artifacts there and `git status` reports the submodules as dirty. That is expected. Do not commit those artifacts.

To bump a solver:

```bash
git -C libs/sat/cadical fetch --tags && git -C libs/sat/cadical checkout <tag>
git add libs/sat/cadical
```

then rebuild from scratch and re-run the performance protocol above before committing the bump. A bump is never a formality.

### Why CaDiCaL is pinned to rel-2.0.0

**Do not bump CaDiCaL without re-measuring.** CaDiCaL is only used for the dual instance, and the unsat-core based modes hammer it with a large number of incremental `solve` calls under slowly shrinking assumption sets. Newer CaDiCaL is markedly slower on exactly that pattern. Measured on this repository's benchmarks, at a fixed 20 second budget and holding everything else constant:

| Step | Geomean throughput | Worst case |
| --- | --- | --- |
| 2.0.0 -> 2.2.1 | 0.87 | 0.25 |
| 2.2.1 -> 3.0.1 | 0.98 | 0.81 |
| 2.0.0 -> 3.0.1 | **0.84** | **0.22** |

Broken down per mode for 2.0.0 -> 3.0.1: `roc` 0.64, `core` 0.80, `carma` 0.85, versus `tale` 0.99 and `mars-nondis` 0.97. The loss falls entirely on the modes that use the dual solver, which is the fingerprint of the incremental workload rather than of anything in our code.

This is not the `factor`/BVA default (CaDiCaL's own `ipasir_init` already disables it) and it is not recoverable by turning off the inprocessing that is new since 2.0.0 — explicitly disabling `congruence`, `sweep`, `backbone`, `inprobing` and `fastelim` changed nothing. It is also not a generalization-quality effect: average cardinality is unchanged across versions (~0.41 on the affected benchmarks), the solver simply gets through fewer cubes per second.

By contrast, bumping `intel_sat_solver` to its current upstream tip measured at 1.02 geomean and is fine.

## Continuous integration

`.github/workflows/build-linux.yml` builds on `ubuntu-latest` for the default CaDiCaL configuration, runs the library tests, smoke-tests the CLI on `benchmarks/AND.aag` and `benchmarks/XOR.aag`, and builds the standalone example. It is a compilation and sanity check, not a performance gate — performance is measured locally.

Only CaDiCaL is covered for now. `-DIPASIR_SAT_SOLVER=MERGESAT` does not link: `libintel_sat_solver.a` contains a `Main.o` that defines `main`, and with MergeSAT's link order the linker pulls that object in and collides with `src/main.cpp` (and with the test's `main`). CaDiCaL happens to be linked first and never triggers it. Fixing that — most likely by keeping `Main.o` out of the IntelSAT archive, or by linking the archive so the object is not pulled in — is what has to happen before the alternative backends go back into CI.

## Known rough edges

These are known and intentionally left alone, listed so nobody rediscovers them the hard way:

- Several identifiers are misspelled and kept for consistency (`AllSatGloblas.hpp`, `GetAndGated`, `TVal::UnKown`, "unkown" in messages). Renaming them is a separate, mechanical change.
- `m_NumberOfModels` accumulates `2^(number of don't cares)` and can overflow on large circuits — there is a `TODO` at the site.
- Only single-output circuits are supported, `AigerMemory::SetOutput` and the parser both enforce it.
- `AllSatAlgoBase::FindAllEnumer` and `InitializeWithAIG*` throw "Function not implemented" in the base class instead of being pure virtual.
- The blocking algorithms own their solvers and the simulator with raw `new` in the constructor and `delete` in `~AllSatAlgoBlockingBase`, so those classes are neither copyable nor movable in practice.
- `SetConflictLimit`, and therefore `lit_drop_conflict_limit`, is unimplemented in both solver backends. It is rejected up front (see above). Implementing it would need a backend-specific hook, IPASIR has no conflict limit in its interface.
- `FixPolarity` and `BoostScore` only exist for IntelSAT. The dual-rail presets call them on the plain solver, so combining one of them with `useIpasirForPlain` throws.
