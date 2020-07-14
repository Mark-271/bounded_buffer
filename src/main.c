/**
 *
 * @file
 * Realization of producer-consumer problem based on usage of multiple threads
 * who shear common buffer used as a circular queue.
 *
 * Program operates with two threads and a fixed-size buffer.
 * Thread #0 writes user's data  line by line from a file or stdin flow
 * into shared fixed-size  buffer.
 * Thread #1 reads data from buffer to stdout flow.
 *
 */

#include <file.h>
#include <tools.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define L_SIZE 5
#define R_SIZE 80

#define TH_NUM 2

typedef void *(*thread_func_t)(void *);

typedef struct shared_buffer {
	char buf[L_SIZE][R_SIZE];
	unsigned produce_c;
	unsigned consume_c;
	bool flag;
} sb_t;

static pthread_mutex_t lock;
static pthread_mutex_t condlock;
static pthread_t th_id[TH_NUM];
static pthread_cond_t cond_id[TH_NUM];

static sb_t sb;
FILE *f;

/**
 *
 * Functions being handled by threads.
 * Two checks are used to solve synchronization problem:
 * 1. "What if shared_ buffer is full" (inside thread0_func);
 * 2. "What if shared_buffer is empty" (inside thread1_func);
 *
 */

static void *thread0_func(void *data)	/*  Push data into array */
{
	UNUSED(data);

	char *line = NULL;
        size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, f)) != -1) {
		if ((sb.produce_c - sb.consume_c) == L_SIZE) {
			pthread_mutex_lock(&condlock);
			pthread_cond_wait(&cond_id[0], &condlock);
			pthread_mutex_unlock(&condlock);
		}

		pthread_mutex_lock(&lock);
		strcpy(sb.buf[sb.produce_c % L_SIZE], line);
		++sb.produce_c;
		pthread_mutex_unlock(&lock);

		pthread_mutex_lock(&condlock);
		pthread_cond_signal(&cond_id[1]);
		pthread_mutex_unlock(&condlock);
	}

	sb.flag = true;

	free(line);

	return NULL;
}

static void *thread1_func(void *data)	/* Pop data from array */
{
	UNUSED(data);
	for (;;) {
		if ((sb.produce_c - sb.consume_c) == 0) {
			if (sb.flag)
				break;
			else {
				pthread_mutex_lock(&condlock);
				pthread_cond_wait(&cond_id[1], &condlock);
				pthread_mutex_unlock(&condlock);
			}
		}
		pthread_mutex_lock(&lock);
		printf("%s", sb.buf[sb.consume_c % L_SIZE]);
		++sb.consume_c;
		pthread_mutex_unlock(&lock);

		msleep(10);

		pthread_mutex_lock(&condlock);
		pthread_cond_signal(&cond_id[0]);
		pthread_mutex_unlock(&condlock);
	}

	return NULL;
}

static thread_func_t thread_func_handler[TH_NUM] = {
	thread0_func,
	thread1_func,
};

/* ------------ Parse and validate user params ------------------------- */
struct param {
	const char *path;
};

void print_usage(const char *app)
{
	printf("Usage: %s [file]\n", app);
}

static bool parse_args(int argc, char *argv[], struct param *p)
{
	if (argc < 1 || argc > 2) {
		fprintf(stderr, "Error; Invalid argument count\n");
		print_usage(argv[0]);
		return false;
	}

	/* Parse file path */
	if (argc == 2) {
		p->path = argv[1];
		if(!file_exist(p->path)) {
			fprintf(stderr, "Error: File %s doesn't exist\n",
				p->path);
			return false;
		}
	} else {
		p->path = NULL;
	}

	return true;
}
/* --------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	bool res;
	struct param p;

	res = parse_args(argc, argv, &p);
	if (!res)
		return EXIT_FAILURE;

	if(p.path) {
		f = fopen(p.path, "r");
		if (!f) {
			perror("Error: Unable to open file");
			return EXIT_FAILURE;
		}
	} else
		f = stdin;

	size_t i;
	int err, ret = EXIT_SUCCESS;

	pthread_mutex_init(&lock, NULL);
	pthread_mutex_init(&condlock, NULL);

	for (i = 0; i < TH_NUM - 1; ++i)
		pthread_cond_init(&cond_id[i], NULL);

	for (i = 0; i < TH_NUM; ++i) {
		err = pthread_create(&th_id[i], NULL,
				     thread_func_handler[i], NULL);
		if (err) {
			perror("Warning: Error in pthread_create");
			ret = EXIT_FAILURE;
			goto err;
		}
	}

	for (i = 0; i < TH_NUM; ++i) {
		err = pthread_join(th_id[i], NULL);
		if (err) {
			perror("Warning: Error in pthread_join");
			ret = EXIT_FAILURE;
		}
	}

err:
	for(i = 0; i < TH_NUM; ++i)
		pthread_cond_destroy(&cond_id[TH_NUM - 1 - i]);
	pthread_mutex_destroy(&condlock);
	pthread_mutex_destroy(&lock);
	return ret;
}
