#!/bin/bash
# Create histogram and statistics from data
# Should be called from PROJECT_ROOT/build directory
# Calls matlab executable in PROJECT_ROOT/logs/analysis and passes
# the number of histogram bins
cd ../logs/analysis_bin
./run_hist.sh /usr/local/MATLAB/R2014a ${1:- 50} 
cd ../../build

