#!/bin/bash

#功能：将可执行文件和用户自定义的动态库复制到三个节点
#输入参数： $1:可执行文件名


list="node1 node2 node3"
#usrlib_dir="/home/xianyun/705/KunPeng/2023.11.7"
#usrlib_dir="/home/hao/Documents/KunPeng/2023.11.7/"
usrlib_dir="/home/xianyun/XD_YLSim/code/usr_lib"
scrip_dir=$(cd $(dirname $0);pwd)
dir=$(dirname $scrip_dir)
#echo "use_dir_top = $(dirname $usrlib_dir); usr_dir = $usrlib_dir."
echo "scrip_dir = $scrip_dir"

for i in $list
do
	#scp -r $scrip_dir $i:$dir;
    scp OUT $i:$scrip_dir;
	scp -r $usrlib_dir $i:$(dirname $usrlib_dir);
done
echo "copy finished"

#./restart.sh OUT
mpirun -np 352 -f servers./OUT