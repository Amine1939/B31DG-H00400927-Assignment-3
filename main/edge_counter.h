#ifndef EDGE_COUNTER_H
#define EDGE_COUNTER_H

// Initialise PCNT hardware for IN_A and IN_B.  Call once at startup.
void edge_counter_init(void);

// Return the number of rising edges on IN_A since the last call to edges_A().
int edges_A(void);

// Return the number of rising edges on IN_B since the last call to edges_B().
int edges_B(void);

#endif // EDGE_COUNTER_H