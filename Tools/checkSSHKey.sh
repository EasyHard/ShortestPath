#!/bin/bash -x

s=$(cat ~/.ssh/id_rsa.pub)
ssh -t mic0 "echo '$s' >> ~/.ssh/authorized_keys"

m=$(ssh -t mic0 "grep -c manycore@master ~/.ssh/authorized_keys"|tr -d '\n\r')
if [ $m -gt 1 ]; then
    ssh -t mic0 "echo \"\$d\\nw\\nq\\ny\\n\" | ed ~/.ssh/authorized_keys"
fi
