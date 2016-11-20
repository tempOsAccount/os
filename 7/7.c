#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

#define HEIGHT 10
#define WIDTH 10

int field[HEIGHT][WIDTH] = {
	{0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,1,1,1,0,0,0},
	{0,0,0,1,0,0,1,0,0,0},
	{0,1,1,1,0,0,1,1,1,0},
	{0,1,0,0,0,0,0,0,1,0},
	{0,1,0,0,0,0,0,0,1,0},
	{0,1,1,1,0,0,1,1,1,0},
	{0,0,0,1,0,0,1,0,0,0},
	{0,0,0,1,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0}
};

int tempField[HEIGHT][WIDTH];

void printField() {
	int i, j;
	for (i = 0; i < WIDTH; i++) {
		for (j = 0; j < HEIGHT; j++) {
			printf("%d ", field[i][j]);
		}
		printf("\n");
	}
}

void readPointNeighbours(int nb[][2], int x, int y) {
	int i, j;
	int k = 0;
	for (i = x - 1; i <= x + 1; i++) {
		for (j = y - 1; j <= y + 1; j++) {
			if (i == x && j == y) {
				continue;
			}
			nb[k][0] = i;
			nb[k][1] = j;
			k++;
		}
	}
}

int countLiveNeighbours(int x, int y) {
	int liveCount = 0;
	int nb[8][2];
	int i, _x, _y;
	readPointNeighbours(nb, x, y);
	for (i = 0; i < 8; i++) {
		_x = nb[i][0];
		_y = nb[i][1];
		if (_x < 0 || _y < 0 || _x >= WIDTH || _y >= HEIGHT) {
			continue;
		}
		if (field[_x][_y]) {
			liveCount++;
		}
	}
	return liveCount;
}

void copyTempField() {
	int i, j;
	for (i = 0; i < WIDTH; i++) {
		for (j = 0; j < HEIGHT; j++) {
			field[i][j] = tempField[i][j];
		}
	}
}

int lifeStep() {
	int i, j, liveCount;
	for (i = 0; i < WIDTH; i++) {
		for (j = 0; j < HEIGHT; j++) {
			liveCount = countLiveNeighbours(i, j);
			switch(liveCount) {
				case 2:
					tempField[i][j] = field[i][j];
					break;
				case 3:
					tempField[i][j] = 1;
					break;
				default: 
					tempField[i][j] = 0;
			}	
		}
	}
	copyTempField();
}

void sendCurrentField(int sock) {
	int err;
	if (err = write(sock, field, sizeof(int)*HEIGHT*WIDTH) < 0) {
		printf("Error with write message to client\n");
		return;
	}
}

void serverHandler() {
	int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client;
    char client_message[2000];
     
    //Создание сокета
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Error with create socket\n");
        exit(1);
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("Error with socket bind\n");
        exit(1);
    }
     
    // прослушиваем в ожидании пользователей
    listen(socket_desc , 3);
    printf("server started!\n");

    c = sizeof(struct sockaddr_in);
    while (1) {
    	//принимаем клиента
    	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	    if (client_sock < 0) {
	        printf("Error with accept client\n");
	        return;
	    }
	    // создаем отдельный поток для работы с клиентом
	    // чтобы сервер мог обслуживать несколько клиентов одновременно
	    pid_t pid = fork();
		if (pid < 0) {
			printf("Error with fork\n");
			return;
		}
		if (pid == 0) {
			close(socket_desc);
			sendCurrentField(client_sock);
			exit(0);
		} else {
			close(client_sock);
		}
    }
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
		FILE* fp;
		fp = fopen(argv[1], "r");
		for (int i = 0; i < HEIGHT; ++i) {
			for (int j = 0; j < WIDTH; ++j) {
				fscanf(fp, "%d", &field[i][j]);
			}		
		}	
		fclose(fp);
	}

	int st;	
	if (fork() != 0)
		exit(0); 	
	pthread_t pth;
	int error;
	// запускаем поток для сервера
	if (error = pthread_create(&pth, NULL, &serverHandler, NULL)) {
		printf("Error with create server thread\n");
		return 1;
	}
	
	pthread_t life;
	// потоки, которые считают конфигурацию жизни
	while(1) {
		//printField();
		//printf("\n");
		if (error = pthread_create(&life, NULL, &lifeStep, NULL)) {
			printf("Error with create life thread\n");
			return 1;
		}

		sleep(1);
		// если не завершился за секунду, выдать ошибку
		if ((error = pthread_kill(life, 0)) == 0) {
			pthread_cancel(life);
			printf("Thread running longer than 1 second\n");
			return 2;
		}

	}
	return 0;
}
