# HALL (Haifa AllSAT)

HALL is an AllSAT enumeration tool for single output combinational circuits in AIGER format.

If you use this tool, please cite our paper Dror Fried, Alexander Nadel, Yogev Shalmon, "AllSAT for Combinational Circuits", SAT'23. Check the [**References**](#references) section for additional details.

## Summary

Given a combinational circuit in AIGER format with a single output which evaluates to 1, *HALL* generates an AllSAT enumeration, that is, it generates all the solutions (satisfying assignments) in Disjunctive Normal Form (DNF). For more information about the AIGER format please visit the page: http://fmv.jku.at/aiger/. 

The solutions are generated in anytime hashion. They  are represented using assignments to the inputs only, that is, only solutins to the the circuit inputs are enumerated.
The tool utilizes **ternary** logic, which extends the standard Boolean Logic values 0 and 1 with an additional value called the don't-care value (denoted by X), which means that the assignment is satisfying regardless of the variable's value, allowing for succinctly represent solutions.

Internally, the incremental SAT solver - "Intel SAT(R) Solver" is being used, please check the repository https://github.com/alexander-nadel/intel_sat_solver for more details.

The solutions returned by HALL can be either disjoint (no overlap) or non-disjoint (may overlap), please check the section [**disjoint and non-disjoint solutions**](#disjoint-vs-non-disjoint-solutions) under [**How to use HALL**](#how-to-use-hall) for more info.

HALL implements different algorithms, please check [**HALL algorithms**](#hall-algorithms) for more details.

## How to build HALL

Please consider the following before continuing: 
- Compilation requires g++ version 10.1.0 or higher.
- We use CMake for building the tool, please verify that you have CMake VERSION 3.8 or higher.

To build the tool, just clone the repository and run this commands(after entering the repository directory)

1.  ```git submodule init```
2.  ```git submodule update```
3.	```cmake -S . -B build```
4.  ```cd build```
5.  ```make```

This will create new folder named "**build**" and will compile the tool in release mode.

this should generate the tool **hall_tool**.

After building the tool in the "build" directory, you should be able to run the tool, for example ask for help:

```
./hall_tool -h
```

## How to use HALL

HALL receives as an input an AIGER file (ascii or binary), which should describe a combinational circuit containing only **one** output.

HALL assumes that the circuit output must evaluate to 1 and enumerates all the solutions to the inputs, which satisfies the output.

For each assignment, the value of an input variable v can either be 1 (positive), 0 (negative) or x (*don't-care*), where the assignment is described by providing a cube (conjunction of literals), where the sign of the variable (v or -v) correspond to the value (1 or 0 respectively) in the current assignment, while don't-care values are not included.

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
./hall_tool ../benchmarks/AND.aag --print_enumer
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

HALL contains several different algorithms, where each algorithm can be executed by HALL using "<-mode> <mode_name>" after the input file name, for example:

```
./hall_tool ../benchmarks/AND.aag -mode tale
```

Here is a list of the algorithms (<mode_name>), sorted according to whether they generate disjoint (mars-dis) or non-disjoint solutions:

disjoint solutions algorithms:

- mars-dis

non-disjoint solutions algorithms:

- tale
- mars-nondis
- duty

## References

HALL is introduced in the following paper: Dror Fried, Alexander Nadel, Yogev Shalmon, "AllSAT for Combinational Circuits", [SAT2023](http://satisfiability.org/SAT23/index.html).
