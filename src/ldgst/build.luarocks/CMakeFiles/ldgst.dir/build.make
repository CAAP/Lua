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
CMAKE_SOURCE_DIR = /home/carlos/Lua/src/ldgst

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carlos/Lua/src/ldgst/build.luarocks

# Include any dependencies generated for this target.
include CMakeFiles/ldgst.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ldgst.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ldgst.dir/flags.make

CMakeFiles/ldgst.dir/ldgst.c.o: CMakeFiles/ldgst.dir/flags.make
CMakeFiles/ldgst.dir/ldgst.c.o: ../ldgst.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/carlos/Lua/src/ldgst/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/ldgst.dir/ldgst.c.o"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/ldgst.dir/ldgst.c.o   -c /home/carlos/Lua/src/ldgst/ldgst.c

CMakeFiles/ldgst.dir/ldgst.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ldgst.dir/ldgst.c.i"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/carlos/Lua/src/ldgst/ldgst.c > CMakeFiles/ldgst.dir/ldgst.c.i

CMakeFiles/ldgst.dir/ldgst.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ldgst.dir/ldgst.c.s"
	/usr/bin/clang  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/carlos/Lua/src/ldgst/ldgst.c -o CMakeFiles/ldgst.dir/ldgst.c.s

CMakeFiles/ldgst.dir/ldgst.c.o.requires:

.PHONY : CMakeFiles/ldgst.dir/ldgst.c.o.requires

CMakeFiles/ldgst.dir/ldgst.c.o.provides: CMakeFiles/ldgst.dir/ldgst.c.o.requires
	$(MAKE) -f CMakeFiles/ldgst.dir/build.make CMakeFiles/ldgst.dir/ldgst.c.o.provides.build
.PHONY : CMakeFiles/ldgst.dir/ldgst.c.o.provides

CMakeFiles/ldgst.dir/ldgst.c.o.provides.build: CMakeFiles/ldgst.dir/ldgst.c.o


# Object files for target ldgst
ldgst_OBJECTS = \
"CMakeFiles/ldgst.dir/ldgst.c.o"

# External object files for target ldgst
ldgst_EXTERNAL_OBJECTS =

ldgst.so: CMakeFiles/ldgst.dir/ldgst.c.o
ldgst.so: CMakeFiles/ldgst.dir/build.make
ldgst.so: /home/carlos/Lua/lib/liblua.a
ldgst.so: /usr/lib64/libcrypto.so
ldgst.so: CMakeFiles/ldgst.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/carlos/Lua/src/ldgst/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C shared library ldgst.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ldgst.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ldgst.dir/build: ldgst.so

.PHONY : CMakeFiles/ldgst.dir/build

CMakeFiles/ldgst.dir/requires: CMakeFiles/ldgst.dir/ldgst.c.o.requires

.PHONY : CMakeFiles/ldgst.dir/requires

CMakeFiles/ldgst.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ldgst.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ldgst.dir/clean

CMakeFiles/ldgst.dir/depend:
	cd /home/carlos/Lua/src/ldgst/build.luarocks && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carlos/Lua/src/ldgst /home/carlos/Lua/src/ldgst /home/carlos/Lua/src/ldgst/build.luarocks /home/carlos/Lua/src/ldgst/build.luarocks /home/carlos/Lua/src/ldgst/build.luarocks/CMakeFiles/ldgst.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ldgst.dir/depend

