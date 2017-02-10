#!/bin/bash

# GTest cmdline arguments
# Use negative filters to filter out the tests we do NOT want to run.
CMDLINE_ARGUMENTS="--gtest_filter=-imx135*:imx230*:ov8858*"

# Check for the test binary existence
BIN_FILE=camera_subsys_test
TEST_EXECUTABLE=`which $BIN_FILE`

if [[ -z $TEST_EXECUTABLE  ]];
then
   echo "Executable '$BIN_FILE' not found in PATH. Not installed?"
   exit 1
fi

if [[ ! -x $TEST_EXECUTABLE ]];
then
   echo "Executable '$TEST_EXECUTABLE' does not exist, or not executable. Forgot to make / sudo make install?"
   exit 1
fi

# Run the actual test. Using BIN_FILE to
# avoid having absolute path showing in valgrind_test.cpp, and breaking its suppression file usage.
$BIN_FILE $CMDLINE_ARGUMENTS
