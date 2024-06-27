#!/bin/bash

ANT=/c/project/ant
ANTBIN=$ANT/bin/mingw/release/ant.exe
#STARTUP=`realpath main.lua`
STARTUP=/c/project/demo/main.lua

cd $ANT
$ANTBIN $ANT/tools/filepack/main.lua /c/project/demo -v
cp $ANT/bin/mingw/release/internal/00.zip /c/project/demo/bin/internal
cp $ANT/bin/mingw/release/internal/00.hash /c/project/demo/bin/internal
