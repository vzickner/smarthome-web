#include "smarthome.h"

int main(int argc, char **argv) {
	serialCommunicationInit();

	setAeration(atoi(argv[1]));

	serialCommunicationClose();
}

int pdebug(char *string) {
#ifdef DEBUG_ENABLED
	if (debugEnabled == 1) {
		printf(string);
		fflush(stdout);
	}
#endif
}

int serialCommunicationInit() {
	system("stty -F /dev/ttyUSB0 -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -opost -onlcr ignbrk -brkint -icrnl -imaxbel 38400");
	serial = open("/dev/ttyUSB0", O_RDWR | O_NONBLOCK);
	return 0;
}

int serialCommunicationClose() {
	close(serial);
	return 0;
}

int serialCommunication(struct symbol *command, int maxRestarts) {
	int error = 0;
	struct symbol *c = command;
	int first = 0;
	unsigned char send;
	unsigned char result = 0x00;
	int i = 0;
	ssize_t resultRead = -1;

	do {
		if (first == 0) first = 1;
		else c = c->nextSymbol;

		send = c->send;
		pdebug("Write: ");
		ssize_t res = write(serial, &send, 1);

		snprintf(debug, 50, "%02x with result %d\n", send, res);
		pdebug(debug);

		resultRead = -1;
		pdebug("Read: ");
		for (i = 0; i < 100 && resultRead != 1; i++) {
			resultRead = read(serial, &result, 1);
			snprintf(debug, 50, "(%02x,%d) ", result, resultRead);
			pdebug(debug);
			usleep(100);
		}
		pdebug("\n");
		result = result & 0xFF;
                
		snprintf(debug, DEBUG_BUFFER_SIZE, "Send: %02x, Expected: %02x, Result: %02x\n", send, c->expectedResult, result);
		pdebug(debug);

		if (c->expectedResult != result) {
			pdebug("ERROR!\n");
			error = 1;
			break;
		}
	} while(c->hasNextSymbol == 1);

	if (error == 1 && maxRestarts > 0) {
		pdebug("\n\nRestart:\n");
		return serialCommunication(command, (maxRestarts-1));
	}
}

int setAeration(int aeration) {
	char nVal = (char)aeration;

	struct symbol writingAeration	= { .send = 0x00, .expectedResult = 0xFF, .additionalReceiveCount = 0, .hasNextSymbol = 1 };
	struct symbol setModeWriting	= { .send = 0x01, .expectedResult = 0x00, .additionalReceiveCount = 0, .hasNextSymbol = 1 };
	struct symbol writeAeration	= { .send = nVal, .expectedResult = 0x01, .additionalReceiveCount = 0, .hasNextSymbol = 0 };
	writingAeration.nextSymbol = &setModeWriting;
	setModeWriting.nextSymbol = &writeAeration;

	serialCommunication(&writingAeration, 10);
}

int getAeration() {
}
