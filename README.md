# HALL (Haifa AllSAT)

HALL solves the AllSAT problem, that is, given a single-output combinational circuits, it generates all its solutions, where each solution is a ternary assignment to the circuit's inputs which entail its output.

If you use this tool, please cite our paper Dror Fried, Alexander Nadel, Yogev Shalmon, "AllSAT for Combinational Circuits", SAT'23. Check the [**References**](#references) section for additional details.

## Summary

Given a combinational circuit in AIGER format (http://fmv.jku.at/aiger/) with a single output which evaluates to 1, *HALL* generates all its solutions. The solutions are generated in anytime fashion (that is, the tool keeps finding and outputting new solutions till completion or time-out). Every solution is an ternary assignment to the circuit inputs which entails the circuit, that is, every input in every solution is assigned either 0, 1 or the don't care value (X).

HALL supports multiple SAT solvers (IntelSAT, Cadical, MergeSAT, CryptoMinisat), where, by default, IntelSAT is used for the so-called "plane" SAT instance, and cadical is applied for the "dual" SAT instance. (see [**Building HALL with different SAT solvers**](#building-hall-with-different-sat-solvers))

The solutions returned by HALL can be either disjoint (no overlap) or non-disjoint (may overlap), please check the section [**disjoint and non-disjoint solutions**](#disjoint-vs-non-disjoint-solutions) under [**How to use HALL**](#how-to-use-hall) for more info.

HALL implements different algorithms, please check [**HALL algorithms**](#hall-algorithms) for more details.

For information on how to repoduce the experiments in our SAT'24 submission Dror Fried, Alexander Nadel, Roberto Sebastiani, Yogev Shalmon, "Entailing Generalization Boosts Enumeration", see: [**Reproducing the experiments in our SAT'24 submission**](#reproducing-the-experiments-in-our-sat24-submission)

## How to build HALL

Please consider the following before continuing: 
- Compilation requires g++ version 10.1.0 or higher.
- We use CMake for building the tool, please verify that you have CMake VERSION 3.8 or higher.

To build the tool, just clone the repository and run this commands(after entering the repository directory)

```
git submodule init
git submodule update
cmake -S . -B build
cd build
make
```

This will create new folder named "**build**" and will compile the tool in release mode.

this should generate the tool **hall_tool**.

After building the tool in the "build" directory, you should be able to run the tool, for example ask for help:

```
./hall_tool -h
```

## Building HALL with different SAT solvers

HALL supports multiple SAT solvers (IntelSAT, Cadical, MergeSAT, CryptoMinisat), where, by default, IntelSAT (https://github.com/alexander-nadel/intel_sat_solver) is used for the so-called "plane" SAT instance, and an IPASIR solver is applied for the "dual" SAT instance. The default IPASIR solver is cadical, to configure different solvers please add the following to the cmake command:

```cmake -S . -B build -DIPASIR_SAT_SOLVER=%SOLVER_NAME%```

Where %SOLVER_NAME% can be either [CADICAL, CRYPTOMINISAT, MERGESAT], for example:

```cmake -S . -B build -DIPASIR_SAT_SOLVER=CRYPTOMINISAT```


### Changing the SAT solver for the plain and dual instances

By default, IntelSAT is used for the "plain" SAT instance, and an IPASIR solver is applied for the "dual" SAT instance.
This can be controlled with additional parameters for HALL.

To change the solver for the "plain" instance to the IPASIR solver, please add the following parameter to HALL:

```hall_tool .... /alg/blocking/use_ipasir_for_plain 1```

To change the solver for the "dual" instance to the IntelSAT solver, please add the following parameter to HALL:

```hall_tool .... /alg/blocking/use_ipasir_for_dual 0```


## How to use HALL

HALL receives as an input an AIGER file (ascii or binary), which should describe a combinational circuit containing only **one** output.

HALL assumes that the circuit output must evaluate to 1 and enumerates all the solutions to the inputs, which entail the output.

For each assignment, the value of an input variable v can either be 1 (positive), 0 (negative) or x (*don't-care*), where the assignment is described by providing a cube (conjunction of literals), where the sign of the variable (v or -v) correspond to the value (1 or 0 respectively) in the current assignment, while don't-care values are not shown.

In the AIGER format, variables are described using non-negative integers (literals), where even numbers represent positive variables and odd numbers represent negated variables, where the inputs are always positive (even).

HALL represent variables with respect to their *index*, that is, for variable with literal 2 the variable value in the assignment can be either 1, -1 or none, which represent that the variable was assigned X (recall that HALL only enumerate the circuit's inputs).

For example, consider the following AIGER model describing a simple AND gate (see also benchmarks/AND.aag):

```
aag 3 2 0 1 1
2
4
6
6 2 4
```

The single solution where both inputs are equal to 1 is represented by the assignment "1 2" in HALL's output.

The following command reproduces this result by running HALL with the AIGER file "AND.aag", provided under the benchmarks folder:

```
./hall_tool ../benchmarks/AND.aag /general/print_enumer 1
```

### Disjoint vs. non-disjoint solutions

An important feature in HALL is that it can generate disjoint or non-disjoint solutions, depending on the user needs, where disjoint solutions do not overlap, and non-disjoint solutions may overlap.
HALL contains different algorithms for the needs of disjoint and non-disjoint solution generation, for more details, refer to the [**HALL algorithms**](#hall-algorithms) section.

Since a solution with don't-care values incorporates multiple (2^{number of don't cares} to be exact) solutions with only 0/1 values, solutions may overlap, that is, a total solution may be contained in more than one partial solutions, returned by HALL. 


For example, assume we want to represent the following solutions with don't-care values:

```
1 2 3
-1 2 3
1 -2 3
```
An example for disjoint solutions with no overlap is (where in the first assignment 1 = X):

```
2 3 
1 -2 3
```

The folowing is an example for non-disjoint partial solutions (where the totoal solution "1 -2 3" is represented by by both partial solutions):

```
2 3
1 3
```


## HALL algorithms

HALL contains several different algorithms, where each algorithm can be executed by HALL using "/mode <mode_name>" after the input file name, for example:

```
./hall_tool ../benchmarks/AND.aag /mode tale
```

Here is a list of the algorithms (<mode_name>), sorted according to whether they generate disjoint (mars-dis) or non-disjoint solutions:

disjoint solutions algorithms:

- mars-dis

non-disjoint solutions algorithms:

- tale
- mars-nondis
- duty
- core
- roc
- carma

## Reproducing the experiments in our SAT'24 submission

This section lays out how to reproduce the experiments, reported in Tables 1 and 2 in our SAT'24 submission.

### Benchmarks

In total we used 140 benchmarks, consist of 14 benchmark families with 10 benchmarks each.
For each benchmark familiy, we specify the exact benchmark directory under the main "benchmarks" directory:

- random_control_(or/xor/only_last_out): benchmarks/islis_benchmarks/random_control/
- arithmetic_(or/xor/only_last_out): benchmarks/islis_benchmarks/arithmetic/
- random_circuits_(or/xor/only_last_out): benchmarks/random_circuits/
- iscas85_(or/xor/only_last_out): benchmarks/iscas85/
- sta_gen: benchmarks/sta_benchmarks/sta_large/
- sta_gen_chunks: benchmarks/sta_benchmarks/sta_chunks_large/


### Reproducing Table 1

To reproduce Table 1, please use the default IPASIR solver, cadical (no need use -DIPASIR_SAT_SOLVER=):

Additionally, only the mode parameter of HALL needs to be controlled, which is specified with "/mode <mode_name>". We specify the command-line for every mode used in Table 1:

- TALE: ```hall_tool %BENCH_FILE_PATH% /mode tale```
- MARS: ```hall_tool %BENCH_FILE_PATH% /mode mars-nondis```
- DUTY: ```hall_tool %BENCH_FILE_PATH% /mode duty```
- CORE: ```hall_tool %BENCH_FILE_PATH% /mode core```
- ROC: ```hall_tool %BENCH_FILE_PATH% /mode roc```
- CARMA: ```hall_tool %BENCH_FILE_PATH% /mode carma```

To reproduce dualiza, clone the dualiza repository from GitHub (https://github.com/arminbiere/dualiza). The default mode of dualiza is sat (dualiza_sat). To use the bdd mode (dualiza_bdd), add the parameter "-b".

### Reproducing the TALE configurations Table (Table 2)

To reproduce this Table, use the mode "tale" (hall_tool ... /mode tale), where additional steps are required for changing the "plain" SAT solver.

To change the "plain" instance SAT solver please first build the tool with the desired IPASIR solver. See [**Building HALL with different SAT solvers**](#building-hall-with-different-sat-solvers) for more details.

Then, add the following parameter "***/alg/blocking/use_ipasir_for_plain 1***":

```hall_tool %BENCH_FILE_PATH% /mode tale /alg/blocking/use_ipasir_for_plain 1```

To get the "Backward-TerSim" configuration, please use the mode "tale" in the default configuration (intelSAT for "plain"), where additionally, the following parameter is required "***/alg/blocking/use_top_to_bot_sim 1***":

```hall_tool %BENCH_FILE_PATH% /mode tale /alg/blocking/use_top_to_bot_sim 1```

### Reproducing the CORE configurations Table (Table 2)

To reproduce this Table, use the mode "core" (hall_tool ... /mode core), where additional steps are required for changing the "dual" SAT solver.
By default, CORE utilizes intelSAT for the "plain" instance and cadical for the "dual". To change the SAT solver for the "dual" instance, please first build the tool with the desired IPASIR solver. See [**Building HALL with different SAT solvers**](#building-hall-with-different-sat-solvers) for more details.

The IPASIR solver is by default chosen for the "dual" instance. To use IntelSAT for the "dual" instance, please add the following parameter (the IPASIR solver configuration does not matter) "***/alg/blocking/use_ipasir_for_dual 0***":

```hall_tool %BENCH_FILE_PATH% /mode core /alg/blocking/use_ipasir_for_dual 0```

To get the "No-UC-Minimization" configuration, please use the mode "core" in the default configuration (intelSAT for "plain", cadical for "dual"), where additionally, the following parameter is required "***/alg/blocking/use_lit_drop 0***":

```hall_tool %BENCH_FILE_PATH% /mode core /alg/blocking/use_lit_drop 0```.


## References

HALL is introduced in the following paper: Dror Fried, Alexander Nadel, Yogev Shalmon, "AllSAT for Combinational Circuits", [SAT2023](http://satisfiability.org/SAT23/index.html).

Additional developments are described in our current SAT'24 accepted paper: Dror Fried, Alexander Nadel, Roberto Sebastiani, Yogev Shalmon, "Entailing Generalization Boosts Enumeration".
