# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/sg/Lua/src/lzmq

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sg/Lua/src/lzmq/build.luarocks

# Include any dependencies generated for this target.
include CMakeFiles/lzmq.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/lzmq.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lzmq.dir/flags.make

CMakeFiles/lzmq.dir/lzmq.c.o: CMakeFiles/lzmq.dir/flags.make
CMakeFiles/lzmq.dir/lzmq.c.o: ../lzmq.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sg/Lua/src/lzmq/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/lzmq.dir/lzmq.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/lzmq.dir/lzmq.c.o   -c /home/sg/Lua/src/lzmq/lzmq.c

CMakeFiles/lzmq.dir/lzmq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/lzmq.dir/lzmq.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/sg/Lua/src/lzmq/lzmq.c > CMakeFiles/lzmq.dir/lzmq.c.i

CMakeFiles/lzmq.dir/lzmq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/lzmq.dir/lzmq.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/sg/Lua/src/lzmq/lzmq.c -o CMakeFiles/lzmq.dir/lzmq.c.s

CMakeFiles/lzmq.dir/lzmq.c.o.requires:

.PHONY : CMakeFiles/lzmq.dir/lzmq.c.o.requires

CMakeFiles/lzmq.dir/lzmq.c.o.provides: CMakeFiles/lzmq.dir/lzmq.c.o.requires
	$(MAKE) -f CMakeFiles/lzmq.dir/build.make CMakeFiles/lzmq.dir/lzmq.c.o.provides.build
.PHONY : CMakeFiles/lzmq.dir/lzmq.c.o.provides

CMakeFiles/lzmq.dir/lzmq.c.o.provides.build: CMakeFiles/lzmq.dir/lzmq.c.o


# Object files for target lzmq
lzmq_OBJECTS = \
"CMakeFiles/lzmq.dir/lzmq.c.o"

# External object files for target lzmq
lzmq_EXTERNAL_OBJECTS =

lzmq.so: CMakeFiles/lzmq.dir/lzmq.c.o
lzmq.so: CMakeFiles/lzmq.dir/build.make
lzmq.so: /usr/local/lib/libzmq.so.4.2
lzmq.so: CMakeFiles/lzmq.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/sg/Lua/src/lzmq/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C shared library lzmq.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lzmq.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/lzmq.dir/build: lzmq.so

.PHONY : CMakeFiles/lzmq.dir/build

CMakeFiles/lzmq.dir/requires: CMakeFiles/lzmq.dir/lzmq.c.o.requires

.PHONY : CMakeFiles/lzmq.dir/requires

CMakeFiles/lzmq.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lzmq.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lzmq.dir/clean

CMakeFiles/lzmq.dir/depend:
	cd /home/sg/Lua/src/lzmq/build.luarocks && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sg/Lua/src/lzmq /home/sg/Lua/src/lzmq /home/sg/Lua/src/lzmq/build.luarocks /home/sg/Lua/src/lzmq/build.luarocks /home/sg/Lua/src/lzmq/build.luarocks/CMakeFiles/lzmq.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/lzmq.dir/depend

