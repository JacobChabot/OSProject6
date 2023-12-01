#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_PROC 18
#define TOT_MEM 256

struct clock { // struct for clock 
	unsigned int seconds;
	unsigned int nanoseconds;
};

struct pcb {
	pid_t pid;
	int number;
};

struct reqMsg { // struct for message queue
	int processNum;
	int address;
	int operation; // read = 0, write = 1
};

struct pageTable { // struct for page table
	int pageNumber;
	int size;
	int valid;
	int dirty;
};

struct frameTable { // struct for frame tablei
	int frameNumber;
	int inUse;
};

void help();
void logFile(int, int, int, int);
void updatePageTable(struct pageTable [], int, int);
void updateFrameTable(struct frameTable [], int, int);
void forkChild(int, int);

int main(int argc, char** argv) {

	int access;
	int processNum = MAX_PROC;

	char ch;
	while((ch = getopt(argc, argv, "hm:p:")) != -1) {
		switch(ch) {
			case 'h':
				help();
				break;
			case 'm':
				access = atoi(strdup(optarg));
				if (access > 1 || access < 0) {
					fprintf(stderr, "Invalid memory access option.");
					exit(1);
				}
				continue;
			case 'p':
				processNum = atoi(strdup(optarg));
				if (processNum > MAX_PROC) {
					processNum = MAX_PROC; // if process number from user exceeds 18, overwrite option and set to 18
				}
				continue;
			default:
				fprintf(stderr, "Unrecogonized options\n");
				break;
		}
	}
	printf("OSS\n");

	//generate key and allocate shared memory for clock
	struct clock * timer;
	int clockId;
	key_t key1 = ftok("./Makefile", 0);
        if ((clockId = shmget(key1, sizeof(struct clock), IPC_CREAT | 0666)) == -1) {
                perror("oss clockId shmget");
                exit(1);
        }
	timer = shmat(clockId, NULL, 0);
	if (timer == (struct clock *)(-1)) {
		perror("oss number shmat");
		exit(1);
	}
	time_t tempTime = time(NULL); // store initial time
	timer->seconds = 0;
	timer->nanoseconds = 0;

	// create message queue
	key_t key2 = ftok("./user_proc.c", 0);
	int msgId = msgget(key2, IPC_CREAT | 0666);
	if (msgId == -1) {
		perror("oss msgget");
		exit(1);
	}
	struct reqMsg rmessage;

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

	struct pageTable pageTableArray[32]; // array for page tables
	struct frameTable frameTableArray[32];
	for (int i = 0; i < 32; i++) {
		pageTableArray[i].pageNumber = i;
		pageTableArray[i].size = 1; // 1k
	}

	bool stillRun = true;
	int pcount = 0;
	int forkTime = (rand() % 500) + 1; // time variable to fork
	int stopwatch = 1; // timer to fork in ms
	while (stillRun) {
		//printf("while loop in oss\n");	
		timer->seconds = time(NULL) - tempTime; // update clock
                timer->nanoseconds = (time(NULL) - tempTime) * 1000000000;
		
		stopwatch++;
		// set up forking at random times
		if (pcount < processNum)
			if (forkTime == stopwatch) {
				forkChild(pcount, processNum);
				stopwatch = 0;
				pcount++;
				forkTime = (rand() % 500) + 1;
			}
	

		// check for message 
        	if (msgrcv(msgId, &rmessage, sizeof(struct reqMsg) - sizeof(long), 0, IPC_NOWAIT) == -1) {
			if (errno != ENOMSG) { // if any other error besides ENOMSG (message not received) stop execution
				perror("msgrcv");
				exit(1);
			}
		}
		else { // message received
			printf("Signal received from process: %d requesting address %05d for operation %d\n", rmessage.processNum, rmessage.address, rmessage.operation);
			logFile(0, rmessage.processNum, rmessage.address, 0); // void logFile(int type, int pnum, int address, int frame)
			updatePageTable(pageTableArray, rmessage.processNum, rmessage.address);
			sleep(2);
			logFile(1, rmessage.processNum, rmessage.address, 0);
			if (kill(block[rmessage.processNum].pid, SIGCONT) == -1) { // signal to process it can continue
                                perror("kill");
                                exit(EXIT_FAILURE);
                       	}
		}
		//sleep(1);

		for (int i = 0; i < processNum; i++) {
			int result = kill(block[i].pid, 0);
			if (result == 0)
				break;
			if (i == processNum)
				stillRun = false; // no child processes running so stop loop
		}

        }


	// free memory
	shmctl(clockId, IPC_RMID, NULL);
	msgctl(msgId, IPC_RMID, NULL);

	return 0;
}

void help() {
	printf("Help function\n");
	exit(0);
}

void updatePageTable(struct pageTable pageTablearray[], int pnum, int address) {
	printf("Update Page Table Function\n");
}

void updateFrameTable(struct frameTable frameTableArray[], int pnum, int address) {
	printf("Update frame table function\n");
}

void logFile(int type, int pnum, int address, int frame) {
	//type: request = 0, giving = 1, fault = 2

	// open input file
	FILE * file = NULL;
	file = fopen("logfile", "a");
	if (file == NULL) {
		fprintf(stderr, "Invalid input file name\n");
		exit(0);
	}

	// get time in HH:MM:SS format
        time_t currentTime;
        struct tm * timeInfo;
        char timeString[9];
        time(&currentTime);
        timeInfo = localtime(&currentTime);
        strftime(timeString, sizeof(timeString), "%H:%M:%S", timeInfo);

	if (type == 0) 
		fprintf(file, "OSS: P%d requesting read of address %05d at time %s\n", pnum, address, timeString);
	else if (type == 1)
		fprintf(file, "OSS: Address %05d in frame %d, giving data to P%d at time %s\n", address, frame, pnum, timeString);
	else if (type == 2)
		fprintf(file, "OSS: Address %05d is not in a frame, pagefault\n", address);

	fclose(file);
}

void forkChild(int pcount, int processNum) {
	char temp[10];
	char nTemp[10];
	snprintf(temp, sizeof(temp), "%d", pcount);
	snprintf(nTemp, sizeof(nTemp), "%d", processNum);
	
	printf("execute child\n");

	pid_t pid = fork();
	if (pid == 0) { // if child, execute user program and termiante
		if (execl("./user_proc.out", "./user_proc.out", temp, nTemp, NULL) == -1) {
			perror("oss execl");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}
}
