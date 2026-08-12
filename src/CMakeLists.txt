# ---------------------------------------------------------------------------
# PULSE — src/ build configuration
# ---------------------------------------------------------------------------
# This file defines actual build targets (executables/libraries). The root
# CMakeLists.txt only sets project-wide settings and delegates here.

# Defines an executable target named "pulse", built from main.cpp.
# As storage/ and query/ subdirectories are added later, this will change
# from a single source file to linking against a "storage" library and a
# "query" library instead of compiling everything directly here.
add_executable(pulse
    main.cpp
)