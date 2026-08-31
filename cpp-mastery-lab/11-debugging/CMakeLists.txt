add_executable(11-debugging-buggy buggy.cpp)
add_executable(11-debugging-fixed fixed.cpp)

# Only the fixed version is part of the automated suite — the buggy version
# is meant to be explored manually with gdb/valgrind/ASan (see README.md),
# and several of its modes deliberately crash or leak.
add_test(NAME 11-debugging-fixed COMMAND 11-debugging-fixed)
