#include <stdio.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <string.h>
#include <libgen.h>

#define LINE_SIZE 200
#define MAX_PROGRAMS 50
#define MAXPROC 50
#define ARG_LENGTH 50

//статусы дочерних процессов
pid_t pid_list[MAXPROC];
int pid_count = 0;

int programsCount = 0;

struct program {
  char name[50];
  char *arguments[20];
  int type;
};

char *WAIT = "wait";
char *RESPAWN = "respawn";

struct program programs[MAX_PROGRAMS];

void saveType(int argc, int type) {
  // NULL для execvp
  // type = 0 - одноразовый запуск
  // type = 1 - respawn
  programs[programsCount].arguments[argc] = NULL;
  programs[programsCount].type = type;  
}

char* getName(int i) {
  char *name = programs[i].name;
  if(name[0] == '.') {
    return name;
  }
  return basename(name);
}

int parseConfig(char *cfg) {
  FILE *fp;
  if ((fp = fopen(cfg, "r")) == NULL) {
    printf("cannot open config file\n");
    return 1;
  }
  char *line = malloc(LINE_SIZE);
  char *temp;
  while (fgets(line, LINE_SIZE, fp) != NULL)  {
      temp = strtok(line,  "\n:");
      strcpy(programs[programsCount].name, temp);
      // первый аргумент всегда название программы
      programs[programsCount].arguments[0] = (char*) malloc(ARG_LENGTH);
      strcpy(programs[programsCount].arguments[0], temp);
      int argc = 1;
      // дальше читаем аргументы, пока не дойдем до :
      while ((temp = strtok(NULL, "\n:")) != NULL)  {
        if (strcmp(temp, WAIT) == 0) {
          saveType(argc, 0);
          break;
        }
        if (strcmp(temp, RESPAWN) == 0) {
          saveType(argc, 1);
          break;
        } 
        programs[programsCount].arguments[argc] = (char*) malloc(ARG_LENGTH);
        strcpy(programs[programsCount].arguments[argc], temp);
        argc++;
      }
      programsCount++;
  }
  fclose(fp);
  return 0;
}

int writePid(pid_t pid, int i) {
  char *name = getName(i);
  char fullName[300];
  snprintf(fullName, sizeof(fullName), "/tmp/%s_%d.pid", name, i);
  FILE *fp;
  if ((fp = fopen(fullName, "w")) == NULL) {
    printf("cannot open file %s\n", fullName);
    return 1;
  }
  fprintf(fp, "%ld", (long)pid);
  fclose(fp);
  return 0;
}

// передаем номер программы в массиве
int forkProc(int i) {
  pid_t cpid = fork(); 
  switch (cpid) { 
      // ошибка
    case -1:
        printf("Fork failed; cpid == -1\n");
        return 1; 
        // дочерний процесс
    case 0: 
        cpid = getpid();         //global PID  
        execvp(programs[i].name, programs[i].arguments);
        exit(0);
        // родительский процесс
    default:
      pid_list[i] = cpid;
      pid_count++;
      writePid(cpid, i);
    }
    return 0;
}

void deleteTempFile(int i) {
  char *name = getName(i);
  char fullName[300];
  snprintf(fullName, sizeof(fullName), "/tmp/%s_%d.pid", name, i);
  remove(fullName);
}

int processPrograms() {
  pid_t pid;
  int i;
  while (pid_count) {
    // ждем, пока завершится какой-нибудь процесс
    pid = waitpid(WAIT_ANY, NULL, 0);
    for (i = 0; i < programsCount; i++) {
      if (pid_list[i] != pid) {
        continue;
      }
      // нашли нужный процесс
      if (programs[i].type == 0) {
        // надо его полностью удалить
        // сначала освободить память, выделенную под аргументы
        int j = 0;
        while (programs[i].arguments[j] != NULL) {
          free(programs[i].arguments[j]);
          j++;
        }
        deleteTempFile(i);
        pid_list[i] = 0;
        pid_count--;
      } else {
        // завершился respawn-процесс, надо перезапустить
        int err = forkProc(i);
        if (err) {
          printf("error with fork!\n");
          return 1;
        }
      }
    }
  }
  return 0;
}

int startProcs() {
  int i, err;
  for (i = 0; i < programsCount; i++) {
    err = forkProc(i);
    if (err) {
      printf("Error with fork!\n");
      return 1;
    }
  }
  err = processPrograms();
  if (err) {
    printf("error in processPrograms\n");
    return 2;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Need config file in argument\n");
    return 1;
  }
  int err = parseConfig(argv[1]);
  if (err) {
    return 1;
  }
  startProcs();

  return 0;
}
