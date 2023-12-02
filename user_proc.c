#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>

#define MAX_PROC 18

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

struct pcb {
        pid_t pid;
        int number;
};

// signal handler
void signalHandler(int signum) {
}

int main(int argc, char* argv[]) {
	

	sleep(2);
	
	const int j = atoi(argv[1]); // own process number
	const int processNum = atoi(argv[2]); // number of processes


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
	struct reqMsg smessage;

	// shared memory of pcbs
        key_t key3 = ftok("./oss.c", 0);
        int blockId = shmget(key3, sizeof(struct pcb) * processNum, IPC_CREAT | 0666);
        if (blockId == -1) {
                perror("oss pcb shmget");
                exit(EXIT_FAILURE);
        }
        struct pcb * block = (struct pcb *)shmat(blockId, NULL, 0);
        if (block == (struct pcb *)(-1)) {
                perror("oss pcb shmat");
                exit(EXIT_FAILURE);
        }
	block[j].pid = getpid();
	block[j].number = j;

	srand(getpid());


	smessage.processNum = j;
	smessage.address = rand() % 32001;
	smessage.operation = rand() % 1;
	int random = (rand() % 5) + 1;
	int counter = 0;
	while (counter != random) { // loop random number of times requesting memory

		if (signal(SIGCONT, signalHandler) == SIG_ERR) { // signal handler for SIGCONT
        		perror("signal");
        		exit(EXIT_FAILURE);
    		}

		if (msgsnd(msgId, &smessage, sizeof(struct reqMsg) - sizeof(long), 0) == -1) {
        		perror("msgsnd");
        		exit(EXIT_FAILURE);
    		}

		pause(); // pause and wait for signal to be received to continue
		
		printf("User process memory acquired\n");
		sleep(2); // simulate some work

		counter++;
	}

	return 0;
}
