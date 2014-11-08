#include "helpers.h"
#include "simulation_messages.h"

void routeMessage(msg * message, int dest, int time, FILE * log_file, int last_added_node) {
	/* announce the type of the received message */
	printf ("Timp %d, Simulator, msg tip%d de la %d\n", time, message->msg_type, message->sender);

	/* announce routing event */
	if (message->msg_type == M4) {
		fprintf (log_file, "\nRutare timp %d, creator %d, nr_secv %d, sender %d, next_hop %d\n",
				message->time, message->creator, message->seq_no, message->sender, message->next_hop);
	}

	/* conditions for errors */
	if ( (message->next_hop < 0 ) || (message->next_hop > last_added_node) ) {
		printf("Nodul %d nu este (inca) in topologie!", message->next_hop);
		exit(-1);
	}

	/* route message */
	write(dest, message, sizeof(msg));
}

void announceNewEvent(event_t *events, int pipes_fds[KIDS][4], fd_set *read_fds, int *fdmax, int time, int no_events, int *last_added_node) {
	msg message;
	int router1;
	int router2;
	int out;

	/* announce events only on even time stamps */
	if ( ((time+1)%2==0) && (time+1 < 2*no_events) ) {
		message.msg_type = M7;

		sprintf (message.payload, "%s\n", events[(time+1)/2].initial_string);

		router1 = events[(time+1)/2].router1;
		message.add = FALSE; // true only for E1 events

		/* add a node to topology */
		if (events[(time+1)/2].event_type == E1) {
			(*last_added_node)++; // update no of nodes

			// update known file descriptors
			FD_SET(pipes_fds[router1][0], read_fds);
			if ( *fdmax < pipes_fds[router1][0] ) {
				*fdmax = pipes_fds[router1][0];
			}

			/* set message parameters */
			message.add = TRUE;
			message.time = time;
		}

		/* announce router1 */
		out = pipes_fds[router1][3];
		write(out, &message, sizeof(msg));

		/* announce router2, if it exists */
		router2 = events[(time+1)/2].router2;
		if (router2 > -1) {
			out = pipes_fds[router2][3];
			write(out, &message, sizeof(msg));
		}
	}
}

void askForRoutingTables(int pipes_fds[KIDS][4], int last_node) {
	msg message;
	int node;
	int out;

	message.msg_type = M8;
	for (node = 0; node <= last_node; node++) {
		out = pipes_fds[node][3];
		write(out, &message, sizeof(msg));
	}
}

void announceFurtherMessageProcessing(int pipes_fds[KIDS][4], int last_node) {
	msg message;
	int node;
	int out;

	/* tell routers to further process their received messages */
	message.msg_type = M6;
	for (node = 0; node <= last_node; node++) {
		out = pipes_fds[node][3];
		write(out, &message, sizeof(msg));
	}
}

void logRoutingTable(msg * rcv_message, FILE * log_file) {
	int received_routing_table[KIDS][2];
	int k;

	memcpy (&received_routing_table, rcv_message->payload, sizeof(received_routing_table));
	fprintf (log_file, "\n Timp %d, tabela rutare ruter %d\n", rcv_message->time, rcv_message->sender);
	for (k = 0; k < KIDS; k++)
		fprintf (log_file, "%d % 4d % 3d\n", k, received_routing_table[k][0], received_routing_table[k][1]);
}

void endSimulation(int pipes_fds[KIDS][4], FILE* log_file, int last_node) {
	msg message;
	int node;
	int fd_status;
	int out;

	/* announce everybody that the simulation is over */
	message.msg_type = M9;
	/* announce routing instances to close */
	for (node = 0; node < KIDS; node++) {
		out = pipes_fds[node][3];
		write(out, &message, sizeof(msg));
	}

	/* routing table logging */
	for (node = 0; node <= last_node; node++) {
		int in = pipes_fds[node][0];

		/* wait for routing table message */
		do {
			fd_status = read (in, &message, sizeof(msg));
			/* check status of the pipe used to receive messages from node */
			DIE(fd_status < 0, "citire message simulator" );
		} while (message.msg_type != M10);

		logRoutingTable(&message, log_file);
	}
}
