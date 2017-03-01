#!/bin/sh

projectName=ShortestPath

cd /tmp
filename=`date +%N`
echo "/tmp/${filename}"

mkdir ${filename}

cd ${filename}

git clone ~/source/${projectName} 
rm -rf ${projectName}/.git

cmd="scp -r ${projectName} 162.105.30.250:/home/public/shuxiong/tmp"
echo $cmd
eval $cmd
cmd="ssh -t 162.105.30.250 \"scp -r /home/public/shuxiong/tmp/${projectName} master:~\""
echo $cmd
eval $cmd

rm -rf /tmp/${filename}

