#!/bin/sh

echo "80" > /proc/sys/vm/overcommit_ratio

echo "1" >  /proc/sys/vm/overcommit_memory

echo "1" >  /proc/sys/vm/drop_caches

count=1

while [ $count -le 10 ] ; do

    cat /proc/shrinkmem &

    echo "cat /proc/shrinkmem ... $count"

    let "count=$count+1"

done

 
