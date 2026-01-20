#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "ewpdef.h"

#define END_MARKER "\r\n.\r\n"
#define END_MARKER_LEN 5

//Ensures that server recieves all data. Avoids data to be in socket
static int recvAll(int sock, void *buf, size_t len) {
	size_t total;
	char *p;
	ssize_t n;
	total = 0;
	p = buf;
	while (total < len) {
		//Store the whole socket into buffer
		n = recv(sock, p + total, len - total, 0);
		if (n < 0) {
			perror("Failed: RecvAll");
			return -1;
		}
		if (n == 0) {
			// peer closed
			return -1;
		}
		total += n;
	}
	return 0;
}

static int sendReply(int sock, const char *code, const char *msg) {
	//Building SERVERREPLY struct
	struct EWA_EXAM25_TASK5_PROTOCOL_SERVERREPLY r;
	ssize_t sent;

	memcpy(r.stHead.acMagicNumber, EWA_EXAM25_TASK5_PROTOCOL_MAGIC, 3);
	snprintf(r.stHead.acDataSize, 5, "%04lu", (unsigned long)sizeof(r));
	r.stHead.acDelimeter[0] = '|';
	memcpy(r.acStatusCode, code, 3);
	r.acHardSpace[0] = ' ';
	snprintf(r.acFormattedString, sizeof(r.acFormattedString), "%s", msg);
	r.acHardZero[0] = '\0';

	sent = send(sock, &r, sizeof(r), 0);
	return (sent == sizeof(r)) ? 0 : -1;
}

static int validFilename(const char *name) {
	const char *p;
	//If the file does not exist or NULL
	if (!name || !*name) return 0;
	for (p = name; *p; ++p) {
		//If the file contains seperator or non-printable chars
		if (*p=='/'||*p=='\\' || (unsigned char)*p < 32 || (unsigned char)*p > 126)
		return 0;
	}
	return 1;
}

