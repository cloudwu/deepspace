#!/bin/bash

ANT=/c/project/ant
ANTBIN=$ANT/bin/mingw/release/ant.exe
#STARTUP=`realpath main.lua`
STARTUP=/c/project/demo/ui.lua

cd $ANT
$ANTBIN $STARTUP
