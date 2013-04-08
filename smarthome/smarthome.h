#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define DEBUG_ENABLED
#define DEBUG_BUFFER_SIZE 50

#define READ_SLEEP_LENGTH 100
#define READ_TRY_COUNT 40

struct symbol {
	unsigned char send;
	unsigned char expectedResult;
	unsigned char additionalReceiveCount;
	unsigned char hasNextSymbol;
	struct symbol *nextSymbol;
};

int serial;
int debugEnabled = 0;
char debug[DEBUG_BUFFER_SIZE];
char deviceName[] = "/dev/ttyUSB0";
int baudRate = 38400;

int pdebug(char *string);
int serialCommunicationInit();
int serialCommunicationClose();
int serialCommunication(struct symbol *command, int maxRestarts, char *additionalReceive, int maxAdditionalReceive);
int setAeration(int aeration);
int getAeration();
