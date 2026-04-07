#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "tasks.h"
#include "edge_counter.h"
#include "gpio.h"
#include "monitor.h"

// CPU Core & scheduling constants
#define APP_CORE 0 // All tasks must run on core 0

// Tasks periods
#define PERIOD_A_MS 10
#define PERIOD_B_MS 20
#define PERIOD_AGG_MS 20
#define PERIOD_C_MS 50
#define PERIOD_D_MS 50

// Rate-monotonic priorities (higher number = higher FreeRTOS priority).
// Task S shares priority 4 with Task B so a sporadic trigger is serviced
// within the same band.
#define PRIORITY_A     5           // 10 ms
#define PRIORITY_B     4           // 20 ms
#define PRIORITY_S     1           // sporadic <= 30 ms response time
#define PRIORITY_AGG   4           // 20 ms
#define PRIORITY_C     3           // 50 ms
#define PRIORITY_D     3           // 50 ms

// 4000 words (~15.6 KB) gives comfortable headroom for printing.
#define STACK_WORDS 4000

// Synchronisation primitives 
static SemaphoreHandle_t xTokenMutex;       // protects shared tokenA / tokenB in tasks.c
static SemaphoreHandle_t xSporadicSem;      // counting semaphore: one unit per IN_S edge

// Task handles (needed so app_main can send the SYNC notification)
static TaskHandle_t hA, hB, hAGG, hC, hD, hS;

// T0 in FreeRTOS ticks: moment SYNC goes HIGH.
static volatile TickType_t t0_tick = 0;

// IN_MODE is sampled at discrete 50ms instants by Task C/D.
static volatile bool in_mode_pressed = false;

// ISR: rising edge on IN_S
// Records the release timestamp for the monitor, then
// gives the counting semaphore so vTaskS wakes up.
static void IRAM_ATTR in_s_isr(void *arg) {
    BaseType_t woken = pdFALSE;                     
    notifySRelease();                               // monitor: record release time of S
    xSemaphoreGiveFromISR(xSporadicSem, &woken);    // wake Task S
    portYIELD_FROM_ISR(woken);                      // yield if a higher-priority task unblocked
}

// Periodic task bodies
// Pattern:
//   1. Block on direct-to-task notification to wait for app_main to signal SYNC.
//   2. Initialise xLastWake to t0_tick so the first release is T0 + 1 period.
//   3. vTaskDelayUntil keeps the task phase-locked to T0 with no drift.

static void vTaskA(void *arg) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for SYNC
    TickType_t xLastWake = t0_tick;
    while (1) {
        task_A();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(PERIOD_A_MS));
    }
}

static void vTaskB(void *arg) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    TickType_t xLastWake = t0_tick;
    while (1) {
        task_B();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(PERIOD_B_MS));
    }
}

static void vTaskAGG(void *arg) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    TickType_t xLastWake = t0_tick;
    while (1) {
        task_AGG();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(PERIOD_AGG_MS));
    }
}

// Tasks C and D: check IN_MODE each period before executing.
// When IN_MODE == 0 the task (and its IDX increment) is skipped.
static void vTaskC(void *arg) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    TickType_t xLastWake = t0_tick;
    uint32_t monitorIDC = 0;
    while (1) {
        if (read_IN_MODE()) {
            in_mode_pressed = true;
        }
        if (in_mode_pressed) {
            task_C(monitorIDC); // beginTaskC uses monitorIDC
        }
        // IN_MODE == 0: code block skipped, IDC unchanged, no ACK pulse
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(PERIOD_C_MS));
        monitorIDC++;
    }
}

static void vTaskD(void *arg) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    TickType_t xLastWake = t0_tick;
    uint32_t monitorIDD = 0;
    while (1) {
        if (read_IN_MODE()) {
            in_mode_pressed = true;
        }
        if (in_mode_pressed) {
            task_D(monitorIDD);
        }
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(PERIOD_D_MS));
        monitorIDD++;
    }
}

// Task S: 
//      1. Blocks on the counting semaphor 
//      2. the ISR gives one unit per IN_S rising edge.  
//      3. Priority 4 ensures an accurate response 
//         even if Task B (same priority) is currently running.
static void vTaskS(void *arg) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);            // wait for SYNC before accepting jobs
    while (1) {
        xSemaphoreTake(xSporadicSem, portMAX_DELAY);    // block until IN_S fires
        task_S();
    }
}

// app_main
void app_main(void) {

    // Hardware initialisation
    gpio_setup();                                   // GPIO setup (includes IN_MODE)
    edge_counter_init();                            // PCNT hardware for IN_A / IN_B
    monitorInit();
    monitorSetPeriodicReportEverySeconds(5);
    monitorSetFinalReportAfterSeconds(20);

    // FreeRTOS synchronisation objects
    xTokenMutex = xSemaphoreCreateMutex();             // protects tokenA/tokenB in tasks.c
    tasks_set_mutex(xTokenMutex);                   // inject mutex into task

    xSporadicSem = xSemaphoreCreateCounting(12, 0);    // up to 12 queued sporadic jobs

    // IN_S rising-edge interrupt
    gpio_set_intr_type(IN_S_PIN, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(IN_S_PIN, in_s_isr, NULL);

    // Create application tasks (all pinned to APP_CORE = core 0)
    // Each task blocks immediately on ulTaskNotifyTake() 
    // no tasks runs until app_main pushes the SYNC notification below.
    xTaskCreatePinnedToCore(vTaskA,   "TaskA",   STACK_WORDS, NULL, PRIORITY_A,   &hA,   APP_CORE);
    xTaskCreatePinnedToCore(vTaskB,   "TaskB",   STACK_WORDS, NULL, PRIORITY_B,   &hB,   APP_CORE);
    xTaskCreatePinnedToCore(vTaskAGG, "TaskAGG", STACK_WORDS, NULL, PRIORITY_AGG, &hAGG, APP_CORE);
    xTaskCreatePinnedToCore(vTaskC,   "TaskC",   STACK_WORDS, NULL, PRIORITY_C,   &hC,   APP_CORE);
    xTaskCreatePinnedToCore(vTaskD,   "TaskD",   STACK_WORDS, NULL, PRIORITY_D,   &hD,   APP_CORE);
    xTaskCreatePinnedToCore(vTaskS,   "TaskS",   STACK_WORDS, NULL, PRIORITY_S,   &hS,   APP_CORE);

    // Wait for SYNC rising edge
    // Polling until SYNC pulse is received, no tasks are running yet.
    while (!read_SYNC());

    // Capture T0 in FreeRTOS ticks before waking any task
    t0_tick = xTaskGetTickCount();
    in_mode_pressed = false;
    synch(); // notify monitor of T0

    // Release all tasks simultaneously
    xTaskNotifyGive(hA);
    xTaskNotifyGive(hB);
    xTaskNotifyGive(hAGG);
    xTaskNotifyGive(hC);
    xTaskNotifyGive(hD);
    xTaskNotifyGive(hS);

    // Monitor loop
    //      1. app_main stays active to poll the timing monitor.  
    //      2. vTaskDelay yields the CPU so application tasks can run.
    //      3. app_main will only consume CPU when woken by the tick (every 100 ms).
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (monitorPollReports()) {
            // Final report has been printed, suspend system.
            while (1) { vTaskDelay(portMAX_DELAY); }
        }
    }
}