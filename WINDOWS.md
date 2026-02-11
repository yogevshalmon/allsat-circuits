# Windows Integration Guide

## Easiest Method: Download Pre-built Libraries

### For End Users (Windows):

1. **Download pre-built package:**
   - Go to: https://github.com/yogevshalmon/allsat-circuits/actions
   - Click on the latest successful workflow run for the `lib_interface` branch
   - Download the `hall-windows-x64` artifact
   - Extract the archive

2. **Integrate into your Visual Studio project:**
   
   In Project Properties:
   
   - **C/C++ → General → Additional Include Directories:**
     ```
     C:\path\to\extracted\package\include
     C:\path\to\extracted\package\src
     ```
   
   - **Linker → Input → Additional Dependencies:**
     ```
     C:\path\to\extracted\package\liballsat.a
     C:\path\to\extracted\package\libcadical.a
     C:\path\to\extracted\package\libintel_sat_solver.a
     ```
   
   - **C/C++ → Language → C++ Language Standard:** ISO C++20

3. **Use the API:**
   
   See `simple_test.cpp` in the package for a complete example.

### Important Notes:

- Libraries are built with MinGW GCC, so you'll need to use MinGW GCC for your project too
- For pure MSVC projects, use WSL or MSYS2 to build from source (see below)

## Alternative: Build from Source (Advanced)

If you need MSVC compatibility or want to build yourself, see the main README.md for WSL or MSYS2 build instructions.
