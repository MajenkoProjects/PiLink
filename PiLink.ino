
#define ST_LOCAL  0x00
#define ST_CALL   0x01
#define ST_ONLINE 0x02

#define SE_ECHO   0x01
#define SE_QUIET  0x02
#define SE_VERBOSE 0x04

#define SE_DEFAULT (SE_VERBOSE | SE_ECHO)

struct connection {
	Stream *stream;
	char buffer[100];
	int remote;
	int state;
	int flags;
	uint32_t timer;
	uint32_t timer2;
	int counter;
	uint32_t last;
	const char *name;
};

struct connection ports[] = {
	{ &Serial, "", -1, ST_LOCAL, SE_DEFAULT, 0, 0, 0, 0, "Dell" },
	{ &Serial0, "", -1, ST_LOCAL, SE_DEFAULT, 0, 0, 0, 0, "Spare" },
	{ &Serial1, "", -1, ST_LOCAL, SE_DEFAULT, 0, 0, 0, 0, "Raspberry Pi" },
};

#define N_PORTS 3

void report(int id, const char *t) {
	if ((ports[id].flags & SE_QUIET) == 0) {
		ports[id].stream->println(t);
	}
}


boolean dialPhone(int id, int dest) {

	if (ports[id].remote >= 0) {
		report(id, "ERROR");
		return true;
	}

	if (dest == id) {
		report(id, "BUSY");
		return true;
	}

	if (dest < 0 || dest >= N_PORTS) {
		report(id, "NO CARRIER");
		return true;
	}
	if (ports[dest].remote > -1) {
		report(id, "BUSY");
		return true;
	}
	ports[dest].remote = id;
	ports[id].remote = dest;
	ports[id].state = ST_CALL;
	ports[dest].stream->println("RING");
	ports[dest].stream->print("NMBR=");
	ports[dest].stream->println(id);
	ports[dest].stream->print("NAME=");
	ports[dest].stream->println(ports[id].name);
	ports[id].timer = millis();
	ports[id].timer2 = millis();

	return true;
}

void parseCommands(int id, char *buf) {

	while (*buf) {
		if (*buf >= 'a' && *buf <= 'z') {
			*buf -= ('a' - 'A');
		}

		if (*buf == 'D') { // Dial - rest of string is the number.
			buf++;
			char type = *buf++;
			if (type == 'L' || type == 'l') {
				dialPhone(id, ports[id].last);
				return;
			}

			if (type == 'P' || type == 'p') {
				ports[id].last = atoi(buf);
				dialPhone(id, ports[id].last);
				return;
			}

			if (type == 'T' || type == 't') {
				ports[id].last = atoi(buf);
				dialPhone(id, ports[id].last);
				return;
			}
			report(id, "ERROR");
			return;
		}

		if (*buf == 'E') {
			buf++;
			if (*buf == '1') { // Do ECHO
				ports[id].flags |= SE_ECHO;
				buf++;
			} else if (*buf == '0') {
				ports[id].flags &= ~SE_ECHO;
				buf++;
			} else {
				ports[id].flags &= ~SE_ECHO;
			}
			continue;
		}

		if (*buf == 'Q') {
			buf++;
			if (*buf == '1') { // Do quiet mode
				ports[id].flags |= SE_QUIET;
				buf++;
			} else if (*buf == '0') {
				ports[id].flags &= ~SE_QUIET;
				buf++;
			} else {
				ports[id].flags &= ~SE_QUIET;
			}
			continue;
		}

		if (*buf == 'V') {
			buf++;
			if (*buf == '1') { // Do verbose
				ports[id].flags |= SE_VERBOSE;
				buf++;
			} else if (*buf == '0') {
				ports[id].flags &= ~SE_VERBOSE;
				buf++;
			} else {
				ports[id].flags &= ~SE_VERBOSE;
			}
			continue;
		}

		if (*buf == 'Z') { // Reset
			buf++;
			if (*buf >= '0' && *buf <= '9') {
				buf++;
			}
			ports[id].flags = SE_DEFAULT;
			continue;
		}

		if (*buf == 'H') { // Hangup
			buf++;
			if (*buf >= '0' && *buf <= '9') {
				buf++;
			}

			if (ports[id].remote >= 0) {
				report(ports[id].remote, "NO CARRIER");
				ports[ports[id].remote].state = ST_LOCAL;
				ports[ports[id].remote].remote = -1;
				ports[id].remote = -1;
				
			}
			continue;
		}


		if (*buf == 'A') { // Answer
			buf++;
			if (*buf >= '0' && *buf <= '9') {
				buf++;
			}

			if (ports[id].remote <= -1) {
				report(id, "ERROR");
				return;
			}
			ports[id].state = ST_ONLINE;
			ports[ports[id].remote].state = ST_ONLINE;
			report(id, "CONNECT 115200");
			report(ports[id].remote, "CONNECT 115200");
			return;
			continue;
		}

		if (*buf == 'I') { // Info
			buf++;
			if (*buf == '0') {
				ports[id].stream->println("PiLink V1.0");
				buf++;
			} else if (*buf == '1') {
				for (int i = 0; i < N_PORTS; i++) {
					ports[id].stream->print(i);
					ports[id].stream->print(": ");
					ports[id].stream->println(ports[i].name);
				}
				buf++;
			} else if (*buf == '2') {
				buf++;
			} else if (*buf == '3') {
				buf++;
			} else if (*buf == '4') {
				buf++;
			} else if (*buf == '5') {
				buf++;
			} else if (*buf == '6') {
				buf++;
			} else if (*buf == '7') {
				buf++;
			} else if (*buf == '8') {
				buf++;
			} else if (*buf == '9') {
				buf++;
			} else {
				ports[id].stream->println("PiLink V1.0");
			}
			continue;
		}

		report(id, "ERROR");
		return;
	}

	report(id, "OK");
}

