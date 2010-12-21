#!/bin/bash
LD_LIBRARY_PATH=./libs:${LD_LIBRARY_PATH} ./manager -c sqlite 1
