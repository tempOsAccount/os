#!/bin/bash

TYPE="";
FIELD=();
error=false;
gameEnd=false;
winner="";
winnerSym="";

function printHelp {
	echo "User -c (for client) or -s (for server)";
	echo "Server should be started first.";
	echo "Write two numbers (row column) to make turn. Example: 2 1";
}

function initField {
	for i in `seq 0 8`
	do
		FIELD[$i]=" ";
	done
}

function printField {
	tput cup 3 0;
	echo "---------------";
	for i in `seq 0 2`
	do
		for j in `seq 0 2`
		do
			sym=" ";
			echo -n " | ";
			if [[ ${FIELD[3*i+j]} == "X" ]]
				then
				sym="X";
				tput setf $xColor;
			elif [[ ${FIELD[3*i+j]} == "O" ]]
				then
				sym="O";
				tput setf $oColor;
			fi
			echo -n "$sym";
			tput setf 7;

		done
		echo " |";
		echo "---------------";
	done
}

function checkInput {
	if [[ "$1" == "" || "$1" -lt 1 || "$1" -gt 3 ]]
		then
		error=true;
		return;
	fi
	if [[ "$2" == "" || "$2" -lt 1 || "$2" -gt 3 ]]
		then
		error=true;
		return;
	fi
	error=false;
}

function checkPlace {
	let index="($1-1)*3+($2-1)";
	if [[ ${FIELD[$index]} == "X" || ${FIELD[$index]} == "O" ]]
		then
		error=true;
		return;
	fi
	error=false;
	return;
}

function makeTurn {
	FIELD[($1-1)*3+($2-1)]=$mySym;
	TURN=0;
	printField;
}

function setEnemyTurn {
	FIELD[($1-1)*3+($2-1)]=$enemySym;
	TURN=1;
	printField;
}

function checkGameEnd {
	# горизонтальные линии
	combo=${FIELD[0]}${FIELD[1]}${FIELD[2]};
	checkCombo 0;
	combo=${FIELD[3]}${FIELD[4]}${FIELD[5]};
	checkCombo 3;
	combo=${FIELD[6]}${FIELD[7]}${FIELD[8]};
	checkCombo 6;
	# вертикальные линии
	combo=${FIELD[0]}${FIELD[3]}${FIELD[6]};
	checkCombo 0;
	combo=${FIELD[1]}${FIELD[4]}${FIELD[7]};
	checkCombo 1;
	combo=${FIELD[2]}${FIELD[5]}${FIELD[8]};
	checkCombo 2;
	# диагонали
	combo=${FIELD[0]}${FIELD[4]}${FIELD[8]};
	checkCombo 0;
	combo=${FIELD[2]}${FIELD[4]}${FIELD[6]};
	checkCombo 2;
	# ничья
	empty=0;
	for i in `seq 0 8`
	do
		if [[ ${FIELD[$i]} == " " ]]
			then
			empty=$empty+1;
		fi
	done 
	if [[ $empty -eq 0 ]]
		then
		gameEnd=true;
		winner="draw";
	fi
}

function checkCombo {
	if [[ $combo == "XXX" || $combo == "OOO" ]]
		then
		winnerSym=${FIELD[$1]};
		gameEnd=true;
		return;
	fi
}

function defineWinner {
	if [[ $winnerSym == $mySym ]]
		then
		winner="you";
	else
		winner="enemy";
	fi
}

#-----------------------------------

# проверяем, есть ли параметр запуска
if [[ "$#" -ne "1" ]]
	then printHelp;
	exit 0;
fi

# проверяем, клиент или сервер
if [[ "$1" == "-c" ]]
	then
	TYPE="client";
elif [[ "$1" == "-s" ]]
	then
	TYPE="server";
elif [[ "$1" == "-h" ]]
	then
	printHelp;
	exit 1;
else
	printHelp;
	exit 1;
fi

PIPE1=/tmp/axaxax-pipe1
PIPE2=/tmp/axaxax-pipe2

if [[ "$TYPE" == "server" ]]
	then
	# проверяем, существует ли файл
	if [[ ! -p $PIPE1 ]]
		then 
		mknod $PIPE1 p;
	fi

	if [[ ! -p $PIPE2 ]]
		then
		mknod $PIPE2 p;
	fi
	readPipe=$PIPE1;
	writePipe=$PIPE2;
	TURN=0;
	mySym="O";
	enemySym="X";
	xColor="4";
	oColor="2";

elif [[ "$TYPE" == "client" ]]
	then
	if [[ ! -p $PIPE1 || ! -p $PIPE2 ]]
		then
		echo "Server not started";
		exit 1;
	fi
	readPipe=$PIPE2;
	writePipe=$PIPE1;
	TURN=1;
	mySym="X";
	enemySym="O";
	xColor="2";
	oColor="4";
else 
	echo "Error";
	exit 1;
fi

echo "type: $TYPE";

clear;
initField;
printField;

while true
do

	if [[ ! -p "$readPipe" || ! -p "$writePipe" ]]
 	then
		echo "Server not started";
		exit 1
	fi
	
	# ход игрока
	if [[ $TURN -eq 1 ]]
		then
		tput cnorm;
		tput cup 11 0;
		if [[ "$error" == true ]]
			then
			echo -n "Wrong coodinates, try again: ";
			tput sc;
			echo "                        ";
			tput rc;
		else
			echo -n "Make your turn: ";
			tput sc;
			echo -n "                    ";
			tput rc;
		fi
		read x y;
		checkInput x y;

		if [[ "$x" == "exit" ]]
			then
			echo "You leave the game";
			echo -n $x>$writePipe;
			break;
		fi
		if [[ "$error" == true ]]
			then
			continue;
		fi
		# ввели правильные координаты
		# надо проверить, что это место не занято
		checkPlace x y;
		if [[ "$error" == true ]]
			then
			continue;
		fi
		makeTurn x y;

		echo -n $x $y>$writePipe;

		checkGameEnd;
		if [[ "$gameEnd" == true ]]
			then
			break;
		fi

	else 
		# Ход противника, ждем получения данных
		tput cup 11 0;
		echo -n "Waiting enemy turn...";
		echo -n "                               ";
		tput civis;
		read x y<$readPipe;
		if [[ "$x" == "exit" ]]
			then
			echo "\nYour enemy left game";
			break;
		fi
		checkInput x y;
		if [[ "$error" == true ]]
			then
			continue;
		fi
		setEnemyTurn x y;
		checkGameEnd;
		if [[ "$gameEnd" == true ]]
			then
			break;
		fi
	fi
done

if [[ "$winner" == "" ]]
	then 
	defineWinner;
	echo "The winner is $winner";
else
	echo "Drawn game";
fi

# удаляем пайпы
if [[ "$TYPE" == "server" ]]
	then
	if [[ -p "$pipe1" ]]
		then
		rm $pipe1;
	fi
	if [[ -p "$pipe2" ]]
		then
		rm $pipe2;
	fi
fi
tput cnorm;

exit 0;	
