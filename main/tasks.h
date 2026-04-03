#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* Mutex injection:
    Call once before any tasks start, passing a FreeRTOS mutex handle.
    Enables thread-safe access to the shared tokenA/tokenB data.
*/
void tasks_set_mutex(SemaphoreHandle_t m);

// Periodic task functions
void task_A();                      // Task A: runs every 10 ms
void task_B();                      // Task B: runs every 20 ms
void task_AGG();                    // Aggregator Task:  runs every 20 ms, after B
void task_C(uint32_t monitor_id);   // Task C: runs every 50 ms (when IN_MODE=1)
void task_D(uint32_t monitor_id);   // Task D: runs every 50 ms (when IN_MODE=1)
void task_S();                      // Task S: sporadic

// Token publication helpers
void publish_tokenA(uint32_t t);    // Store token from Task A for Task AGG
void publish_tokenB(uint32_t t);    // Store token from Task B for Task AGG

#endif