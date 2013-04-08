#include "smarthome.h"

int main(int argc, char **argv) {
	int opt;
	int setValue = 0;
	int stty = 1;
	char *device = deviceName;
	int action = ACTION_NO;
	int mode = MODE_READ;
	int result = 0;

	while ((opt = getopt(argc, argv, "dt:b:a:rw:h")) != -1) {
		switch (opt) {
		case 's':
			stty = 0;
			break;
		case 'd':
			debugEnabled = 1;
			break;
		case 't':
			device = optarg;
			break;
		case 'b':
			baudRate = atoi(optarg);
			break;
		case 'a':
			switch (optarg[0]) {
			case 'a':
				action = ACTION_AERATION;
				break;
			default:
				action = ACTION_NO;
				break;
			}
			break;
		case 'r':
			mode = MODE_READ;
			break;
		case 'w':
			mode = MODE_WRITE;
			setValue = atoi(optarg);
			break;
		case 'h':
			outputHelp(argv[0]);
			exit(EXIT_SUCCESS);
		default: /* '?' */
			outputHelp(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	switch (action) {
	case ACTION_AERATION:
		serialCommunicationInit(device, baudRate, stty);

		switch (mode) {
		case MODE_READ:
			result = getAeration();
			printf("%d\n", result);
			break;
		case MODE_WRITE:
			result = setAeration(setValue);
			break;
		}

		serialCommunicationClose();
		if (result >= 0) {
			exit(EXIT_SUCCESS);
		} else {
			exit(EXIT_FAILURE);
		}
		break;
	default:
		outputHelp(argv[0]);
		exit(EXIT_FAILURE);
	}
}

int outputHelp(char *name) {
	fprintf(stderr, "Usage: %s -a action\n", name);
	fprintf(stderr, "\n\nArguments:\n");
	fprintf(stderr, "  -h		Show this help\n");
	fprintf(stderr, "  -d		Enable debugging\n");
	fprintf(stderr, "  -s		Disable set of stty command\n");
	fprintf(stderr, "  -t device	Name of device to use for communication\n");
	fprintf(stderr, "  -b baud	Baud rate for communication\n");
	fprintf(stderr, "  -a action	Set the action\n");
	fprintf(stderr, "  -r		Use read as mode (default)\n");
	fprintf(stderr, "  -w val	Write mode with value (numeric)\n");
	fprintf(stderr, "\n\nPossible actions:\n");
	fprintf(stderr, "  a		Read/write acceleration level\n");
	return 0;
}

int pdebug(char *string) {
#ifdef DEBUG_ENABLED
	if (debugEnabled == 1) {
		fprintf(stderr, string);
		fflush(stderr);
	}
#endif
}

int serialCommunicationInit(char *device, int baud, int stty) {
	if (stty == 1) {
		char sttyCmd[150];
		snprintf(sttyCmd, 150, "stty -F %s -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -opost -onlcr ignbrk -brkint -icrnl -imaxbel %d",
						device, baud);
		pdebug(sttyCmd);
		pdebug("\n");
		system(sttyCmd);
	}
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
