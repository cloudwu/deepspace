#!/bin/bash

ANT=/c/project/ant
ANTBIN=$ANT/bin/mingw/release/ant.exe
#STARTUP=`realpath main.lua`
STARTUP=/c/project/demo/main.lua

cd $ANT
$ANTBIN $STARTUP
