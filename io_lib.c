#include "helpers.h"
#include "io_lib.h"

event_t * readEventsFromFile(char* filename, int * no_events) {
	FILE *events_file;
	event_t * event_queue;
	int event_type;
	int i;

	/* open file */
	events_file = fopen(filename,"rt");
	DIE (events_file == NULL, "fopen fisier intrare" );

	/* read number of events and allocate space for event queue */
	fscanf(events_file, "%d ", no_events);
	event_queue = (event_t *) malloc((*no_events) * sizeof (event_t)) ;

	//salvez evenimentele din fisierul de intrare intr-un vector; la momentul potrivit, il voi trimite la nodul/nodurile direct implicate
	for (i = 0; i < *no_events; i++) {
		/* read line */
		fgets(event_queue[i].initial_string, LINE_SIZE-1, events_file);

		/* process line */
		// event_type, router1, router2
		sscanf (event_queue[i].initial_string, "%d %d %d", &event_type, &event_queue[i].router1, &event_queue[i].router2);

		if ((event_type == E1) || (event_type == E4)) {
			event_queue[i].router2 = -1; /* invalid second router ID */
		}

		event_queue[i].event_type = event_type;
	}

	fclose(events_file);

	return event_queue;
}

int createRouterInstances(pid_t routers[KIDS], int pipes_fds[KIDS][4]) {
	/* data to be sent to new router processes */
	char write_pipe[BUFSIZE];
	char read_pipe[BUFSIZE];
	char nodeID[BUFSIZE];

	// flag to avoid  'if pid == 0' repetition
	int child = FALSE;

	// each child needs to know what pipe to use
	int  routerID = -1;

	int k;

	for(k=0; k<KIDS; k++) {

		if (!child) {

			DIE (pipe(pipes_fds[k]) < 0, "creare pereche 1 pipe-uri" );  //server citeste, client scrie
			DIE (pipe(&(pipes_fds[k][2])) < 0, "creare pereche 2 pipe-uri" );  //server scrie, client citeste

			routers[k] = fork();
			DIE (routers[k] < 0, "fork");

			if (routers[k]==0) {   //CHILD

				child = TRUE;
				routerID = k;

				DIE (close(pipes_fds[routerID][0]) < 0, "close pipe1 child" ); //inchid capetele nefolosite ale pipe-urilor
				DIE (close(pipes_fds[routerID][3]) < 0, "close pipe2 child" );

				memset(write_pipe, 0, BUFSIZE);
				sprintf (write_pipe, "%d", pipes_fds[routerID][1]);
				memset(read_pipe, 0, BUFSIZE);
				sprintf (read_pipe, "%d", pipes_fds[routerID][2]);
				memset(nodeID, 0, BUFSIZE);
				sprintf (nodeID, "%d", routerID);

				//trimit proceselor fiu ca argumente pipe de scriere, pipe de citire si indicele nodului
				execl("./ruter","ruter", write_pipe, read_pipe, nodeID, NULL);

				fprintf(stderr, "Execution failed for client %s %s %s\n",  write_pipe, read_pipe, nodeID);
				fflush(stdout);
				exit(EXIT_FAILURE);
			}

			else {  //PARENT -> pid [k] > 0

				DIE (close(pipes_fds[k][1]) < 0, "close pipe1 parent" ); //inchid capetele nefolosite ale pipe-urilor
				DIE (close(pipes_fds[k][2]) < 0, "close pipe2 parent" );
			}
		} //if
	}//for
	// this is outside the for loop

	return child;
}
