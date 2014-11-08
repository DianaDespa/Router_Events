#ifndef __IO_LIB_H_
#define __IO_LIB_H_

event_t * readEventsFromFile(char* filename, int * no_events);
int createRouterInstances(pid_t routers[KIDS], int pipes_fds[KIDS][4]); 

#endif