void setup() {
	Serial.begin(115200);
	Serial1.begin(115200);
}

void loop() {
	for (int i = 0; i < N_PORTS; i++) {
		boolean ok = false;
		switch (ports[i].state) {
			case ST_ONLINE:
				if (millis() - ports[i].timer > 500) {
					if (ports[i].counter == 3) {
						ports[i].state = ST_LOCAL;
						ports[i].counter = 0;
						ports[i].buffer[0] = 0;
						report(i, "OK");
						break;
					}
				}
						
				if (ports[i].stream->available()) {
					int ch = ports[i].stream->read();
					if (ch == '+') {
						if (ports[i].counter == 0) { // first +
							if (millis() - ports[i].timer > 500) { // At least 500ms since last character
								ports[i].counter++;
							} else {
								ports[ports[i].remote].stream->write('+');
							}
						} else { // Not the first
							if (millis() - ports[i].timer < 250) { // No more than 250ms since last +
								ports[i].counter++;
							} else { // Too slow!
								for (int x = 0; x < ports[i].counter; x++) {
									ports[ports[i].remote].stream->write('+');
								}
								ports[i].counter = 0;
							}
						}
					} else {
						for (int x = 0; x < ports[i].counter; x++) {
							ports[ports[i].remote].stream->write('+');
						}
						ports[i].counter = 0;
						ports[ports[i].remote].stream->write(ch);
					}
					ports[i].timer = millis();
						
				}
				break;

			case ST_LOCAL:
				if (ports[i].stream->available()) {
					int ch = ports[i].stream->read();
					if (ports[i].flags & SE_ECHO) {
						ports[i].stream->write(ch);
					}
					switch (ch) {
						case '\n': break; // ignore
						case '\r': 
							if (ports[i].flags & SE_ECHO) {
								ports[i].stream->write('\n');
							}

							if (!strncasecmp(ports[i].buffer, "AT", 2)) {
								parseCommands(i, ports[i].buffer + 2);
							}

							ports[i].buffer[0] = 0;
							ports[i].counter = 0;

							break;
						default:
							if (ports[i].counter < 99) {
								ports[i].buffer[ports[i].counter++] = ch;
								ports[i].buffer[ports[i].counter] = 0;
							}
					}
				}
				break;

			case ST_CALL:
				if (ports[i].stream->available()) {
					ports[i].stream->read();
					ports[ports[i].remote].remote = -1;
					ports[i].remote = -1;
					ports[i].state = ST_LOCAL;
					ports[i].stream->println("NO CARRIER");
					break;
				} 
				if (millis() - ports[i].timer > 2000) {
					ports[i].timer = millis();
					if (ports[i].remote > -1) {
						ports[ports[i].remote].stream->println("RING");
					}
				}
				if (millis() - ports[i].timer2 > 15000) {
					ports[ports[i].remote].remote = -1;
					ports[i].remote = -1;
					ports[i].state = ST_LOCAL;
					ports[i].stream->println("NO CARRIER");
				}
			
				break;
		}
	}
}
