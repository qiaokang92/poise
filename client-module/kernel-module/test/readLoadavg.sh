#!/system/bin/sh
echo $0
echo $1
echo "" > $1
while :
do
	t=`date`
	l=`cat /proc/loadavg`
	echo $t $l >> $1
	echo $t $l
	sleep 1
done
