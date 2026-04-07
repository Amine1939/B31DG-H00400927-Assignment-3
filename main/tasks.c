#include "tasks.h"
#include "edge_counter.h"   // edges_A(), edges_B()
#include "gpio.h"           // set_ack_X()
#include <stdio.h>          // standard I/O
#include "monitor.h"        // beginTaskX(), endTaskX()

extern uint32_t WorkKernel(uint32_t budget_cycles, uint32_t seed);

// Mutex for shared token data
// The LOCK / UNLOCK functions dont operate when the mutex is NULL so the same
// source file compiles and works correctly for both assignments.
static SemaphoreHandle_t token_mutex = NULL;

void tasks_set_mutex(SemaphoreHandle_t m) {
    token_mutex = m;
}

#define TOKEN_LOCK()   do { if (token_mutex) xSemaphoreTake(token_mutex, portMAX_DELAY); } while(0)
#define TOKEN_UNLOCK() do { if (token_mutex) xSemaphoreGive(token_mutex); } while(0)

// Token Declaration
static uint32_t tokenA = 0;    // latest token produced by Task A
static uint32_t tokenB = 0;    // latest token produced by Task B

// Token Availability variables
static bool A_ready = false;
static bool B_ready = false;

// Task Indices (release counters)
static uint32_t IDA = 0, IDB = 0, IDAGG = 0, IDC = 0, IDD = 0, IDS = 0;

// Publish Token Helpers
void publish_tokenA(uint32_t t) { TOKEN_LOCK(); tokenA  = t; A_ready = true; TOKEN_UNLOCK(); }
void publish_tokenB(uint32_t t) { TOKEN_LOCK(); tokenB  = t; B_ready = true; TOKEN_UNLOCK(); }

// Task A (10 ms periodic)
void task_A() {
    beginTaskA(IDA);
    set_ack_A(1);   // When ACK is HIGH, execution begins

    int countA = edges_A(); // rising edges on IN_A over last 10 ms

    uint32_t seed  = (IDA << 16) ^ countA ^ 0xA1;
    uint32_t token = WorkKernel(672000, seed);
    
    publish_tokenA(token);  // share result with Task AGG (mutex-protected)
    
    printf("A,%"PRIu32",%d,%"PRIu32"\n", IDA, countA, token);
    IDA++;

    set_ack_A(0);   // ACK LOW: execution ends
    endTaskA();
}

// Task B (20 ms periodic)
void task_B() {
    beginTaskB(IDB);
    set_ack_B(1);   // When ACK is HIGH, execution begins

    int countB = edges_B(); // rising edges on IN_A over last 20 ms

    uint32_t seed  = (IDB << 16) ^ countB ^ 0xB2;
    uint32_t token = WorkKernel(960000, seed);

    publish_tokenB(token);  // share result with Task AGG (mutex-protected)

    printf("B,%"PRIu32",%d,%"PRIu32"\n", IDB, countB, token);
    IDB++;

    set_ack_B(0);   // ACK LOW: execution ends
    endTaskB();
}

// Task AGG (20 ms periodic)
void task_AGG() {
    beginTaskAGG(IDAGG);
    set_ack_AGG(1);

    // Read shared tokens under the mutex so we can't see a half-written value
    TOKEN_LOCK();
    uint32_t agg;
    if (A_ready && B_ready) {
        agg = tokenA ^ tokenB;
    } else {
        agg = 0xDEADBEEF;
    }
    TOKEN_UNLOCK();

    uint32_t seed  = (IDAGG << 16) ^ agg ^ 0xD4;
    uint32_t token = WorkKernel(480000, seed);
    (void)token; // WorkKernel burns CPU; AGG doesn't publish its result.

    printf("AGG,%"PRIu32",%"PRIu32"\n", IDAGG, agg);
    IDAGG++;

    set_ack_AGG(0);
    endTaskAGG();
}

// Task C (50 ms periodic if in_mode = 1)
// IDC only increments when this function is actually called.
void task_C(uint32_t monitor_id) {
    beginTaskC(monitor_id);
    set_ack_C(1);

    uint32_t seed  = (IDC << 16) ^ 0xC3;
    uint32_t token = WorkKernel(1680000, seed);
    (void)token; // WorkKernel burns CPU; C doesn't publish its result.

    printf("C,%"PRIu32",%"PRIu32"\n", IDC, token);
    IDC++;

    set_ack_C(0);
    endTaskC();
}

// Task D (50 ms periodic if in_mode = 1)
void task_D(uint32_t monitor_id) {
    beginTaskD(monitor_id);
    set_ack_D(1);

    uint32_t seed  = (IDD << 16) ^ 0xD5;
    uint32_t token = WorkKernel(960000, seed);
    (void)token; // WorkKernel burns CPU; D doesn't publish its result.

    printf("D,%"PRIu32",%"PRIu32"\n", IDD, token);
    IDD++;

    set_ack_D(0);
    endTaskD();
}

// Task S (sporadic)
void task_S() {
    beginTaskS(IDS);
    set_ack_S(1);

    uint32_t seed  = (IDS << 16) ^ 0x55;
    uint32_t token = WorkKernel(600000, seed);
    (void)token; // WorkKernel burns CPU; S doesn't publish its result.

    printf("S,%"PRIu32",%"PRIu32"\n", IDS, token);
    IDS++;

    set_ack_S(0);
    endTaskS();
}