#!/bin/bash
make clean && make && cd test && make check; cd ..
