#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "tea.h"
#include "dbj2.h"
#include "task4_threads.h"

#define BLOCK_SIZE 8
#define OUTPUT_FILE "task4_pg2265.enc"

// Producer thread: read chunks into buffer
void* thread_producer(void *arg) {
	ctx_t *ctx = (ctx_t*)arg;
	FILE *fp;
	size_t got;

	fp = fopen(ctx->infile, "rb");
	if (!fp) { 
		perror("FAILED:fopen input"); 
		exit(1); 
    }

	while (true) {
		// wait for buffer empty
		sem_wait(&ctx->sem_empty);
		// lock buffer to gain exclusive access
		sem_wait(&ctx->sem_mutex);
        
		//Read binary input file saves to buffer
		got = fread(ctx->buffer, 1, BUFFER_SIZE, fp);
		ctx->bytes_in_buffer = (int)got;
		//Last chunk will mark the producer TRUE
		ctx->producer_done = (got < BUFFER_SIZE);
        
		// unlock buffer
		sem_post(&ctx->sem_mutex);
		// signal data ready
		sem_post(&ctx->sem_full);
        
		if (got < BUFFER_SIZE) {
			// last chunk produced
			break;                          
		}
	}

	fclose(fp);
	return NULL;
}

//Consumer thread: pad, encrypt and write
void* thread_consumer(void *arg) {
	ctx_t *ctx = (ctx_t*)arg;
	uint32_t v[2], w[2];

	while (true) {
		bool done;
		unsigned char local_buf[BUFFER_SIZE];
		int pad_len, total, offs, data_bytes, pad_start, pad_count, n;
		unsigned char block[BLOCK_SIZE];

		// wait for data and lock buffer
		sem_wait(&ctx->sem_full);
		sem_wait(&ctx->sem_mutex);

		n = ctx->bytes_in_buffer;
		done = ctx->producer_done;
		// copy the data out
		memcpy(local_buf, ctx->buffer, n);

		// unlock for producer
		sem_post(&ctx->sem_mutex);

		//compute PKCS#5 padding for the last chunk
		pad_len = 0;
		if (done) {
			pad_len = BLOCK_SIZE - (n % BLOCK_SIZE);
			if (pad_len == 0) pad_len = BLOCK_SIZE;
		}

		// total bytes we need to encrypt included with padding
		total = n + pad_len;

		// process each 8-byte block
		for (offs = 0; offs < total; offs += BLOCK_SIZE) {
			// determining real data bytes in this block
			if ((offs + BLOCK_SIZE) <= n) {
				data_bytes = BLOCK_SIZE;
			} else if (offs < n) {
				data_bytes = n - offs;
			} else {
				data_bytes = 0;
			}

			// copy data
			if (data_bytes > 0) {
				memcpy(block, local_buf + offs, data_bytes);
			}
			// pad remainder
			pad_start = data_bytes;
			if ((offs + BLOCK_SIZE) > n) {
				if (offs < n) {
					//Final partial block only pads the missing final pad_len
					pad_count = pad_len;
				} else {
					//pad the whole block
					pad_count = BLOCK_SIZE;
				}
			} else {
				pad_count = 0;
			}
			if (pad_count > 0) {
				memset(block + pad_start, pad_len, pad_count);
			}

			memcpy(v, block, BLOCK_SIZE);
			//Calls the function provided by EWA
			encipher(v, w, ctx->key);
			fwrite(w, 1, BLOCK_SIZE, ctx->outfp);
		}

		// signal buffer empty
		sem_post(&ctx->sem_empty);

		if (done) {
			break;
		}
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
		return 1;
	}

	const uint32_t tea_key[4] = {
		0x01234567,
		0x89ABCDEF,
		0xFEDCBA98,
		0x76543210
	};
	ctx_t ctx;
	pthread_t prod, cons;

	// set input file from command line
	ctx.infile = argv[1];

	// open output file
	ctx.outfp = fopen(OUTPUT_FILE, "wb");
	if (!ctx.outfp) { 
		perror("Failed to open output file"); 
		return 1; 
	}

	// initialize other context fields
	ctx.bytes_in_buffer = 0;
	ctx.producer_done   = false;
	ctx.key             = tea_key;

	// initialize semaphores
	if (sem_init(&ctx.sem_empty, 0, 1) != 0) {
		perror("sem_init(sem_empty)");
		return 1;
	}
	if (sem_init(&ctx.sem_full, 0, 0) != 0) {
		perror("sem_init(sem_full)");
		return 1;
	}
	if (sem_init(&ctx.sem_mutex, 0, 1) != 0) {
		perror("sem_init(sem_mutex)");
		return 1;
	}

	// create threads
	if (pthread_create(&prod, NULL, thread_producer, &ctx) != 0 ) {
		perror("Failed to create producer thread"); 
		return 1;
	}
	if (pthread_create(&cons, NULL, thread_consumer, &ctx) != 0) {
		perror("Failed to create consumer thread");
		return 1;
	}

	// wait for threads
	pthread_join(prod, NULL);
	pthread_join(cons, NULL);
	fclose(ctx.outfp);

	// destroy semaphores
	sem_destroy(&ctx.sem_empty);
	sem_destroy(&ctx.sem_full);
	sem_destroy(&ctx.sem_mutex);

	//compute DJB2 hash
	{
		FILE *hf, *outHash;
		int hash;

		hf = fopen(OUTPUT_FILE, "rb");
		if (!hf) { 
			perror("Failed to open output for hashing"); 
			return 1; 
		}
		Task2_SimpleDjb2Hash(hf, &hash);
		fclose(hf);

		outHash = fopen("task4_pg2265.hash", "w");
		if (!outHash) { 
			perror("Failed to open hash file"); 
			return 1; 
		}
		fprintf(outHash, "%d\n", hash);
		fclose(outHash);
	}

	printf("Encrypted file written to %s\n", OUTPUT_FILE);
	printf("Hash written to task4_pg2265.hash\n");
	return 1;
}
