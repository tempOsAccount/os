#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define WIDTH 3
#define HEIGHT 3
#define THREADS 4

fd_set fdset;
struct timeval tv;

int matrix_1[HEIGHT][WIDTH];
int matrix_2[HEIGHT][WIDTH];
int matrix_3[HEIGHT][WIDTH];

struct my_pipe {
	int r;
	int w;
};

my_pipe input_pipes[THREADS];
my_pipe output_pipes[THREADS];

bool isSrc;
bool isHandler;
int handlerIndex = -1;
bool isDest;

void init(int count, int fd[]) {
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	FD_ZERO(&fdset);
	for(int i = 0; i < count; ++i)
		FD_SET(fd[i], &fdset);	
}

int parseFile(char *file) {
	FILE *fp;
 	if ((fp = fopen(file, "r")) == NULL) {
    		printf("cannot open file\n");
    		return 1;
  	}
	
	for (int line = 0; line < HEIGHT; ++line) {
		for (int column = 0; column < WIDTH; ++column) {
			int val;
			fscanf(fp, "%d", &val);
			matrix_1[line][column]=val;	
		}	
	}

	for (int line = 0; line < HEIGHT; ++line) {
		for (int column = 0; column < WIDTH; ++column) {
			int val;
			fscanf(fp, "%d", &val);
			matrix_2[line][column]=val;	
		}	
	}	

  	fclose(fp);
  	return 0;
}

int multiplyVectors(int line, int column) {
	int sum = 0;
	for (int i = 0; i < WIDTH; ++i)
		sum += matrix_1[line][i] * matrix_2[i][column];	
	return sum;
}

void startDestination() {
	int temp[THREADS];
	for (int i = 0; i < THREADS; ++i)
		temp[i] = output_pipes[i].r;
		
	while (true) {
		bool flag = false;
		init(THREADS, temp);
		int selectResult = select(FD_SETSIZE, &fdset, NULL, NULL, &tv);
		if (selectResult == 0 || selectResult == -1)
			break;

		int data[3];
		for (int i = 0; i < THREADS; ++i) {	
			if (FD_ISSET(output_pipes[i].r, &fdset) && read(output_pipes[i].r, data, sizeof(int) * 3)) {
				flag = true;
				matrix_3[data[0]][data[1]] = data[2];					
			}
		}
		if (!flag) break;	
	}

	FILE* fp = fopen("output", "w");
	for (int i = 0; i < HEIGHT; ++i) {
		for (int j = 0; j < WIDTH; ++j)
			fprintf(fp, "%d ", matrix_3[i][j]);
		fprintf(fp, "\n");
	}
	fclose(fp);
}

void startHandler() {
	int pipeForRead = input_pipes[handlerIndex].r;
	int pipeForWrite = output_pipes[handlerIndex].w;	

	int data[2];
	while (true) {
		init(1, &pipeForRead);
		
		int selectResult = select(FD_SETSIZE, &fdset, NULL, NULL, &tv);
		if (selectResult == 0 || selectResult == -1)
			break;

		if (!read(pipeForRead, data, sizeof(int) * 2))
            		break;

		int value = multiplyVectors(data[0], data[1]);
		
		init(1, &pipeForWrite);
		selectResult = select(FD_SETSIZE, NULL, &fdset, NULL, &tv);
		if (selectResult == 0 || selectResult == -1)
			break;

		int answer[3];
		answer[0] = data[0];
		answer[1] = data[1];
		answer[2] = value;
		write(pipeForWrite, answer, sizeof(int) * 3);		
	}
}

void startSrc() {
	int handlerNumber = 0;
	for (int i = 0; i < HEIGHT; ++i) {
		for (int j = 0; j < WIDTH; ++j) {
			int data[2];
			data[0] = i;
			data[1] = j;
			write(input_pipes[(handlerNumber++)%THREADS].w, data, sizeof(int) * 2); 					
		}
	}
}

void createAllForks() {
	for (int i = 0; i < THREADS + 1; ++i) {
		pid_t pid = fork();
		if (pid == 0) { 
			if (i != THREADS) {
				isHandler = true;
				handlerIndex = i;
				return;
			} else {
				isDest = true;
				return;
			}
		}
	}
	isSrc = true;
}

void createPipes() {
	int new_pipe[2];
	for (int i = 0; i < THREADS; ++i) {
		pipe(new_pipe);
		input_pipes[i].r = new_pipe[0];
		input_pipes[i].w = new_pipe[1];	
	}
	for (int i = 0; i < THREADS; ++i) {
		pipe(new_pipe);
		output_pipes[i].r = new_pipe[0];
		output_pipes[i].w = new_pipe[1];	
	}
}

void start() {
	createPipes();	
	createAllForks();
	if (isSrc) startSrc();
	else if (isHandler) startHandler();
	else startDestination();
}

int main(int argc, char *argv[]) {
	if (parseFile(argv[1]) != 0)
		return 1;
	
	start();			
	return 0;
}
