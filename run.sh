#!/usr/bin/env bash

#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: run.sh
#* About this file: Submission shell script.
#* 
#***********************

# 20191021: Edited by CS Instructional Design staff for compatibility with macOS
# 20191024: Packaging the needed files directly to remove download dependency,
#   adding more error checking for the environment setup

if [ ! -e "Application.cpp" ]; then
  echo -e '\n\nERROR: This script "run.sh" should be located in the same directory where the Application.cpp file is.\n\n'
  exit 1
fi

if [ ! -e "mp1-regen-data" ]; then
  echo -e '\n\nERROR: The "mp1-regen-data" file was not found in this directory. Replace it from the files you were given.\n\n'
  exit 1
fi

if [ ! $(which tar) ]; then
  echo -e '\n\nERROR: Your system needs the "tar" command to be installed first.\n\n'
  exit 1
fi

if [ ! $(which make) ]; then
  echo -e '\n\nERROR: You do not have the "make" tool installed in your shell environment.\n\n'
  exit 1
fi

if [ ! $(which g++) ]; then
  echo -e '\n\nERROR: You do not have the "g++" tool installed in your shell environment. You need to install gcc and/or g++ tools first.\n\n'
  exit 1
fi

rm -rf grade-dir # Make sure grade-dir is clean before starting
rm -f dbg.*.log

mkdir grade-dir
cd grade-dir

cp ../mp1-regen-data mp1-regen-data-tmp.tar
tar -xf mp1-regen-data-tmp.tar

cd mp1
cp ../../MP1Node.* .
make clean > /dev/null
make > /dev/null

if [ ! -e "./Application" ]; then
  echo -e '\n\nERROR: Compilation was not successful.\nSee error messages by typing: make clean && make\n\n'
  exit 1
fi

./Application testcases/singlefailure.conf > /dev/null
cp dbg.log ../../dbg.0.log
./Application testcases/multifailure.conf > /dev/null
cp dbg.log ../../dbg.1.log
./Application testcases/msgdropsinglefailure.conf > /dev/null
cp dbg.log ../../dbg.2.log
cd ../..
rm -rf grade-dir

