#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int BUF_SIZE = 1000;

int unpackSparse(char *name) {
	int fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		printf("Cannot open file %s\n", name);
		return 1;
	}
	int n;
	char buffer[BUF_SIZE];
	int nullCount = 0;
	int error = 0;
	int blocksWritten = 0;

	while ((n = read(0, buffer, BUF_SIZE)) != 0) {
		int i;
		for (i = 0; i < n; i++) {
			if (buffer[i] == 0) {
				nullCount++;
			} else {
				++blocksWritten;
				if (nullCount != 0)
				error = lseek(fd, nullCount, SEEK_CUR);
				nullCount = 0;	
				error = write(fd, &buffer[i], 1);
				if (error < 0) {
					printf("Cannot write to file %s\n", name);
					return 2;
				}
			}
		}
	}

	if (blocksWritten == 0) {
		ftruncate(fd, nullCount);
	} else if (nullCount > 0) {
		lseek(fd, nullCount, SEEK_CUR);
	}
	close(fd);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
		printf("Filename not set\n");
		return -1;
	}	
	return unpackSparse(argv[1]);
}
