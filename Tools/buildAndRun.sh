#!/bin/sh

projectName=ShortestPath

curPath=`dirname $0`
./${curPath}/checkSSHKey.sh
cd ${curPath}/../
cmd="make Mic && ssh -t mic0 \"mkdir ~/ShortestPath\" ; scp main mic0:~/ShortestPath && ssh -t mic0 \"cd ~/ShortestPath; LD_LIBRARY_PATH=~ KMP_AFFINITY=scatter ./main\""
echo $cmd
eval $cmd
