#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <time.h>

struct clock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

// message queue struct
struct reqMsg {
	int processNum;
	int address;
	int operation;
};

int main() {

       	//generate key and allocate shared memory
        struct clock * timer;
        int clockId;
        key_t key1 = ftok("./Makefile", 0);
        if ((clockId = shmget(key1, sizeof(struct clock), IPC_CREAT | 0666)) == -1) {
                perror("uproc clockId shmget");
                exit(1);
        }
        timer = shmat(clockId, NULL, 0);
        if (timer == (struct clock *)(-1)) {
                perror("uproc number shmat");
                exit(1);
        } 

	// create message queue 
	key_t key2 = ftok("./user_proc.c", 0);
        int msgId = msgget(key2, IPC_CREAT | 0666);
        if (msgId == -1) {
                perror("uproc msgget");
                exit(1);
        }
	struct reqMsg message;

	srand(getpid());

        printf("User proc Program\n");
	printf("Seconds: %d Nanoseconds: %d\n", timer->seconds, timer->nanoseconds);

	message.processNum = getpid();
	message.address = rand() % 32001;
	message.operation = rand() % 1;
	if (msgsnd(msgId, &message, sizeof(struct reqMsg) - sizeof(long), 0) == -1) {
        	perror("msgsnd");
        	exit(EXIT_FAILURE);
    	}

	return 0;
}