static void handleClient(int csock) {
	struct EWA_EXAM25_TASK5_PROTOCOL_SERVERACCEPT a;
	struct EWA_EXAM25_TASK5_PROTOCOL_CLIENTHELO ch;
	struct EWA_EXAM25_TASK5_PROTOCOL_SERVERHELO sh;
	struct EWA_EXAM25_TASK5_PROTOCOL_SIZEHEADER h;
	struct EWA_EXAM25_TASK5_PROTOCOL_SIZEHEADER dh;
	time_t now;
	struct tm *tm;
	char ts[64], buf[51], user[51], cip[51];
	char *dot, *payload, *cmd, *fname, *data;
	int len, payload_len, i, dlen, found;
	size_t cmd_len;

	// 1. SERVER ACCEPT 220
	{
		//Building SERVERACCEPT struct 
		memcpy(a.stHead.acMagicNumber, EWA_EXAM25_TASK5_PROTOCOL_MAGIC, 3);
		snprintf(a.stHead.acDataSize, 5, "%04lu", (unsigned long)sizeof(a));
		a.stHead.acDelimeter[0] = '|';
		memcpy(a.acStatusCode, "220", 3);
		a.acHardSpace[0] = ' ';

		//Time function taken from time.h this allows to send current time and date
		now = time(NULL);
		tm = localtime(&now);
		ts[0] = '\0';
		strftime(ts, sizeof(ts), "%d %b %Y, %H:%M:%S", tm);

		snprintf(a.acFormattedString, sizeof(a.acFormattedString),
				"127.0.0.1 SMTP MyServer %s", ts);
		a.acHardZero[0] = '\0';

		send(csock, &a, sizeof(a), 0);
	}

	// 2. HELO → 250 CLIENTIP Hello USER
	{
		if (recvAll(csock, &ch, sizeof(ch)) < 0) return;

		// parse USER.CLIENTIP
		strncpy(buf, ch.acFormattedString, sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		dot = strchr(buf, '.');
		user[0] = '\0';
		cip[0] = '\0';
		if (dot) {
			*dot = 0;
			strcpy(user, buf);
			strcpy(cip, dot+1);
		}
		//Building server HELO protocol struct send a 250 code
		memcpy(sh.stHead.acMagicNumber, EWA_EXAM25_TASK5_PROTOCOL_MAGIC, 3);
		snprintf(sh.stHead.acDataSize, 5, "%04lu", (unsigned long)sizeof(sh));
		sh.stHead.acDelimeter[0] = '|';
		memcpy(sh.acStatusCode, "250", 3);
		sh.acHardSpace[0] = ' ';
		snprintf(sh.acFormattedString, sizeof(sh.acFormattedString),
				"%s Hello %s", cip, user);
		sh.acHardZero[0] = '\0';

		//Send to client
		send(csock, &sh, sizeof(sh), 0);
	}

    // 3) MAIL FROM & RCPT TO → each 250
    for (i = 0; i < 2; ++i) {
        if (recvAll(csock, &h, sizeof(h)) < 0) return;
        len = atoi(h.acDataSize);
        if (len <= (int)sizeof(h)) return;

		//Determine the length of incoming command store it in memory
        payload_len = len - sizeof(h);
        payload = malloc(payload_len);
        memset(payload, 0, payload_len);
        recvAll(csock, payload, payload_len);
        free(payload);

        //Reply to client
        sendReply(csock, "250", "OK");
    }

	    // 4) DATA/QUIT loop
    while (1) {
        if (recvAll(csock, &h, sizeof(h)) < 0)
            break;
        len = atoi(h.acDataSize);
        if (len <= (int)sizeof(h))
            break;

        cmd_len = len - sizeof(h);
        cmd = malloc(cmd_len + 1);
        memset(cmd, 0, cmd_len + 1);
        if (recvAll(csock, cmd, cmd_len) < 0) {
            free(cmd);
            break;
        }
        cmd[cmd_len] = '\0';

        // Compare if client sends DATA command
        if (strncmp(cmd, "DATA ", 5) == 0) {
            fname = cmd + 5;
            if (!validFilename(fname)) {
                sendReply(csock, "501", "Invalid filename");
                free(cmd);
				//back to the top of the while loop
                continue;      
            }

            // Open the file for writing
            FILE *fp = fopen(fname, "wb");
            if (!fp) {
                sendReply(csock, "550", "Cannot open file");
                free(cmd);
				//still inside the while loop
                continue;      
            }

            sendReply(csock, "354", "Ready for message");

            // Discard all data frames until END_MARKER
            while (1) {
                if (recvAll(csock, &dh, sizeof(dh)) < 0) break;
                dlen = atoi(dh.acDataSize) - sizeof(dh);
                if (dlen <= 0) break;

                data = malloc(dlen);
                memset(data, 0, dlen);
                if (recvAll(csock, data, dlen) < 0) {
                    free(data);
                    break;
                }

                // check for end marker
                found = 0;
                for (i = 0; i + END_MARKER_LEN <= dlen; ++i) {
                    if (memcmp(data + i, END_MARKER, END_MARKER_LEN) == 0) {
                        found = 1;
                        break;
                    }
                }

                if (found) {
                    // write up to but not including the END_MARKER
                    fwrite(data, 1, i, fp);
                    free(data);
                    break;
                }

                // no marker: write the entire block
                fwrite(data, 1, dlen, fp);
                free(data);
            }

            // close the file and acknowledge
            fclose(fp);
            sendReply(csock, "250", "Message accepted");

            free(cmd);
            continue; 
        }
        else if (strncmp(cmd, "QUIT", 4) == 0) {
            sendReply(csock, "221", "Service closing transmission channel");
            free(cmd);
            close(csock);
            exit(0);
        }
        else {
            sendReply(csock, "500", "Unrecognized command");
        }

        free(cmd);
    }  // end of while(1)

    close(csock);
}



int main(int argc, char *argv[]) {
	int port, sock, opt, cs;
	const char *serverId;
	struct sockaddr_in addr;
	struct sockaddr_in cli;
	socklen_t cliLen;

	if (argc != 5 || strcmp(argv[1], "-port") || strcmp(argv[3], "-id")) {
		fprintf(stderr, "Usage: %s -port <port> -id <ServerName>\n", argv[0]);
		return 1;
	}
	//Variables from the terminal(user)
	port = atoi(argv[2]);
	serverId = argv[4];

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) { 
		perror("Failed to create socket."); 
		return 1; 
	}
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("FAILED: bind"); 
		return 1;
	}
	if (listen(sock, 5) < 0) {
		perror("FAILED: listen"); 
		return 1;
	}

	printf("Server %s listening on port %d\n", serverId, port);
	while (1) {
		cliLen = sizeof(cli);
		cs = accept(sock, (struct sockaddr*)&cli, &cliLen);
		if (cs < 0) {
			perror("Failed to accept");
			continue;
		}
		handleClient(cs);
	}

	close(sock);
	return 0;
}
