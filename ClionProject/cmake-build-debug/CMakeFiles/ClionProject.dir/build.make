# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

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
CMAKE_COMMAND = /cygdrive/c/Users/shira/.CLion2019.3/system/cygwin_cmake/bin/cmake.exe

# The command to remove a file.
RM = /cygdrive/c/Users/shira/.CLion2019.3/system/cygwin_cmake/bin/cmake.exe -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug"

# Include any dependencies generated for this target.
include CMakeFiles/ClionProject.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ClionProject.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ClionProject.dir/flags.make

CMakeFiles/ClionProject.dir/main.cpp.o: CMakeFiles/ClionProject.dir/flags.make
CMakeFiles/ClionProject.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ClionProject.dir/main.cpp.o"
	/usr/bin/c++.exe  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ClionProject.dir/main.cpp.o -c "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/main.cpp"

CMakeFiles/ClionProject.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ClionProject.dir/main.cpp.i"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/main.cpp" > CMakeFiles/ClionProject.dir/main.cpp.i

CMakeFiles/ClionProject.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ClionProject.dir/main.cpp.s"
	/usr/bin/c++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/main.cpp" -o CMakeFiles/ClionProject.dir/main.cpp.s

# Object files for target ClionProject
ClionProject_OBJECTS = \
"CMakeFiles/ClionProject.dir/main.cpp.o"

# External object files for target ClionProject
ClionProject_EXTERNAL_OBJECTS =

ClionProject.exe: CMakeFiles/ClionProject.dir/main.cpp.o
ClionProject.exe: CMakeFiles/ClionProject.dir/build.make
ClionProject.exe: CMakeFiles/ClionProject.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ClionProject.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ClionProject.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ClionProject.dir/build: ClionProject.exe

.PHONY : CMakeFiles/ClionProject.dir/build

CMakeFiles/ClionProject.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ClionProject.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ClionProject.dir/clean

CMakeFiles/ClionProject.dir/depend:
	cd "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject" "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject" "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug" "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug" "/cygdrive/c/DATA/Operating Systems 234123/HW1/ClionProject/cmake-build-debug/CMakeFiles/ClionProject.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/ClionProject.dir/depend

