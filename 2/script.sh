#!/bin/bash

pe=0
prevPid=0

while true; do
	sleep 1s

	if [[ "$prevPid" -gt 0 ]]
	then
		(ps -p $pid > /dev/null)
		pe=$?
		
		if [[ "$pe" -eq 0 ]]
		then
			echo pid=$prevPid program still running
			continue
		else
			echo pid=$prevPid program is gone!
		fi
	fi
	
	typeset -i pid=$(cat $1)	
	
	if [[ "$pid" -eq 0 ]]
	then
		echo current pid not set
		continue
	fi

	(ps -p $pid > /dev/null)
	pe=$?

	if [[ "$pe" -eq 0 ]]
	then
		prevPid=$pid
		echo pid=$pid new program really running 
	else
		echo something is bad!
	fi
done
