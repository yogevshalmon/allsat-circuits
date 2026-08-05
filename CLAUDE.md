# CLAUDE.md

Guidance for Claude Code working in this repository.

HALL (Haifa AllSAT) solves AllSAT for single-output combinational circuits: given an AIGER circuit, it enumerates all ternary input assignments (0 / 1 / X) that *entail* the output. It ships as a CLI (`hall_tool`) and a static library (`liballsat.a`).

Companion documents: [README.md](README.md) is the user-facing manual, [DEVELOPING.md](DEVELOPING.md) is the full developer guide. **Read DEVELOPING.md before any non-trivial change** — this file is the short version plus the things that are easy to get wrong.

## Build, test, run

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

./build/allsat_lib_tests                                  # from the repo root, ~2s
./build/hall_tool benchmarks/AND.aag /general/print_enumer 1
```

The first build compiles the SAT solvers under `libs/` and takes a few minutes; later builds are incremental. Options: `IPASIR_SAT_SOLVER` (`CADICAL` default, or `CRYPTOMINISAT` / `MERGESAT`), `BUILD_TESTS`, `USE_DEBUG`, `USE_64b_INDEX_SOLVER`, `USE_COMPRESS_SOLVER`. Benchmark numbers must come from the default CaDiCaL build.

`./build/allsat_lib_tests` **must be run from the repository root** — a few tests load files from `benchmarks/`.

## Architecture in one picture

```
src/main.cpp (CLI argv)  ─┐
                          ├─→ AllSatConfig ─→ AllSatAlgo{Tseitin,DualRail}Enc ─→ InitializeFromAiger(IAigerView)
src/Api/AllSatLib.cpp    ─┘                                                        ↓
(EnumerateOptions)                                        BeginEnumeration(), then NextModel() until Exhausted
```

- `src/Globals/AllSatConfig.hpp` is the **single source of truth** for configuration. Nothing below `src/main.cpp` and `src/Api/` parses command-line strings; `InputParser` is CLI-only.
- `src/Aiger/IAigerView.hpp` abstracts the circuit. `AigerParser` (from a file) and `AigerMemory` (from `AigBuilder`) both implement it, so file-based and in-memory circuits are interchangeable.
- The enumeration loop is `AllSatAlgoBlockingBase::NextModel`: solve → generalize → block → solve again. `FindAllEnumer` (the CLI) is just a loop over it, so CLI and library share one code path.
- **Plain vs dual instance**: the plain solver holds the circuit plus blocking clauses and produces candidates; the dual solver holds the negated circuit and generalizes a model by returning an unsat core. IntelSAT is the plain default, CaDiCaL (via IPASIR) the dual default.

## Things that are easy to get wrong

- **Literals vs indices.** `AIGLIT` is an AIGER literal (even = positive, odd = negated); `AIGINDEX` is `lit >> 1`. User-facing surfaces (printing, `/general/projection_vars`) speak **indices**; internals speak **literals**. This is the most common source of confusion here.
- **Inputs and AND gates share one index space.** The third input is not index 3 if a gate was created in between. There is a regression test for exactly this (`non consecutive inputs`).
- **Any output from library-reachable code must be guarded by `m_PrintInfo`**, otherwise library users get stray text on stdout.
- **A reported model never contains `TVal::DontCare`.** The dual-rail encodings emit explicit don't-care entries internally, the Tseitin ones do not; `Enumerator::Next` normalizes that away. An input absent from a cube *is* don't-care.
- **Reject impossible configurations up front**, with a message that names the problem. Two live examples: `litDropConflictLimit > 0` (no backend implements `SetConflictLimit`) and dual-rail polarity/score hints combined with `useIpasirForPlain` (only IntelSAT implements them).
- **Adding an option touches six places** and **adding a mode touches five** — both lists are in DEVELOPING.md. Presets and CLI mode names must stay in sync.
- Existing misspellings (`AllSatGloblas.hpp`, `GetAndGated`, `TVal::UnKown`, "unkown" in messages) are kept deliberately for consistency. Don't opportunistically rename them.

## Performance is a correctness-level concern

This is a solver. A "harmless refactor" in the enumeration, generalization or simulation loop can cost a factor of two, and it will not show up in the tests.

**Measure before and after any change to** `AllSatAlgoBlockingBase`, `AllSatSolverBase::GetUnSATCore`, `CirSim`, or a submodule pin. Build the before and after into separate directories, run a fixed benchmark set at a fixed timeout, and compare *assignments produced* for runs that hit the timeout and *cpu time* for runs that complete. Both are printed by the tool. The protocol is written up in DEVELOPING.md.

**CaDiCaL is pinned to rel-2.0.0 on purpose.** Newer versions are much slower on this workload (roc 0.64x, core 0.80x). Do not bump it without re-measuring — DEVELOPING.md has the numbers and what was already ruled out.

## Testing

`tests/allsat_lib_tests.cpp` is one self-contained executable, no framework: each test throws on failure, `main` runs them all and reports every failure. Add new tests to the `kTests` table.

The highest-value tests are the brute-force ones: for a circuit small enough to evaluate exhaustively, the complete solution set is computed by hand and the enumeration is checked against it *exactly*, across every preset, projected and non-projected. That is what catches unsoundness — the failure mode that actually matters for an AllSAT tool, and the one that bit this codebase before. Prefer extending `TestRandomCircuitsAgainstBruteForce` / `TestNonDefaultOptionsAgainstBruteForce` over writing new hand-computed expectations.

Helpers available: `EvaluateCircuit`, `BruteForceSolutions`, `Enumerate`, `ExpandCubes`, `CheckAgainstBruteForce`, `QuietOptions`.

CI (`.github/workflows/build-linux.yml`) builds the default CaDiCaL configuration, runs the tests, smoke-tests the CLI, and builds the standalone example. Linux only — Windows support was removed deliberately. The other IPASIR backends are not in CI yet: MergeSAT fails to link because `libintel_sat_solver.a` ships a `Main.o` defining `main`, which collides with ours under that link order.

## Conventions

- C++20, g++ >= 10.1.0. Match the surrounding style: `m_` members, `PascalCase` methods, allman braces, comments in lowercase prose.
- Do not add a dependency or a build system. `libs/` is submodules only.
- Solvers are built in place inside their submodules, so `git status` may show them dirty. That is expected; never commit those artifacts.
