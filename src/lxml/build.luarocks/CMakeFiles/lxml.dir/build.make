# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.9

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
CMAKE_SOURCE_DIR = /home/carlos/Lua/src/lxml

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carlos/Lua/src/lxml/build.luarocks

# Include any dependencies generated for this target.
include CMakeFiles/lxml.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/lxml.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lxml.dir/flags.make

CMakeFiles/lxml.dir/lxml.o: CMakeFiles/lxml.dir/flags.make
CMakeFiles/lxml.dir/lxml.o: ../lxml.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/carlos/Lua/src/lxml/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/lxml.dir/lxml.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/lxml.dir/lxml.o -c /home/carlos/Lua/src/lxml/lxml.cpp

CMakeFiles/lxml.dir/lxml.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lxml.dir/lxml.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/carlos/Lua/src/lxml/lxml.cpp > CMakeFiles/lxml.dir/lxml.i

CMakeFiles/lxml.dir/lxml.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lxml.dir/lxml.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/carlos/Lua/src/lxml/lxml.cpp -o CMakeFiles/lxml.dir/lxml.s

CMakeFiles/lxml.dir/lxml.o.requires:

.PHONY : CMakeFiles/lxml.dir/lxml.o.requires

CMakeFiles/lxml.dir/lxml.o.provides: CMakeFiles/lxml.dir/lxml.o.requires
	$(MAKE) -f CMakeFiles/lxml.dir/build.make CMakeFiles/lxml.dir/lxml.o.provides.build
.PHONY : CMakeFiles/lxml.dir/lxml.o.provides

CMakeFiles/lxml.dir/lxml.o.provides.build: CMakeFiles/lxml.dir/lxml.o


CMakeFiles/lxml.dir/tinyxml2.o: CMakeFiles/lxml.dir/flags.make
CMakeFiles/lxml.dir/tinyxml2.o: ../tinyxml2.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/carlos/Lua/src/lxml/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/lxml.dir/tinyxml2.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/lxml.dir/tinyxml2.o -c /home/carlos/Lua/src/lxml/tinyxml2.cpp

CMakeFiles/lxml.dir/tinyxml2.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/lxml.dir/tinyxml2.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/carlos/Lua/src/lxml/tinyxml2.cpp > CMakeFiles/lxml.dir/tinyxml2.i

CMakeFiles/lxml.dir/tinyxml2.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/lxml.dir/tinyxml2.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/carlos/Lua/src/lxml/tinyxml2.cpp -o CMakeFiles/lxml.dir/tinyxml2.s

CMakeFiles/lxml.dir/tinyxml2.o.requires:

.PHONY : CMakeFiles/lxml.dir/tinyxml2.o.requires

CMakeFiles/lxml.dir/tinyxml2.o.provides: CMakeFiles/lxml.dir/tinyxml2.o.requires
	$(MAKE) -f CMakeFiles/lxml.dir/build.make CMakeFiles/lxml.dir/tinyxml2.o.provides.build
.PHONY : CMakeFiles/lxml.dir/tinyxml2.o.provides

CMakeFiles/lxml.dir/tinyxml2.o.provides.build: CMakeFiles/lxml.dir/tinyxml2.o


# Object files for target lxml
lxml_OBJECTS = \
"CMakeFiles/lxml.dir/lxml.o" \
"CMakeFiles/lxml.dir/tinyxml2.o"

# External object files for target lxml
lxml_EXTERNAL_OBJECTS =

lxml.so: CMakeFiles/lxml.dir/lxml.o
lxml.so: CMakeFiles/lxml.dir/tinyxml2.o
lxml.so: CMakeFiles/lxml.dir/build.make
lxml.so: /usr/local/lib/liblua53.so
lxml.so: CMakeFiles/lxml.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/carlos/Lua/src/lxml/build.luarocks/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX shared library lxml.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lxml.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/lxml.dir/build: lxml.so

.PHONY : CMakeFiles/lxml.dir/build

CMakeFiles/lxml.dir/requires: CMakeFiles/lxml.dir/lxml.o.requires
CMakeFiles/lxml.dir/requires: CMakeFiles/lxml.dir/tinyxml2.o.requires

.PHONY : CMakeFiles/lxml.dir/requires

CMakeFiles/lxml.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lxml.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lxml.dir/clean

CMakeFiles/lxml.dir/depend:
	cd /home/carlos/Lua/src/lxml/build.luarocks && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carlos/Lua/src/lxml /home/carlos/Lua/src/lxml /home/carlos/Lua/src/lxml/build.luarocks /home/carlos/Lua/src/lxml/build.luarocks /home/carlos/Lua/src/lxml/build.luarocks/CMakeFiles/lxml.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/lxml.dir/depend
