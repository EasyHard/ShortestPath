#!/bin/sh

pn=ShortestPath
pnp="~/${pn}"
dsp="${pnp}/Dataset"

lib=/opt/intel/composer_xe_2013_sp1.2.144/compiler/lib/mic/libiomp5.so

curPath=`dirname $0`
./${curPath}/checkSSHKey.sh
cd ${curPath}/../
cmd="ssh -t mic0 \"mkdir -p ${dsp}; rm -rf ${dsp};\" && scp -r Dataset mic0:${pnp} && scp ${lib} mic0:~"
echo $cmd
eval $cmd
