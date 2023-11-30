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

#define MAX_PROC 18
#define TOT_MEM 256

struct clock { // struct for clock 
	unsigned int seconds;
	unsigned int nanoseconds;
};

struct reqMsg { // struct for message queue
	int processNum;
	int address;
	int operation; // read = 0, write = 1
};

struct pageTable { // struct for page table
	int number;
	int size;
	int valid;
	int dirty;
};

struct frameTable { // struct for frame table
	int pageNumber;
	int inUse;
};

void help();
void logFile();

int main(int argc, char** argv) {

	int access;
	int processNum;

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
	struct reqMsg message;

	struct pageTable pageTableArray[32]; // array for page tables

	pid_t pid;
	pid = fork();
	if (pid == 0) {
		if (execl("./user_proc.out", "./user_proc.out", NULL) == -1) {
                        perror("execl");
                        exit(1);
                }
		exit(0);
	}

	// wait for message
	if (msgrcv(msgId, &message, sizeof(struct reqMsg) - sizeof(long), 0, 0) == -1) {
        	perror("msgrcv");
                exit(1);
        }
	printf("Signal received from process: %d requesting address %05d for operation %d\n", message.processNum, message.address, message.operation);
	logFile();
	sleep(2);

	timer->seconds = time(NULL) - tempTime;
	timer->nanoseconds = (time(NULL) - tempTime) * 1000000000;

	printf("OSS Program\n");
	printf("Seconds: %d Nanoseconds: %d\n", timer->seconds, timer->nanoseconds);

	// free memory
	shmctl(clockId, IPC_RMID, NULL);
	msgctl(msgId, IPC_RMID, NULL);

	return 0;
}

void help() {
	printf("Help function\n");
	exit(0);
}

void logFile() {
	printf("Log file function\n");
	
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

	fprintf(file, "OSS at %s\n", timeString);

	fclose(file);
}
