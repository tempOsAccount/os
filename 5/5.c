#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 100

fd_set fdset;
struct timeval tv;

int width_1, height_1;
int width_2, height_2;

int matrix_1[MAX_HEIGHT][MAX_WIDTH];
int matrix_2[MAX_HEIGHT][MAX_WIDTH];

struct my_pipe {
	int r;
	int w;
};

my_pipe input_pipes[10];
my_pipe output_pipes[10];

bool isSrc;
bool isHandler;
int handlerIndex = -1;
bool isDest;

int parseFile(char *file) {
	FILE *fp;
 	if ((fp = fopen(file, "r")) == NULL) {
    		printf("cannot open file\n");
    		return 1;
  	}
	
	fscanf(fp, "%d%d", &height_1, &width_1);
	if (0 > height_1 || height_1 > MAX_HEIGHT)
		return 1;
	if (0 > width_1 || width_1 > MAX_WIDTH)
		return 1;

	for (int line = 0; line < height_1; ++line) {
		for (int column = 0; column < width_1; ++column) {
			int val;
			fscanf(fp, "%d", &val);
			matrix_1[line][column]=val;	
		}	
	}  	

	fscanf(fp, "%d%d", &height_2, &width_2);
	if (0 > height_2 || height_2 > MAX_HEIGHT)
		return 1;
	if (0 > width_2 || width_2 > MAX_WIDTH)
		return 1;

	for (int line = 0; line < height_2; ++line) {
		for (int column = 0; column < width_2; ++column) {
			int val;
			fscanf(fp, "%d", &val);
			matrix_2[line][column]=val;	
		}	
	}	

	if (width_1 != height_2)
		return 1;

  	fclose(fp);
  	return 0;
}

int multiplyVectors(int line, int column) {
	int sum = 0;
	for (int i = 0; i < length; ++i)
		sum += matrix_1[line][i] * matrix_2[i][column];	
	return sum;
}

void start() {
	createPipes();
	createAllForks();
	if (1) startSrc();
	else if (2) startHandlers();
	else startDest();
}

void createPipes() {
	int new_pipe[2];
	for (int i = 0; i < 10; ++i) {
		pipe(new_pipe);
		input_pipes[i].r = new_pipe[0];
		input_pipes[i].w = new_pipe[1];	
	}
	for (int i = 0; i < 10; ++i) {
		pipe(new_pipe);
		output_pipes[i].r = new_pipe[0];
		output_pipes[i].w = new_pipe[1];	
	}
}

void startSrc() {
	int handlerNumber = 0;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			int data[2];
			data[0] = i;
			data[1] = j;
			write(input_pipe[(handlerNumber++)%10].w, data, sizeof(int) * 2); 					
		}
	}
}

void startHandler() {
	int pipeForRead = input_pipes[handlerIndex];
	int pipeForWrite = output_pipes[handlerIndex];

	int data[2];
	while (true) {
		init(1, 1, &pipeForRead);
		if (select(FD_SETSIZE, &fdset, NULL, NULL, &tv) == -1)
			break;

		if (!read(pipeForRead, data, sizeof(int) * 2))
            		break;

		int value = multiplyVectors(data[0], data[1]);
		
		init(1, 1, &pipeForWrite);
		if (select(FD_SETSIZE, NULL, &selectVars.fdset, NULL, &selectVars.tv) == -1)
			break;

		write(pipeForWrite, &value, sizeof(int));		
	}
}

int matrix_3[MAX_HEIGHT][MAX_WIDTH];

void startDestination() {
	while (true) {
		bool flag = false;
		init(10, 1, output_pipes);
		if (select(FD_SETSIZE, &selectVars.fdset, NULL, NULL, &selectVars.tv) == -1)
			break;

		int data[3];
		for (int i = 0; i < 10; ++i) {
			if (read(output_pipes[i].r, data, sizeof(int) * 3)) {
				flag = true;
				matrix_3[data[0]][data[1]] = data[2];						
			}
		}
		if (!flag) break;
	}
	for (int line = 0; line < height_1; ++line) {
		for (int column = 0; column < width_2; ++column) {
			printf("%d ", matrix_3[line][column);
		}	
		putchar('\n');
	}
}

void createAllForks() {
	for (int i = 0; i < 11; ++i) {
		pid_t pid = fork();
		if (pid == 0 && i != 10) {
			isHandler = true;
			return;
		}
		else if (pid == 0) {
			isDest = true;
			return;
		}
	}
	isSrc = true;
}

void init(int count, int seconds, int fd[]) {
	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	
	FD_ZERO(&fdset);
	for(int i = 0; i < count; ++i)
		FD_SET(fd[i], &fdset);	
}

int main(int argc, char *argv[]) {
	if (parseFile(argv[1]) != 0)
		return 1;
	
	start();
			
	return 0;
}
