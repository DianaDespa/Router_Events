all: ruter sim

sim: simulator.c helpers.h
	gcc -g -Wall -o sim simulator.c
	
	
ruter: ruter.cpp helpers.h
	g++ -g -Wall -o ruter ruter.cpp
	

run: sim ruter events.in
	./sim events.in
	
	
clean:
	rm -f ruter sim log_rutare
	
	

