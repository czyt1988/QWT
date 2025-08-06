#!/bin/bash

DEST=../src
OPTS='-i "../src" -w "*.cpp;*.h;*.hpp" -s'
./Amalgamate.exe $OPTS ./amalgamate/QWTAmalgamTemplate.h $DEST/../src-amalgamate/QwtPlot.h
./Amalgamate.exe $OPTS ./amalgamate/QWTAmalgamTemplate.cpp $DEST/../src-amalgamate/QwtPlot.cpp
#  使用read命令达到类似bat中的pause命令效果
echo 按任意键继续
read -n 1
echo 继续运行