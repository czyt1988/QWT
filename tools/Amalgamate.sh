#!/bin/bash

DEST=../src
OPTS='-i "../src" -w "*.cpp;*.h;*.hpp" -s'
./Amalgamate.exe $OPTS ./amalgamate/QWTAmalgamTemplate.h $DEST/../src-amalgamate/QwtPlot.h
./Amalgamate.exe $OPTS ./amalgamate/QWTAmalgamTemplate.cpp $DEST/../src-amalgamate/QwtPlot.cpp
#  ʹ��read����ﵽ����bat�е�pause����Ч��
echo �����������
read -n 1
echo ��������