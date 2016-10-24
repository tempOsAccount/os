#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_NUMBERS 100

int numbers[MAX_NUMBERS];
int numbersCount = 0;
int pipes[MAX_NUMBERS];

int parseFile(char *file) {
	FILE *fp;
 	if ((fp = fopen(file, "r")) == NULL) {
    		printf("cannot open file\n");
    		return 1;
  	}
  	int n, i;
  	while((fscanf(fp, "%d", &n)) == 1) {
  		numbers[numbersCount] = n;
  		numbersCount++;
  		if (numbersCount == MAX_NUMBERS) {
  			printf("Error, too much numbers in file\n");
  			return 2;
  		}
  	}
  	fclose(fp);
  	return 0;
}

int fibonacci(int number) {
	int a = 0;
	int b = 1;
	int i;
	if (number == 0) {
		return a;
	}
	if (number == 1) {
		return b;
	}
	int temp;
	for (i = 1; i < number; i++) {
		temp = a;
		a = b;
		b = temp + a;
	}
	return b;
}

int parallel(int r, int w, int seconds) {
	fd_set set;
	struct timeval timeout;

	FD_ZERO (&set);
	FD_SET (r, &set);

	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	int err;
	if ((err = select(FD_SETSIZE, &set, NULL, NULL, &timeout)) == -1) {
		printf("Error with select\n");
		exit(1);
	}
	// дождались данных, получаем число из пайпа
	int number;
	read(r, &number, sizeof(number));
	int fib = fibonacci(number);
	// ждем записи в пайп
	FD_SET (w, &set);
	if ((err = select(FD_SETSIZE, &set, NULL, NULL, &timeout)) == -1) {
		printf("Error with select\n");
		exit(1);
	}
	// возвращаем результат
	write(w, &fib, sizeof(fib));
	return 0;
}

int forkProc(int i) {
	// fd[0] для чтения, fd[1] для записп
	int fd[2];
	int err = pipe(fd);
	if (err) {
		printf("Cannot create pipe\n");
		return 2;
	}
  	pid_t cpid = fork(); 
  	switch (cpid) { 
	      // ошибка
	    case -1:
	        printf("Fork failed; cpid == -1\n");
	        return 1; 
	        // дочерний процесс
	    case 0: 
	        cpid = getpid();         //global PID  
	        parallel(fd[0], fd[1], 1);
	        exit(0);
	        // родительский процесс
	    default:
	    	// передаем дочернему процессу число
	    	write(fd[1], &numbers[i], sizeof(numbers[i]));
	    	pipes[i] = fd[0];
    }	
    return 0;
}

int startProcs() {
	int i, err;
  	for (i = 0; i < numbersCount; i++) {
    	err = forkProc(i);
    	if (err) {
      		printf("Error with fork!\n");
      		return 1;
    	}
  	}
  	return 0;
}


int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Need file name in arguments\n");
		return 1;
	}
	int err = parseFile(argv[1]);
	if (err) {
		printf("Cannot parse file\n");
		return 2;
	}
	err = startProcs();
	if (err) {
		printf("Error with starting processes\n");
		return 3;
	}
	// ждем пока все дочерние процессы завершатся
	int wpid, i, res;
	while((wpid = wait(NULL)) > 0);
	// выводим результаты
	for (i = 0; i < numbersCount; i++) {
		read(pipes[i], &res, sizeof(res));
		printf("%d\n", res);
	}
	return 0;
}
