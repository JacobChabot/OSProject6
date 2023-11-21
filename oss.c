#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

	//generate key and allocate shared memory
	int shmId;
	key_t key1 = ftok("./Makefile", 0);
        if ((shmId = shmget(key1, sizeof(int), IPC_CREAT | 0666)) == -1) {
                perror("oss shmId shmget");
                exit(1);
        }
	int * number = shmat(shmId, NULL, 0);
	if (number == (int*)(-1)) {
		perror("oss number shmat");
		exit(1);
	}

	*number = 7;

	pid_t pid;
	pid = fork();
	if (pid == 0) {
		if (execl("./user_proc.out", "./user_proc.out", NULL) == -1) {
                        perror("execl");
                        exit(1);
                }
		exit(0);
	}

	wait(0);

	printf("OSS Program\n");
	printf("Number: %d\n", *number);

	// free memory
	shmctl(shmId, IPC_RMID, NULL);

}
