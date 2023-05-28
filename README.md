# HALL (Haifa AllSAT)

HALL is an AllSAT enumeration tool for single output combinational circuits in AIGER format.

## Summary

This tool *HALL*, given a combinational circuit in AIGER format with a single output whose evaluates to 1, generates an AllSAT enumeration: all the solutions (satisfying assignments) in Disjunctive Normal Form (DNF). For more information about the AIGER format please visit the page: http://fmv.jku.at/aiger/.

The solutions are represented using assignments to the inputs only, meaning only the circuit inputs are enumerated.
The tool utilizes **ternary** values, which extends the Boolean values 0/1 with an additional value called the don't-care value (denoted by X), which means that the assignment is satisfying regardless of the variable's value, allowing for succinctly describe multiple assignments.

Internally, incremental sat solver - "intel_sat_solver" is being used, please check the repository https://github.com/alexander-nadel/intel_sat_solver for more details.

The solutions returned from HALL can be either disjoint(no overlap) or non-disjoint(may overlap), please check the section [**disjoint and non-disjoint solutions**](#disjoint-and-non-disjoint-solutions) under [**How to use HALL**](#how-to-use-hall) for more info.

HALL contain different algorithms where each one produces disjoint or non-disjoint solutions, please check [**HALL algorithms**](#hall-algorithms) for more details.

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

HALL assume that the circuit output must evaluate to 1, and enumerate all the solutions under this condition to the inputs.

For each assignment, the value of an input variable v can either be 1 (positive), 0 (negative) or x ( *don't-care* ), where the assignment is described by providing a cube (conjunction of literals), where the sign of the variable (v or -v) correspond to the value (1 or 0 respectively) in the current assignment, while don't-care values are not included.

In the AIGER format variables are described with non negative integers (literals), where even numbers represent positive variables and odd numbers represent a negated variable, where the inputs are always positive (even).

HALL represent variables with respect to their *index*, meaning for variable with literal 2 the variable value in the assignment can be either 1, -1 or none, which represent that the variable was assigned X. (recall that HALL only enumerate the circuit's inputs).

For example consider the next AIGER model describing a simple AND gate (see also benchmarks/AND.aag):

```
aag 3 2 0 1 1
2
4
6
6 2 4
```
The single solution where both inputs are equal to 1 is incorporated with the next assignment "1 2" HALL outputs.

The following command reproduce this result by running HALL with the AIGER file "AND.aag", provided under the benchmarks folde:

```
./hall_tool ../benchmarks/AND.aag --print_enumer
```

### disjoint and non-disjoint solutions

An important feature in HALL is if the solutions are disjoint or non-disjoint, where disjoint solutions do not overlap, and non-disjoint solutions may overlap.
HALL contain different algorithms where each one produces disjoint or non-disjoint solutions, for more details, refer to the [**HALL algorithms**](#hall-algorithms) section.

Since a solution with don't-care values incorporate multiple (2^{number of don't cares} to be exact) solutions with only 0/1 values, solutions may overlap. Meaning that a solution may be represented by more than one satisfying assignments HALL produce. 


to clarify assume we want to represent the next following solutions with don't-care values:

```
1 2 3
-1 2 3
1 -2 3
```
An example for disjoint solutions where there are no overlap is (where in the first assignment 1 = X):

```
2 3 
1 -2 3
```

And non-disjoint solutions where the solution "1 -2 3" is represented by both assignments:

```
2 3
1 3
```


## HALL algorithms

HALL contains several different algorithms, where each algorithm can be provided to HALL using "<-mode> <mode_name>" after the input file name, for example:

```
./hall_tool ../benchmarks/AND.aag -mode tale
```

We provide next the list of the algorithms (<mode_name>) where each one produces disjoint (mars-dis) or non-disjoint solutions:

disjoint solutions algorithms:

- mars-dis

non-disjoint solutions algorithms:

- tale
- mars-nondis
- duty
