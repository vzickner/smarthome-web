#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define DEBUG_ENABLED
#define DEBUG_BUFFER_SIZE 50

#define READ_SLEEP_LENGTH 100
#define READ_TRY_COUNT 40

#define ACTION_NO 0
#define ACTION_AERATION 1

#define MODE_READ 0
#define MODE_WRITE 1

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
char deviceName[] = "/dev/ttyAMA0";
int baudRate = 9600;

int outputHelp(char *name);
int pdebug(char *string);
int serialCommunicationInit(char *device, int baud, int stty);
int serialCommunicationClose();
int serialCommunication(struct symbol *command, int maxRestarts, char *additionalReceive, int maxAdditionalReceive);
int setAeration(int aeration);
int getAeration();
