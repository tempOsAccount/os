#include <stdio.h>
#include <stdlib.h>

#define MAX_NUMBERS 10000000

int numbers[MAX_NUMBERS];
int count = 0;

int readFile (char *name) {
	FILE *fp;
	char *errorString;
	if ((fp=fopen(name, "r")) == NULL) {
		asprintf(&errorString, "Cannot open file %s\n", name);
		fputs(errorString,  stderr);
		free(errorString);
		return 1;
	}
	int numbersCount = 0;
  	fseek(fp , 0 , SEEK_END);
  	long lSize = ftell(fp);
  	rewind (fp);

  	char * buffer = (char*) malloc (sizeof(char)*lSize);
  	if (buffer == NULL) {
  		asprintf(&errorString, "Memory error while reading file %s\n", name);
		fputs(errorString,  stderr);
		free(errorString);
		return 2;
	}

  	size_t result = fread(buffer, 1, lSize, fp);
  	if (result != lSize) {
  		asprintf(&errorString, "Cannot read file %s\n", name);
		fputs(errorString,  stderr);
		free(errorString);
  		return 3;
  	}
  	fclose(fp);

  	char sym;
  	int i, number = 0;
  	for (i = 0; i < lSize; i++) {
  		sym = buffer[i];
  		if (sym >= '0' && sym <= '9') {
  			number = (number * 10) + (sym - '0');
  		} else {
  			if (number) {
  				numbers[count] = number;
  				count++;
  				number = 0;
  			}
  		}

  		if (count == MAX_NUMBERS) {
  			fputs ("Too much numbers", stderr);
  			return 4;
  		}
  	}
  	free (buffer);
 	return 0;
}

int compare(const void *a, const void *b) {
	return ( *(int*)a - *(int*)b );
}

int writeNumbers(char *name) {
	FILE *fp;
	char *errorString;
	if ((fp=fopen(name, "w")) == NULL) {
		asprintf(&errorString, "Cannot open file %s\n", name);
		fputs(errorString,  stderr);
		free(errorString);
		return 1;
	}	
	int i, n;
	for (i = 0; i < count; i++) {
		n = fprintf(fp, "%d ", numbers[i]);
		if (n < 0) {
			asprintf(&errorString, "Cannot write to file %s\n", name);
			fputs(errorString,  stderr);
			free(errorString);
			return 2;
		}
	}
	fclose(fp);
}

int checkSort() {
	int i;
	for(i = 0; i < count - 1; i++) {
		if(numbers[i] > numbers[i+1]) {
			return 1;
		}
	}
	return 0;
}

int main (int argc, char *argv[]) {
	if (argc == 1) {
		printf("Need filenames in arguments, exit\n");
		return 1;
	}
	int i, error;
	for (i = 1; i < argc - 1; i++) {
		error = readFile(argv[i]);
	}
	qsort(numbers, (size_t)count, sizeof(int), compare);
	if((error = checkSort()) != 0) {
		fputs("Wrong sort",  stderr);
	}
	error = writeNumbers(argv[argc - 1]);
	if (error) {
		return -1;
	}
	return 0;
}
