#ifndef GPIO_SETUP_H
#define GPIO_SETUP_H

#define SYNC_PIN 34
#define IN_S_PIN 17     // sporadic trigger input
#define IN_MODE_PIN 4     // mode-control input (HIGH = tasks C/D enabled)

#define ACK_A   13
#define ACK_B   12
#define ACK_AGG 14
#define ACK_C   27
#define ACK_D   26
#define ACK_S   25

void gpio_setup();          // Call once at the beginning to set up GPIO directions

int read_SYNC();            // Returns 0 or 1 depending on the level of the SYNC pin
int read_IN_S();            // Returns 0 or 1 depending on the level of the IN_S pin
int read_IN_MODE();         // Returns 0 or 1 depending on the level of the IN_MODE pin

void set_ack_A(int v);      // Set the ACK_A pin to level v (0 or 1)
void set_ack_B(int v);      // Set the ACK_B pin to level v (0 or 1)
void set_ack_AGG(int v);    // Set the ACK_AGG pin to level v (0 or 1)
void set_ack_C(int v);      // Set the ACK_C pin to level v (0 or 1)
void set_ack_D(int v);      // Set the ACK_D pin to level v (0 or 1)
void set_ack_S(int v);      // Set the ACK_S pin to level v (0 or 1)

#endif