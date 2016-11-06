#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>

#define HOST "localhost"
#define PORT "8888"

#define HEIGHT 10
#define WIDTH 10

int field[HEIGHT][WIDTH];

void printField() {
	int i, j;
	for (i = 0; i < WIDTH; i++) {
		for (j = 0; j < HEIGHT; j++) {
			printf("%d ", field[i][j]);
		}
		printf("\n");
	}
}

int connectToHost() {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	portno = atoi(PORT);

	// Создаем сокет
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		printf("Error with opening socket\n");
		exit(1);
	}

	server = gethostbyname(HOST);

	if (server == NULL) {
		printf("No such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	// коннектимся к серверу
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error with connect to server\n");
		exit(1);
   }
   int err = read(sockfd, field, sizeof(int)*HEIGHT*WIDTH);
   if (err < 0) {
   		printf("error with read from socket\n");
   		return;
   }
   printField();
   return;
}

int main(int argc, char *argv[]) {
	connectToHost();
	return 0;
}
