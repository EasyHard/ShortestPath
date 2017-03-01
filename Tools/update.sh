#!/bin/bash
cp $1 $(echo "$1" | sed 's/\.[^.]*$//')
ssh -v -t -R33873:127.0.0.1:873 shuxiong@mcore "./updateSource.sh"
