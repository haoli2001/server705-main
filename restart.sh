#!/bin/bash

#功能： 循环监测mpirun进程，如果找不到该进程，说明进程挂掉，重启之
#输入参数： $1:可执行文件名
while true
do
	process=`ps -ef| grep mpirun | grep -v grep | wc -l`
	if [ "$process" -eq 0 ];then
			echo "process down,now start"
			mpirun -np 2 servers ./$1 &
	else
		sleep 5;
	fi
done
