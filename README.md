# HALL (Haifa AllSAT)

HALL is an AllSAT enumeration tool for Combinational Circuits(in AIGER format)

## Summary

This tool **HALL** given an Combinational Circuit in AIGER format, generates an AllSAT enumeration: all the solutions (satisfying assignments) in Disjunctive Normal Form (DNF). For more information about the AIGER format please visit the page: http://fmv.jku.at/aiger/

The solutions are represented using **satisfying input assignment**, meaning only the circuit inputs are enumerated.
Further more, this tool utilizing **partial assignments**, which are assignment for only part of the variables (where the rest assign the don't-care value), allowing for succinctly describe multiple assignments.

Internally, incremntal sat solver - "intel_sat_solver" is been used, please check the repository https://github.com/alexander-nadel/intel_sat_solver for more details.

Please check the **How to use the tool** section for more info.

## How to build

Please consider the following before continuing: 
- Compilation requires g++ version 10.1.0 or higher.
- We use CMake for building the tool, please verify that you have CMake VERSION 3.8 or higher.

To build the tool, just clone the repository and run this commands(after entering the repository directory)

1.	```cmake -S . -B build```
2.  ```make```

This will create new folder named "**build**" and will compile the tool in release mode.

this should generate the tool **hall_tool**.

After building the tool in the "build" directory, you should be able to run the tool, for example:

```
./hall_tool ../benchmarks/...
```

## How to use the tool

The tool gets as an input AIGER file, and outputs all the satisfying partial input assignment in DNF.

Each assignment the value of each variable v can either be v (positive), -v (negative) or x (*don't-care*).
