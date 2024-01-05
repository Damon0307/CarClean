#!/bin/sh

mkdir build && cd build

cmake .. && make -j
cd .. && mv build/hello .

rm build -rf
