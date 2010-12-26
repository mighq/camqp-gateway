#!/bin/bash
LD_LIBRARY_PATH=./libs:${LD_LIBRARY_PATH} ./manager -c sqlite 1
#| tee -a ./logs/instance_1.out.log
