#ifndef __HELPERS_H__
#define __HELPERS_H__ 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

/* Dimensiunea maxima a calupului de date */
#define BUFLEN 1600
#define MAX_CLIENTS 10

#define COMMAND_LEN 5

#define INT 0
#define INT_OFFSET 52

#define SHORT_REAL 1
#define SHORT_REAL_OFFSET 51

#define FLOAT 2
#define FLOAT_OFFSET 52
#define POWER10_OFFSET 56

#define STRING 3

#define TYPE_OFFSET 50
#define SIGN_OFFSET 51

#endif // __HELPERS_H__
