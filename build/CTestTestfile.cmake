# CMake generated Testfile for 
# Source directory: /home/shahzaibsidd/projects/gbEmulator
# Build directory: /home/shahzaibsidd/projects/gbEmulator/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(check_gbe "/home/shahzaibsidd/projects/gbEmulator/build/tests/check_gbe")
set_tests_properties(check_gbe PROPERTIES  _BACKTRACE_TRIPLES "/home/shahzaibsidd/projects/gbEmulator/CMakeLists.txt;96;add_test;/home/shahzaibsidd/projects/gbEmulator/CMakeLists.txt;0;")
subdirs("lib")
subdirs("gbemu")
subdirs("tests")
