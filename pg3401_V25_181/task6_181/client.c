#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdint.h>

#define BUF_SIZE 4096 // Size of buffer for receiving data
#define PROBE_TIMEOUT_SECS 20 // Seconds to wait for protocol response
#define DELTA 0x9E3779B9 // TEA algorithm constant (taken from tea.c task4)
#define ROUNDS 32 // Number of TEA rounds per block

// Print program usage and exit
static int usage(const char *prog)
{
	fprintf(stderr, "Usage: %s -port <port>\n", prog);
	return 1;
}

// Decrypt a single 64-bit block using TEA
static void tea_decrypt_block(uint32_t v[2], const uint32_t k[4])
{
	uint32_t v0 = v[0];
	uint32_t v1 = v[1];
	uint32_t sum = DELTA * ROUNDS;
	int i;

	for (i = 0; i < ROUNDS; i++) {
		v1 -= ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
		v0 -= ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
		sum -= DELTA;
	}
	v[0] = v0;
	v[1] = v1;
}

// Remove PKCS#5 padding; return new length or 0 if invalid
static size_t strip_pkcs5(uint8_t *buf, size_t len)
{
	uint8_t p;
	size_t i;

	if (len == 0)
		return 0;
	p = buf[len - 1];
	if (p < 1 || p > 8 || p > len)
		return 0;
	for (i = 0; i < p; i++) {
		if (buf[len - 1 - i] != p)
			return 0;
	}
	return len - p;
}

// Check if buffer contains only printable ASCII or whitespace
static int valid_ascii(const uint8_t *buf, size_t len)
{
	size_t i;
	uint8_t c;
	for (i = 0; i < len; i++) {
		c = buf[i];
		if (c == '\n' || c == '\r' || c == '\t')
			continue;
		if (c < 0x20 || c > 0x7E)
			return 0;
	}
	return 1;
}

int main(int argc, char *argv[])
{
	const char *server = "127.0.0.1";
	const char *port = NULL;
	int sockFd, best_key, b;
	struct sockaddr_in saAddr;
	struct timeval tv;
	size_t total, cap, i ,j, plen, best_len, clen;
	ssize_t n;
	uint8_t *plain, *best_plain, *data, tmp[BUF_SIZE];
	long off, best_off;
	uint32_t kwords[4], w, vtemp[2];
	char outname[64];
	FILE *fout;

	// parse command-line args
	if (argc != 3 || strcmp(argv[1], "-port") != 0) {
		return usage(argv[0]);
	}
	port = argv[2];

	// open socket
	sockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockFd < 0) {
		perror("FAILED: socket");
		return 1;
	}

	// fill server address struct
	memset(&saAddr, 0, sizeof(saAddr));
	saAddr.sin_family = AF_INET;
	saAddr.sin_port   = htons(atoi(port));
    // connect to server
	if (connect(sockFd, (struct sockaddr *)&saAddr, sizeof(saAddr)) < 0) {
		perror("FAILED: connect");
		close(sockFd);
		return 1;
	}
	fprintf(stderr, "→ Connected via TCP to %s:%s\n", server, port);

	// set recv timeout
	tv.tv_sec  = PROBE_TIMEOUT_SECS;
	tv.tv_usec = 0;

	// read all data into buffer
	data = NULL;
	total = 0;
	cap   = 0;
	while ((n = recv(sockFd, tmp, BUF_SIZE, 0)) > 0) {
		// expand buffer if needed
		if ((size_t)total + n > cap) {
			size_t oldcap = cap;
			size_t newcap = cap ? cap * 2 : BUF_SIZE;
			while (newcap < total + n)
				newcap *= 2;
			{
                uint8_t *newdata = realloc(data, newcap);
				if (!newdata) {
					perror("FAILED: realloc");
					free(data);
					close(sockFd);
					return 1;
				}
				memset(newdata + oldcap, 0, newcap - oldcap);
				data = newdata;
				cap  = newcap;
			}
		}
		memcpy(data + total, tmp, n);
		total += n;
	}
	// On recv() error, ignore "no data" (EAGAIN/EWOULDBLOCK) and treat others as fatal
	if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		perror("recv");
		close(sockFd);
		free(data);
		return 1;
	}
	close(sockFd);

	// if no data free data and exit
	if (total == 0) {
		fprintf(stderr, "No data received after %d seconds\n", PROBE_TIMEOUT_SECS);
		free(data);
		return 1;
	}
	fprintf(stderr, "Received %zu bytes\n", total);

	// prepare for brute-force
	plain = malloc(total);
	if (!plain) {
		perror("malloc");
		free(data);
		return 1;
	}
	memset(plain, 0, total);

	best_len   = 0;
	best_key   = 0;
	best_off   = 0;
	best_plain = NULL;

	// try every block-aligned offset
	for (off = 0; off <= (long)total - 8; off++) {
		clen = total - off;
		if (clen % 8)
			continue;

		// try all single-byte keys repeated
		for (b = 0; b < 256; b++) {
			w = (uint8_t)b;
			//This will shift the bytes and place and put in operation OR 
			//the number 8 and 16 is how many bits
			//if w = 0x000000BB -> 0x0000BB00 -> 0x0000BBBB
			w |= w << 8;
			//0x0000BBBB -> 0xBBBB0000 -> 0xBBBBBBB
			w |= w << 16;
			for (i = 0; i < 4; i++) {
				kwords[i] = w;
			}
			// decrypt each 8-byte block
			for (j = 0; j < clen; j += 8) {
				memcpy(vtemp, data + off + j, 8);
				tea_decrypt_block(vtemp, kwords);
				memcpy(plain + j, vtemp, 8);
			}
			// remove padding and check ASCII
			plen = strip_pkcs5(plain, clen);
			if (!plen)
				continue;
			if (!valid_ascii(plain, plen))
				continue;

			// track best (longest) plaintext
			if (plen > best_len) {
				best_len   = plen;
				best_key   = b;
				best_off   = off;
				free(best_plain);
				best_plain = malloc(plen);
				if (!best_plain) {
					perror("FAILED: malloc");
					free(data);
					free(plain);
					return 1;
				}
				// zero it in case it's larger than plen
				memset(best_plain, 0, plen);
				memcpy(best_plain, plain, plen);
			}
		}
	}

    // no valid decryption
	if (best_len == 0) {
		fprintf(stderr, "No valid plaintext found\n");
		free(data);
		free(plain);
		return 1;
	}

	// write result to file
	snprintf(outname, sizeof outname, "decrypted_0x%02x_offset_%ld.txt",
			best_key, best_off);
	fout = fopen(outname, "w");
	if (!fout) {
		perror("FAILED: fopen");
		free(data);
		free(plain);
		free(best_plain);
		return 1;
	}
	fwrite(best_plain, 1, best_len, fout);
	fclose(fout);

	fprintf(stderr, "Decrypted with key=0x%02x offset=%ld length=%zu → %s\n",
			best_key, best_off, best_len, outname);

	// Free all data
	free(data);
	free(plain);
	free(best_plain);
	return 0;
}