# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake3

# The command to remove a file.
RM = /usr/bin/cmake3 -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/carlos/Lua/src/lcdf

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carlos/Lua/src/lcdf/build.luarocks

# Include any dependencies generated for this target.
include CMakeFiles/lcdf.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/lcdf.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lcdf.dir/flags.make

CMakeFiles/lcdf.dir/dcdflib.c.o: CMakeFiles/lcdf.dir/flags.make
CMakeFiles/lcdf.dir/dcdflib.c.o: ../dcdflib.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/carlos/Lua/src/lcdf/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/lcdf.dir/dcdflib.c.o"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/lcdf.dir/dcdflib.c.o   -c /home/carlos/Lua/src/lcdf/dcdflib.c

CMakeFiles/lcdf.dir/dcdflib.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/lcdf.dir/dcdflib.c.i"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/carlos/Lua/src/lcdf/dcdflib.c > CMakeFiles/lcdf.dir/dcdflib.c.i

CMakeFiles/lcdf.dir/dcdflib.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/lcdf.dir/dcdflib.c.s"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/carlos/Lua/src/lcdf/dcdflib.c -o CMakeFiles/lcdf.dir/dcdflib.c.s

CMakeFiles/lcdf.dir/dcdflib.c.o.requires:

.PHONY : CMakeFiles/lcdf.dir/dcdflib.c.o.requires

CMakeFiles/lcdf.dir/dcdflib.c.o.provides: CMakeFiles/lcdf.dir/dcdflib.c.o.requires
	$(MAKE) -f CMakeFiles/lcdf.dir/build.make CMakeFiles/lcdf.dir/dcdflib.c.o.provides.build
.PHONY : CMakeFiles/lcdf.dir/dcdflib.c.o.provides

CMakeFiles/lcdf.dir/dcdflib.c.o.provides.build: CMakeFiles/lcdf.dir/dcdflib.c.o


CMakeFiles/lcdf.dir/ipmpar.c.o: CMakeFiles/lcdf.dir/flags.make
CMakeFiles/lcdf.dir/ipmpar.c.o: ../ipmpar.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/carlos/Lua/src/lcdf/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/lcdf.dir/ipmpar.c.o"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/lcdf.dir/ipmpar.c.o   -c /home/carlos/Lua/src/lcdf/ipmpar.c

CMakeFiles/lcdf.dir/ipmpar.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/lcdf.dir/ipmpar.c.i"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/carlos/Lua/src/lcdf/ipmpar.c > CMakeFiles/lcdf.dir/ipmpar.c.i

CMakeFiles/lcdf.dir/ipmpar.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/lcdf.dir/ipmpar.c.s"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/carlos/Lua/src/lcdf/ipmpar.c -o CMakeFiles/lcdf.dir/ipmpar.c.s

CMakeFiles/lcdf.dir/ipmpar.c.o.requires:

.PHONY : CMakeFiles/lcdf.dir/ipmpar.c.o.requires

CMakeFiles/lcdf.dir/ipmpar.c.o.provides: CMakeFiles/lcdf.dir/ipmpar.c.o.requires
	$(MAKE) -f CMakeFiles/lcdf.dir/build.make CMakeFiles/lcdf.dir/ipmpar.c.o.provides.build
.PHONY : CMakeFiles/lcdf.dir/ipmpar.c.o.provides

CMakeFiles/lcdf.dir/ipmpar.c.o.provides.build: CMakeFiles/lcdf.dir/ipmpar.c.o


CMakeFiles/lcdf.dir/lcdf.c.o: CMakeFiles/lcdf.dir/flags.make
CMakeFiles/lcdf.dir/lcdf.c.o: ../lcdf.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/carlos/Lua/src/lcdf/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/lcdf.dir/lcdf.c.o"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/lcdf.dir/lcdf.c.o   -c /home/carlos/Lua/src/lcdf/lcdf.c

CMakeFiles/lcdf.dir/lcdf.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/lcdf.dir/lcdf.c.i"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/carlos/Lua/src/lcdf/lcdf.c > CMakeFiles/lcdf.dir/lcdf.c.i

CMakeFiles/lcdf.dir/lcdf.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/lcdf.dir/lcdf.c.s"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/carlos/Lua/src/lcdf/lcdf.c -o CMakeFiles/lcdf.dir/lcdf.c.s

CMakeFiles/lcdf.dir/lcdf.c.o.requires:

.PHONY : CMakeFiles/lcdf.dir/lcdf.c.o.requires

CMakeFiles/lcdf.dir/lcdf.c.o.provides: CMakeFiles/lcdf.dir/lcdf.c.o.requires
	$(MAKE) -f CMakeFiles/lcdf.dir/build.make CMakeFiles/lcdf.dir/lcdf.c.o.provides.build
.PHONY : CMakeFiles/lcdf.dir/lcdf.c.o.provides

CMakeFiles/lcdf.dir/lcdf.c.o.provides.build: CMakeFiles/lcdf.dir/lcdf.c.o


# Object files for target lcdf
lcdf_OBJECTS = \
"CMakeFiles/lcdf.dir/dcdflib.c.o" \
"CMakeFiles/lcdf.dir/ipmpar.c.o" \
"CMakeFiles/lcdf.dir/lcdf.c.o"

# External object files for target lcdf
lcdf_EXTERNAL_OBJECTS =

lcdf.so: CMakeFiles/lcdf.dir/dcdflib.c.o
lcdf.so: CMakeFiles/lcdf.dir/ipmpar.c.o
lcdf.so: CMakeFiles/lcdf.dir/lcdf.c.o
lcdf.so: CMakeFiles/lcdf.dir/build.make
lcdf.so: /home/carlos/Lua/lib/liblua.a
lcdf.so: CMakeFiles/lcdf.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/carlos/Lua/src/lcdf/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C shared library lcdf.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lcdf.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/lcdf.dir/build: lcdf.so

.PHONY : CMakeFiles/lcdf.dir/build

CMakeFiles/lcdf.dir/requires: CMakeFiles/lcdf.dir/dcdflib.c.o.requires
CMakeFiles/lcdf.dir/requires: CMakeFiles/lcdf.dir/ipmpar.c.o.requires
CMakeFiles/lcdf.dir/requires: CMakeFiles/lcdf.dir/lcdf.c.o.requires

.PHONY : CMakeFiles/lcdf.dir/requires

CMakeFiles/lcdf.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lcdf.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lcdf.dir/clean

CMakeFiles/lcdf.dir/depend:
	cd /home/carlos/Lua/src/lcdf/build.luarocks && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carlos/Lua/src/lcdf /home/carlos/Lua/src/lcdf /home/carlos/Lua/src/lcdf/build.luarocks /home/carlos/Lua/src/lcdf/build.luarocks /home/carlos/Lua/src/lcdf/build.luarocks/CMakeFiles/lcdf.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/lcdf.dir/depend
