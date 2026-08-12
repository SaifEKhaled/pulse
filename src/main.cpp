// src/main.cpp
//
// Placeholder entry point for PULSE.
//
// This file exists right now purely to prove the build system works
// end-to-end (CMake configure -> compile -> link -> run). It has no real
// logic yet. As the storage and query engines are built out, this will
// evolve into the actual entry point that wires together:
//   - the storage engine (WAL, on-disk segments)
//   - the query engine (parser, planner, executor)
//   - a CLI or API surface for issuing queries



#include <iostream>

int main() {
    std::cout << "PULSE starting up..." << std::endl;
    return 0;
}