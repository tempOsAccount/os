#!/bin/bash

prevPid=0

while true; do
	sleep 1s	
	typeset -i pid=$(cat $1)	
	
	if [[ "$pid" -eq 0 ]]
	then
		echo current pid not set
		continue
	fi

	if [[ "$pid" == "$prevPid" ]]
	then
		echo pid=$prevPid program still running
		continue
	fi

	(ps -p $pid > /dev/null)
	pe="$?"

	if [[ "$pe" -eq 0 ]]
	then
		prevPid=$pid
		echo pid=$pid new program really running 
	else
		echo something is bad!
	fi
done
