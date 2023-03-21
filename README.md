# HALL (Haifa AllSAT)

HALL is an AllSAT enumeration tool for single output combinational circuits(in AIGER format).

## Summary

This tool **HALL** given a combinational circuit in AIGER format with a single output (which evalutes to 1), generates an AllSAT enumeration: all the solutions (satisfying assignments) in Disjunctive Normal Form (DNF). For more information about the AIGER format please visit the page: http://fmv.jku.at/aiger/

The solutions are represented using assignments to the inputs only, meaning only the circuit inputs are enumerated.
Further more, this tool utilizing **ternary** values, which extends the Boolean values 0/1 with an additional value called the don't-care value (denoted by X), which means that the assignemnt is satisfying regard of the variable value, allowing for succinctly describe multiple assignments.

Internally, incremental sat solver - "intel_sat_solver" is been used, please check the repository https://github.com/alexander-nadel/intel_sat_solver for more details.

The solutions returned from HALL can be either disjoint(no overlap) or non-disjoint(may overlap), please check the [**How to use the tool**](#how-to-use-the-tool) section for more info.

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

## How to use the tool

HALL recive as an input an AIGER file (ascii or binary), The AIGER file should describe a Combinational Circuit containing only **one** output.

HALL assume that the circuit output must evalute to 1, and enumerate all the solution to the inputs were the output still evalute to 1.

For each assignment, the value of an input variable v can either be 1 (positive), 0 (negative) or x (*don't-care*), where we describe the assignment by providing a cube (conjunction of literals), where the sign of the variable (v or -v) correspond to the value (1 or 0 respectively) in the current assignment while don't-care values are not included.

In the AIGER format variables are described with non negative integers (literals), where even numbers represent positive variables and odd numbers represent a negated variable, where the inputs are always positive (even).

HALL represent variables with respect to their *index*, meaning for variable with literal 2 the variable value in the assignment can be either 1, -1 or none, which represent that the variable was assigned X. (recall that HALL only enumerate the circuit's inputs)

For example consider the next AIGER model describing a simple AND gate:

```
aag 3 2 0 1 1
2
4
6
6 2 4
```
The single solution where both inputs are equal to 1 is incorparated with the next assignment HALL outputs:

```
1 2
```

### disjoint and non-disjoint solutions

An important feature in HALL is if the solutions are disjoint or non-disjoint,
disjoint solutions do not overlap, meaning that any full assignment is represent by a single partial assignment, In contrast in non-disjoint solutions full assignments may be represent by more than a single partial assignment.
to clarfiy assume we have the next following solutions:

```
1 2 3
-1 2 3
1 -2 3
```
An example for disjoint solutions where there are no overlap is:

```
2 3 
1 -2 3
```

And non-disjoint solutions where the solution "1 -2 3" is represent by both partial assignments:

```
2 3
1 3
```

HALL contain different algorithms where each one produces disjoint or non-disjoint solutions, please check the next section for more details.
