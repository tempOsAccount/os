// example: ./a.out pass.txt user 123

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>

#define USERS_COUNT 100
#define LINE_SIZE 100
struct user {
	char name[50];
	char password[50];
}; 
struct user USERS[USERS_COUNT];
char lckFile[40];

int waitUnlock(char *lck) {
	int error;
	FILE *fp;
	if ((fp = fopen(lck, "r")) == NULL) {
		printf("cannot open lock file\n");
		return 1;
	}
	// читаем pid и тип операции - 0 чтение, 1 запись
	int pid, type;
	fscanf(fp, "%d %d", &pid, &type);
	printf("pid: %d operation: %d\n", pid, type);
	fclose(fp);
	sleep(10);
	return 0;
}

char * getLckName(char *name) {
	int nameLength = sizeof(name) / sizeof(name[0]);
    strcpy(lckFile, name);
    strcat(lckFile, ".lck");
    return lckFile;
}

void printToLck(char *name, int type) {
	getLckName(name);
	FILE *fp = fopen(lckFile, "w");
	fprintf(fp, "%d %d", getpid(), type);
	fclose(fp);
}

int setNewPassword(char *file, char *user, char *password) {
	FILE *ifp, *ofp;
	// открываем на чтение и запись
	if ((ifp=fopen(file, "r")) == NULL) {
		printf("Cannot open password file\n");
		return 1;
	}
	char* line = malloc(LINE_SIZE);
	int fd = fileno(ifp);
	// устанавливаем эксклюзивную блокировку
	flock(fd, LOCK_EX);
	printToLck(file, 0);
	int i = 0;
	// читаем построчно и ищем имя пользователя
	while (fgets(line, LINE_SIZE, ifp) != NULL)  {
    	// получаем имя пользователя
    	char *temp = strtok(line, " ");
    	strcpy(USERS[i].name, temp);
    	// нашли пользователя, меняем пароль
    	if(strcmp(temp, user) == 0) {
    		strcpy(USERS[i].password, password);
    	} else {
    		temp = strtok(NULL, " \n");
    		strcpy(USERS[i].password, temp);
    	}
    	i++;
	}
	int count = i;
	fclose(ifp);
	printToLck(file, 1);
	if ((ofp=fopen(file, "w")) == NULL) {
		printf("Cannot open password file\n");
		return 1;
	}
	for (i = 0; i < count; i++) {
		fprintf(ofp, "%s %s\n", USERS[i].name, USERS[i].password);
	}
	free(line);
	sleep(10);
	flock(fd, LOCK_UN);
	fclose(ofp);
	return 0;
}

int changePassword(char *file, char *user, char *password) {
	getLckName(file);
    // ждем освобождения файла
    while (1) {
    	if(access(lckFile, F_OK) == -1) {
			break;
		}
		waitUnlock(lckFile);
    }
    // файл освободился, можем менять пароль
    int r = setNewPassword(file, user, password);
    remove(lckFile);
    return 0;
}

int main(int argc, char *argv[]) {
	// первый аргумент - имя файла с паролями
	// второй аргумент - имя пользователя
	// третий аргумент - пароль
	if (argc < 4) {
		printf("Need file, username and password in arguments\n");
		return 1;
	}
	return changePassword(argv[1], argv[2], argv[3]);
}
