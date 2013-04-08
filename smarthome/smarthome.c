#include "smarthome.h"

int main(int argc, char **argv) {
	serialCommunicationInit(deviceName, baudRate);

	//setAeration(atoi(argv[1]));
	printf("Test: %d\n", getAeration());

	serialCommunicationClose();
}

int pdebug(char *string) {
#ifdef DEBUG_ENABLED
	if (debugEnabled == 1) {
		fprintf(stderr, string);
		fflush(stderr);
	}
#endif
}

int serialCommunicationInit(char *device, int baud) {
	char sttyCmd[150];
	snprintf(sttyCmd, 150, "stty -F %s -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -opost -onlcr ignbrk -brkint -icrnl -imaxbel %d",
					device, baud);
	pdebug(sttyCmd);
	pdebug("\n");
	system(sttyCmd);
	serial = open(device, O_RDWR | O_NONBLOCK);
	return 0;
}

int serialCommunicationClose() {
	close(serial);
	return 0;
}

int serialCommunication(struct symbol *command, int maxRestarts, char *additionalReceive, int maxAdditionalReceive) {
	int i, j;
	int error = 0;

	struct symbol *c = command;

	int first = 0;
	unsigned char resultCommand = 0x00;
	unsigned char result = 0x00;
	int positionAdditionalReceive = 0;
	ssize_t resultRead = -1;

	do {
		if (first == 0) first = 1;
		else c = c->nextSymbol;

		pdebug("Write: ");
		ssize_t res = write(serial, &(c->send), 1);

		snprintf(debug, DEBUG_BUFFER_SIZE, "%02x with result %d\n", c->send, res);
		pdebug(debug);
		if (res != 1) {
			pdebug("Write Error.");
			error = 1;
			break;
		}

		for (j = 0; j <= c->additionalReceiveCount; j++) {
			resultRead = -1;
			pdebug("Read: ");
                        
			for (i = 0; i < READ_TRY_COUNT && resultRead != 1; i++) {
				resultRead = read(serial, &result, 1);
				snprintf(debug, DEBUG_BUFFER_SIZE, "(%02x,%d) ", result, resultRead);
				pdebug(debug);
				usleep(READ_SLEEP_LENGTH);
			}
			if (resultRead != 1) {
				pdebug("Read Error.");
				error = 1;
				break;
			}
			pdebug("\n");
			result = result & 0xFF;
			if (j == 0) {
				resultCommand = result;
			} else if (positionAdditionalReceive < maxAdditionalReceive) {
				additionalReceive[positionAdditionalReceive++] = result;
			}
		}
                
		snprintf(debug, DEBUG_BUFFER_SIZE, "Send: %02x, Expected: %02x, Result: %02x\n", c->send, c->expectedResult, resultCommand);
		pdebug(debug);

		if (c->expectedResult != resultCommand) {
			pdebug("Read not expected result.\n");
			error = 1;
			break;
		}
	} while(c->hasNextSymbol == 1);

	if (error == 1) {
		if (maxRestarts > 0) {
			pdebug("\n\nRestart:\n");
			return serialCommunication(command, (maxRestarts-1), additionalReceive, maxAdditionalReceive);
		} else {
			return 1;
		}
	}
	return 0;
}

int setAeration(int aeration) {
	char nVal = (char)aeration;

	struct symbol writingAeration	= { .send = 0x00, .expectedResult = 0xFF, .additionalReceiveCount = 0, .hasNextSymbol = 1 };
	struct symbol setModeWriting	= { .send = 0x01, .expectedResult = 0x00, .additionalReceiveCount = 0, .hasNextSymbol = 1 };
	struct symbol writeAeration	= { .send = nVal, .expectedResult = 0x01, .additionalReceiveCount = 0, .hasNextSymbol = 0 };
	writingAeration.nextSymbol = &setModeWriting;
	setModeWriting.nextSymbol = &writeAeration;

	return serialCommunication(&writingAeration, 10, 0, 0);
}

int getAeration() {
	int resultCommunication = 0;
	char readedResult[] = { 0x00, 0xFF };

	struct symbol readingAeration	= { .send = 0x00, .expectedResult = 0xFF, .additionalReceiveCount = 0, .hasNextSymbol = 1 };
	struct symbol setModeReading	= { .send = 0x02, .expectedResult = 0x00, .additionalReceiveCount = 2, .hasNextSymbol = 0 };
	readingAeration.nextSymbol = &setModeReading;

	resultCommunication = serialCommunication(&readingAeration, 10, readedResult, 2);

	if (resultCommunication == 0) {
		if (readedResult[0] != ~readedResult[1]) {
			return getAeration();
		}
        
		return readedResult[0];
	} else {
		return -1;
	}
}
