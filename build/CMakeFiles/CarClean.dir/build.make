# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/sam/WorkingRoom/CarClean

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sam/WorkingRoom/CarClean/build

# Include any dependencies generated for this target.
include CMakeFiles/CarClean.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/CarClean.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/CarClean.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/CarClean.dir/flags.make

CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o: ../src/DirectorLinkClient.cpp
CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o -MF CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o.d -o CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o -c /home/sam/WorkingRoom/CarClean/src/DirectorLinkClient.cpp

CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/DirectorLinkClient.cpp > CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.i

CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/DirectorLinkClient.cpp -o CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.s

CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o: ../src/NetFoundation.cpp
CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o -MF CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o.d -o CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o -c /home/sam/WorkingRoom/CarClean/src/NetFoundation.cpp

CMakeFiles/CarClean.dir/src/NetFoundation.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/NetFoundation.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/NetFoundation.cpp > CMakeFiles/CarClean.dir/src/NetFoundation.cpp.i

CMakeFiles/CarClean.dir/src/NetFoundation.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/NetFoundation.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/NetFoundation.cpp -o CMakeFiles/CarClean.dir/src/NetFoundation.cpp.s

CMakeFiles/CarClean.dir/src/UartMod.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/UartMod.cpp.o: ../src/UartMod.cpp
CMakeFiles/CarClean.dir/src/UartMod.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/CarClean.dir/src/UartMod.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/UartMod.cpp.o -MF CMakeFiles/CarClean.dir/src/UartMod.cpp.o.d -o CMakeFiles/CarClean.dir/src/UartMod.cpp.o -c /home/sam/WorkingRoom/CarClean/src/UartMod.cpp

CMakeFiles/CarClean.dir/src/UartMod.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/UartMod.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/UartMod.cpp > CMakeFiles/CarClean.dir/src/UartMod.cpp.i

CMakeFiles/CarClean.dir/src/UartMod.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/UartMod.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/UartMod.cpp -o CMakeFiles/CarClean.dir/src/UartMod.cpp.s

CMakeFiles/CarClean.dir/src/WashReport.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/WashReport.cpp.o: ../src/WashReport.cpp
CMakeFiles/CarClean.dir/src/WashReport.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/CarClean.dir/src/WashReport.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/WashReport.cpp.o -MF CMakeFiles/CarClean.dir/src/WashReport.cpp.o.d -o CMakeFiles/CarClean.dir/src/WashReport.cpp.o -c /home/sam/WorkingRoom/CarClean/src/WashReport.cpp

CMakeFiles/CarClean.dir/src/WashReport.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/WashReport.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/WashReport.cpp > CMakeFiles/CarClean.dir/src/WashReport.cpp.i

CMakeFiles/CarClean.dir/src/WashReport.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/WashReport.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/WashReport.cpp -o CMakeFiles/CarClean.dir/src/WashReport.cpp.s

CMakeFiles/CarClean.dir/src/main.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/main.cpp.o: ../src/main.cpp
CMakeFiles/CarClean.dir/src/main.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/CarClean.dir/src/main.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/main.cpp.o -MF CMakeFiles/CarClean.dir/src/main.cpp.o.d -o CMakeFiles/CarClean.dir/src/main.cpp.o -c /home/sam/WorkingRoom/CarClean/src/main.cpp

CMakeFiles/CarClean.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/main.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/main.cpp > CMakeFiles/CarClean.dir/src/main.cpp.i

CMakeFiles/CarClean.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/main.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/main.cpp -o CMakeFiles/CarClean.dir/src/main.cpp.s

CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o: ../src/tinyxml2.cpp
CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o -MF CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o.d -o CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o -c /home/sam/WorkingRoom/CarClean/src/tinyxml2.cpp

CMakeFiles/CarClean.dir/src/tinyxml2.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/tinyxml2.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/tinyxml2.cpp > CMakeFiles/CarClean.dir/src/tinyxml2.cpp.i

CMakeFiles/CarClean.dir/src/tinyxml2.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/tinyxml2.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/tinyxml2.cpp -o CMakeFiles/CarClean.dir/src/tinyxml2.cpp.s

CMakeFiles/CarClean.dir/src/uart.cpp.o: CMakeFiles/CarClean.dir/flags.make
CMakeFiles/CarClean.dir/src/uart.cpp.o: ../src/uart.cpp
CMakeFiles/CarClean.dir/src/uart.cpp.o: CMakeFiles/CarClean.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/CarClean.dir/src/uart.cpp.o"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CarClean.dir/src/uart.cpp.o -MF CMakeFiles/CarClean.dir/src/uart.cpp.o.d -o CMakeFiles/CarClean.dir/src/uart.cpp.o -c /home/sam/WorkingRoom/CarClean/src/uart.cpp

CMakeFiles/CarClean.dir/src/uart.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CarClean.dir/src/uart.cpp.i"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sam/WorkingRoom/CarClean/src/uart.cpp > CMakeFiles/CarClean.dir/src/uart.cpp.i

CMakeFiles/CarClean.dir/src/uart.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CarClean.dir/src/uart.cpp.s"
	/opt/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ --sysroot=/opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sam/WorkingRoom/CarClean/src/uart.cpp -o CMakeFiles/CarClean.dir/src/uart.cpp.s

# Object files for target CarClean
CarClean_OBJECTS = \
"CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o" \
"CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o" \
"CMakeFiles/CarClean.dir/src/UartMod.cpp.o" \
"CMakeFiles/CarClean.dir/src/WashReport.cpp.o" \
"CMakeFiles/CarClean.dir/src/main.cpp.o" \
"CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o" \
"CMakeFiles/CarClean.dir/src/uart.cpp.o"

# External object files for target CarClean
CarClean_EXTERNAL_OBJECTS =

CarClean: CMakeFiles/CarClean.dir/src/DirectorLinkClient.cpp.o
CarClean: CMakeFiles/CarClean.dir/src/NetFoundation.cpp.o
CarClean: CMakeFiles/CarClean.dir/src/UartMod.cpp.o
CarClean: CMakeFiles/CarClean.dir/src/WashReport.cpp.o
CarClean: CMakeFiles/CarClean.dir/src/main.cpp.o
CarClean: CMakeFiles/CarClean.dir/src/tinyxml2.cpp.o
CarClean: CMakeFiles/CarClean.dir/src/uart.cpp.o
CarClean: CMakeFiles/CarClean.dir/build.make
CarClean: CMakeFiles/CarClean.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/sam/WorkingRoom/CarClean/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX executable CarClean"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/CarClean.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/CarClean.dir/build: CarClean
.PHONY : CMakeFiles/CarClean.dir/build

CMakeFiles/CarClean.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/CarClean.dir/cmake_clean.cmake
.PHONY : CMakeFiles/CarClean.dir/clean

CMakeFiles/CarClean.dir/depend:
	cd /home/sam/WorkingRoom/CarClean/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sam/WorkingRoom/CarClean /home/sam/WorkingRoom/CarClean /home/sam/WorkingRoom/CarClean/build /home/sam/WorkingRoom/CarClean/build /home/sam/WorkingRoom/CarClean/build/CMakeFiles/CarClean.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/CarClean.dir/depend

