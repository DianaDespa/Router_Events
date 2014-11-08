#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>


//NU AVETI VOIE SA MODIFICATI acest fisier!

// Macro de verificare a erorilor

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)
	

#define BUFSIZE 1024
#define KIDS	10
#define FALSE 0
#define TRUE 1
#define LINE_SIZE 100
#define DRUMAX 999

/* Message Types */
#define M1 1
#define M2 2
#define M3 3
#define M4 4
#define M5 5
#define M6 6
#define M7 7
#define M8 8
#define M9 9
#define M10 10

/* Event Types */
#define E1 1
#define E2 2
#define E3 3
#define E4 4

/* other constants */
#define CHILD_DONE_AND_CLOSED_CONNECTION 0


typedef struct {
	int msg_type;
	int creator; // id-ul ruterului care a creat mesajul
	int seq_no;  // numar de secventa; id unic al mesajului in cadrul ruterului creator
	int sender;  // previous_hop
	int next_hop;
	int time;
	int add;  // e TRUE daca mesajul e de tip eveniment de adaugare link
	int length;
	char payload[1400];
} msg;

/* used to read/process events from file */
typedef struct {
	/* > router1 =
	 * >> for E1: ID of new added router
	 * >> for E2: one end (router ID) of the new link that will be added
	 * >> for E3: one end (router ID) of the old link that will be removed
	 * >> for E4: ID of source router */
	int router1;
	/* > router2 =
	 * >> for E1: invalid
	 * >> for E2: one end (router ID) of the new link that will be added
	 * >> for E3: one end (router ID) of the old link that will be removed
	 * >> for E4: invalid */
	int router2;
	int event_type; /* {E1, E2, E3, E4}*/
	char initial_string[LINE_SIZE]; /* original string */
} event_t;


#endif

