/* 
 * Despa Diana Alexandra 321CA
 * Tema 2 Protocoale de comunicatii
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <vector>
#include <queue>
#include <sstream>
#include "helpers.h"

int out, in, nod_id;
int timp = -1;
int gata = FALSE;
int secventa = 0; // numar de secventa pentru mesaje

int tab_rutare [KIDS][2];

// Structura payload particulara
typedef struct {
	int vecini_costuri[KIDS]; // vectori de costuri pentru vecini
			// vecini_costuri[i] == 0 daca ruterul nu il are ca vecin pe i
	int dest; // destinatie pentru mesajele M4
	int timp_creare; 
} my_payload;

// Structuri de date
std::vector<msg> lsad(KIDS); // LSADatabase
int	topologie[KIDS][KIDS];
std::queue<msg> q_msg; // Coada de mesaje neprocesate
int vecini_costuri[KIDS];

// Functie care intorce indicele elementului minim din vectorul dist[], in
// functie de pozitiile disponibile date de vectorul availb[]
int minDistance(int dist[], bool availb[])
{
	int min = DRUMAX, min_index;
	for (int v = 0; v < KIDS; v++)
		if (availb[v] == false && dist[v] < min) {
			min = dist[v];
			min_index = v;
		}
	return min_index;
}

// Functie care foloseste algoritmul lui Dijkstra pentru a calcula tabela de rutare
void calcul_tabela_rutare() {
	int dist[KIDS]; // vector de muchii

	bool availb[KIDS]; // pozitiile disponibile din vectorul de muchii
	int prev[KIDS]; // vector de predecesori

	for (int i = 0; i < KIDS; ++i) {
		dist[i] = DRUMAX;
		availb[i] = false;
		prev[i] = -1;
	}
	dist[nod_id] = 0;
	tab_rutare[nod_id][0] = 0;
	
	for (int i = 0; i < KIDS - 1; ++i) {
		int u = minDistance(dist, availb);
		availb[u] = true;
		for (int v = 0; v < KIDS; ++v) {
			if (!availb[v] && topologie[u][v] && (topologie[u][v] < DRUMAX) 
				&& dist[u] != DRUMAX && dist[u] + topologie[u][v] < dist[v]) {
				dist[v] = dist[u] + topologie[u][v];
				prev[v] = u;
				tab_rutare[v][0] = dist[v];
			}
		}
	}
	
	// Aflu next_hop "mergand inapoi" in vectorul de predecesori pana ajung la sursa.
	for (int i = 0; i < KIDS; i++) {
		if (prev[i] == nod_id)
			tab_rutare[i][1] = i; 
		else {
			int u = prev[i];
			while (prev[i] != -1 && prev[u] != nod_id) {
				u = prev[u];
			}
			tab_rutare[i][1] = u;
		}
	}
}

void procesare_eveniment(msg mevent) {
	// Fac citirea din payload printr-un stringstream
	std::stringstream ss(mevent.payload);
	msg request, lsa, new_msg;
	my_payload req_payload, new_payload;
	int tip_even;
	
	ss >> tip_even;
	
	if (mevent.add == TRUE) {
		printf ("Nod %d, msg tip eveniment - am aderat la topologie la pasul %d\n", nod_id, timp);
		// eveniment tip E1: ruterul nou adaugat la retea
		
		int r_id, nr_vec, vecin, cost; 
		ss >> r_id >> nr_vec;
	
		if (nod_id == r_id) {
		// Construiesc mesajul de tip M2
			request.msg_type = M2;
			request.creator = nod_id;
			request.sender = nod_id;
			request.seq_no = ++secventa;
			request.time = timp;
			req_payload.timp_creare = timp;
			
			// Trimit tuturor vecinilor.
			for (int i = 0; i < nr_vec; ++i) {
				ss >> vecin >> cost;
				printf("VECIN: %d COST %d\n", vecin, cost);
				vecini_costuri[vecin] = cost;
				req_payload.vecini_costuri[vecin] = cost;
				memcpy(&request.payload, &req_payload, sizeof(my_payload));
				request.next_hop = vecin;
				write(out, &request, sizeof(msg));
			}
		}
	} else {
		printf ("Timp %d, Nod %d, procesare eveniment\n", timp, nod_id);
		
		int r1, r2, cost;
		
		switch (tip_even) {	
		case E2:
		// eveniment tip E2: capetele noului link
			ss >> r1 >> r2 >> cost;
		
			if (nod_id == r1 || nod_id == r2) {
				request.msg_type = M2;
				request.creator = nod_id;
				request.seq_no = ++secventa;
				request.time = timp;
				req_payload.timp_creare = timp;
				
				if (nod_id == r1) {
					// Trimit request lui r2
					request.next_hop = r2;
					vecini_costuri[r2] = cost;
					topologie[r1][r2] = cost;
					topologie[r2][r1] = cost;
					req_payload.vecini_costuri[r2] = cost;
					memcpy(&request.payload, &req_payload, sizeof(my_payload));
					write(out, &request, sizeof(msg));
				}
				if (nod_id == r2) {
					// Trimit request lui r1
					request.next_hop = r1;
					vecini_costuri[r1] = cost;
					topologie[r1][r2] = cost;
					topologie[r2][r1] = cost;
					req_payload.vecini_costuri[r1] = cost;
					memcpy(&request.payload, &req_payload, sizeof(my_payload));
					write(out, &request, sizeof(msg));
				}
			}
			break;
			
		case E3:
		// eveniment tip E3: capetele linkului suprimat
			ss >> r1 >> r2;
			
			if (nod_id == r1 || nod_id == r2) {
				if (nod_id == r1) {
					vecini_costuri[r2] = 0;
				}
				if (nod_id == r2) {					
					vecini_costuri[r1] = 0;					
				}
				
				// Sterg nodul vecin din topologie
				topologie[r1][r2] = DRUMAX;
				topologie[r2][r1] = DRUMAX;
				
				// Construiesc si trimit M1
				lsa.msg_type = M1;
				lsa.creator = nod_id;
				lsa.seq_no = ++secventa;
				lsa.sender = nod_id;
				lsa.time = timp;
				new_payload.timp_creare = timp;
				for (int i = 0; i < KIDS; ++i) {
					new_payload.vecini_costuri[i] = vecini_costuri[i];
				}
				memcpy(&lsa.payload, &new_payload, sizeof(my_payload));
				for (int i = 0; i < KIDS; ++i) {
					if (vecini_costuri[i] != 0) {
						lsa.next_hop = i;
						write(out, &lsa, sizeof(msg));
					}
				}
			}
			break;
		
		case E4:
		// eveniment tip E4:  ruterul sursa al pachetului (event_t.router1)
			int s, d;
			ss >> s >> d;
			if (nod_id == s) {
				new_msg.msg_type = M4;
				new_msg.sender = nod_id;
				new_msg.creator = nod_id;
				new_msg.seq_no = ++secventa;
				new_msg.time = timp;
				new_msg.next_hop = tab_rutare[d][1];
				write(out, &new_msg, sizeof(msg));
			}
			break;
		}
	}
}

int main (int argc, char ** argv) {
	msg mesaj, mesaj_event, new_reply;
	my_payload new_payload;
	
	int fd_status, k, cost;
	
	int event = FALSE;
	
	out = atoi(argv[1]);  // legatura pe care se trimit mesaje catre simulatorul central (toate mesajele se trimit pe aici)
	in = atoi(argv[2]);   // legatura de pe care se citesc mesajele

	nod_id = atoi(argv[3]); // procesul curent participa la simulare numai dupa ce nodul cu ID-ul lui este adaugat in topologie

	// tab_rutare[k][0] reprezinta costul drumului minim de la ruterul curent (nod_id) la ruterul k
	// tab_rutare[k][1] reprezinta next_hop pe drumul minim de la ruterul curent (nod_id) la ruterul k
	for (k = 0; k < KIDS; k++) {
		tab_rutare[k][0] = DRUMAX;  // drum =DRUMAX daca ruterul k nu e in retea sau informatiile despre el nu au ajuns la ruterul curent
		tab_rutare[k][1] = -1; // ruterul k nu e (inca) cunoscut de ruterul nod_id
	}	
	tab_rutare[nod_id][0] = 0;
	
	// Initializare topologie
	for (int i = 0; i < KIDS; ++i) {
		for (int j = 0; j < KIDS; ++j) {
			topologie[i][j] = DRUMAX; // nu exista nicio legatura intre rutere initial
		}
		topologie[i][i] = 0; // costul de la un nod la el insusi este 0
	}

	printf ("Nod %d, pid %u alive & kicking\n", nod_id, getpid());

	if (nod_id == 0) { // sunt deja in topologie
		timp = -1; // la momentul 0 are loc primul eveniment
		mesaj.msg_type = 5; // finish procesare mesaje timp -1
		mesaj.sender = nod_id;
		write (out, &mesaj, sizeof(msg)); 
		printf ("TRIMIS Timp %d, Nod %d, msg tip 5 - terminare procesare mesaje vechi din coada\n", timp, nod_id);

	}

	while (!gata) {
		fd_status = read(in, &mesaj, sizeof(msg));

		if (fd_status <= 0) {
			printf ("Adio, lume cruda. Timp %d, Nod %d, msg tip %d cit %d\n", timp, nod_id, mesaj.msg_type, fd_status);
			exit (-1);
		}
		
		switch (mesaj.msg_type) {

		// 1,2,3,4 sunt mesaje din protocolul link state;
		// actiunea imediata corecta la primirea unui pachet de tip 1,2,3,4 este buffer-area
		// (punerea in coada /coada new daca sunt 2 cozi - vezi enunt)

		case M1:
			printf ("Timp %d, Nod %d, msg tip 1 - LSA\n", timp, nod_id);
			q_msg.push(mesaj);
			break;

		case M2:
			printf ("Timp %d, Nod %d, msg tip 2 - Database Request\n", timp, nod_id);
			q_msg.push(mesaj);
			break;

		case M3:
			printf ("Timp %d, Nod %d, msg tip 3 - Database Reply\n", timp, nod_id);
			q_msg.push(mesaj);
			break;

		case M4:
			printf ("Timp %d, Nod %d, msg tip 4 - pachet de date (de rutat)\n", timp, nod_id);
			q_msg.push(mesaj);
			break;

		case M6:
		{
			printf ("Timp %d, Nod %d, msg tip 6 - incepe procesarea mesajelor puse din coada la timpul anterior (%d)\n", timp, nod_id, timp-1);
			
			while (1) {
				// Procesez toate mesajele venite la timpul anterior
				// (sau toate mesajele primite inainte de inceperea timpului curent - marcata de mesaj de tip 6)

				// Cand coada devine goala sau urmatorul mesaj are timpul de trimitere == pasul curent de timp:
				if (q_msg.empty() || ((my_payload*)&q_msg.front().payload)->timp_creare == timp) {
					break;
				}
				
				msg top_msg = q_msg.front();
				q_msg.pop();
				
				// Destinatia unui mesaj de tip M4
				int destinatie = ((my_payload*)&top_msg.payload)->dest;
				
				bool drop; // indicator pentru ignorarea unui mesaj
				
				switch (top_msg.msg_type) {
				case M1:
					// Verific daca mesajul a ajuns inapoi la creator prin forwarding.
					if (top_msg.creator == nod_id)
						drop = true;
					else
						drop = false;
						
					// Verific daca exista mesaj lsa de la acelasi creator,
					// o copie sau unul mai nou.
					if (!drop && lsad[top_msg.creator].msg_type != 0 &&
						(lsad[top_msg.creator].time > top_msg.time ||
						lsad[top_msg.creator].seq_no >= top_msg.seq_no)){
						drop = true;
					}
					
					if (!drop) {
						// Actualizez LSADatabase si topologia.
						lsad[top_msg.creator] = top_msg;
						for (int i = 0; i < KIDS; ++i) {
							int cost = ((my_payload *)&top_msg.payload)->vecini_costuri[i];
							if (cost > 0) {
								topologie[i][top_msg.creator] = cost;
								topologie[top_msg.creator][i] = cost;
							}
							else if (i != top_msg.creator) {
								topologie[i][top_msg.creator] = DRUMAX;
								topologie[top_msg.creator][i] = DRUMAX;
							}
						}				
						
						// Trimit mai departe vecinilor mei.
						top_msg.sender = nod_id;
						top_msg.time = timp;
						for (int i = 0; i < KIDS; ++i) {
							if (vecini_costuri[i] != 0 && i != nod_id) {
								top_msg.next_hop = i;
								write(out, &top_msg, sizeof(msg));
							}
						}
					}
					break;
			
				case M2:
					// Am primit request. Construiesc mesaje de tip M3 pe
					// baza LSADatabase si trimit celui care a facut request.
					
					new_reply.msg_type = M3;
					new_reply.sender = nod_id;
					new_reply.time = timp;
					new_reply.next_hop = top_msg.creator;
					for (int i = 0; i < KIDS; ++i) {
						if (lsad[i].msg_type != 0) { // 
							new_reply.seq_no = lsad[i].seq_no;
							new_reply.creator = i;
							memcpy(&new_reply.payload, &lsad[i].payload, sizeof(new_reply.payload));
							((my_payload*)&new_reply.payload)->timp_creare = timp;
							write(out, &new_reply, sizeof(msg));
						}
					}
					
					// Updatez structurile de date
					cost = ((my_payload*)&top_msg.payload)->vecini_costuri[nod_id];
					
					vecini_costuri[top_msg.creator] = cost;
					lsad[top_msg.creator] = top_msg;
					
					topologie[nod_id][top_msg.creator] = cost;
					topologie[top_msg.creator][nod_id] = cost;
					
					
					// Trimit M1 tuturor vecinilor daca exista
					new_reply.msg_type = M1;
					new_reply.creator = nod_id;
					new_payload.timp_creare = timp;
					
					for (int i = 0; i < KIDS; ++i) {
						new_payload.vecini_costuri[i] = vecini_costuri[i];
					}
					
					memcpy(&new_reply.payload, &new_payload, sizeof(my_payload));
					
					new_reply.seq_no = ++secventa;
					for (int i = 0; i < KIDS; ++i) {
						if (vecini_costuri[i] != 0) {
							new_reply.next_hop = i;
							write(out, &new_reply, sizeof(msg));
						}
					}
					// Trimit noului vecin
					new_reply.next_hop = top_msg.creator;
					write(out, &new_reply, sizeof(msg));
					
					break;
					
				case M3:
					// Am primit reply.
					drop = false;
					// Verific daca exista mesaj lsa de la acelasi creator,
					// o copie sau unul mai nou.
					if (lsad[top_msg.creator].msg_type != 0 &&
						(lsad[top_msg.creator].time > top_msg.time ||
						lsad[top_msg.creator].seq_no >= top_msg.seq_no)) {
						drop = true;
					}
				
					if (!drop) {
						// Actualizez LSADatabase si topologia.
						lsad[top_msg.creator] = top_msg;
						for (int i = 0; i < KIDS; ++i) {
							cost = ((my_payload*)&top_msg.payload)->vecini_costuri[i];
							if (cost > 0) {
								topologie[i][top_msg.creator] = cost;
								topologie[top_msg.creator][i] = cost;
							}
							else if (i != top_msg.creator) {
								topologie[i][top_msg.creator] = DRUMAX;
								topologie[top_msg.creator][i] = DRUMAX;
							}
						}	
					}
					break;
					
				case M4:
					if (destinatie == nod_id) {
						;// s-a ajuns la destinatie
					} else {
						if (topologie[nod_id][destinatie] == DRUMAX) {
							// Mesajul este ignorat deoarece ruterul nu stie de
							// existenta destinatiei.
							continue;
						} else {
							top_msg.sender = nod_id;
							top_msg.time = timp;
							top_msg.next_hop = tab_rutare[destinatie][1];
							write(out, &top_msg, sizeof(msg));
						}
					}
					break;
				}
			}

			// procesez mesaj eveniment
			if (event == TRUE) {
				procesare_eveniment(mesaj_event);
				event = FALSE;
			}

			// calculez tabela de rutare
			calcul_tabela_rutare();
			
			// nu mai sunt mesaje vechi, am procesat evenimentul(daca exista), am calculat tabela de rutare(in aceasta ordine)
			// trimit mesaj de tip 5 (terminare pas de timp)
			mesaj.msg_type = 5;
			mesaj.sender = nod_id;
			write (out, &mesaj, sizeof(msg));
		}
		break;

		case M7:
			// aici se trateaza numai salvarea mesajului de tip eveniment(acest mesaj nu se pune in coada), pentru a fi procesat la finalul acestui pas de timp
			// procesarea o veti implementa in functia procesare_eveniment(), apelata in case 6
		{
			event = TRUE;
			memcpy (&mesaj_event, &mesaj, sizeof(msg));

			if (mesaj.add == TRUE)
				timp = mesaj.time+1; // initializam timpul pentru un ruter nou
		}
		break;

		case M8:
		{
			printf ("Timp %d, Nod %d, msg tip 8 - cerere tabela de rutare\n", timp, nod_id);

			mesaj.msg_type = 10;  // trimitere tabela de rutare
			mesaj.sender = nod_id;
			mesaj.time = timp;
			memcpy (mesaj.payload, &tab_rutare, sizeof (tab_rutare));
			write (out, &mesaj, sizeof(msg));
			timp++;
		}
		break;

		case M9:
		{
			// Aici poate sa apara timp -1 la unele "noduri"
			// E ok, e vorba de procesele care nu reprezentau rutere in retea, deci nu au de unde sa ia valoarea corecta de timp
			// Alternativa ar fi fost ca procesele neparticipante la simularea propriu-zisa sa ramana blocate intr-un apel de read()
			printf ("Timp %d, Nod %d, msg tip 9 - terminare simulare\n", timp, nod_id);
			gata = TRUE;
		}
		break;

		default:
			printf ("\nEROARE: Timp %d, Nod %d, msg tip %d - NU PROCESEZ ACEST TIP DE MESAJ\n", timp, nod_id, mesaj.msg_type);
			exit (-1);
		}			
	}
	
	return 0;
}
