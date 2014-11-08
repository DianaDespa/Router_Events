#ifndef __SIMULATION_MESSAGES_H_
#define __SIMULATION_MESSAGES_H_

void routeMessage(msg * message, int dest, int time, 
	FILE * log_file, int last_added_node);

void announceNewEvent(event_t *events, int pipes_fds[KIDS][4],
	fd_set *read_fds, int *fdmax, int time, int no_events, int *last_added_node);

void askForRoutingTables(int pipes_fds[KIDS][4], int last_node);

void announceFurtherMessageProcessing(int pipes_fds[KIDS][4], int last_node);

void logRoutingTable(msg * rcv_message, FILE * log_file);

void endSimulation(int pipes_fds[KIDS][4], FILE* log_file, int last_node);


#endif 
