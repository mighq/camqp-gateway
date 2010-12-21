#!/bin/bash
for i in $(ps a | sed -ne '/manager/p' | sed -e 's#^\s*\(\S\+\).*#\1#'); do kill -9 $i; done
