#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/sync.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "Lab5.h"

#define READ_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define READ_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

QueueHandle_t queue;
int toggle = 1;

typedef struct {
    uint gpio;
    uint32_t mask;
} isr_params_t;

void irq_callback(uint gpio, uint32_t event_mask)
{
    isr_params_t params;
    if (gpio != IN_PIN) return;

    params.gpio = gpio;
    params.mask = event_mask;

    xQueueSendFromISR(queue, &params, 0);
}

void read_task (__unused void *params) {
    isr_params_t isr_params;

    xQueueReceive(queue, &isr_params, 0);

    if (isr_params.gpio == IN_PIN) {
        toggle = !toggle;
        if (isr_params.mask & GPIO_IRQ_EDGE_RISE) {
            gpio_put(OUT_PIN, true);
        } else if (isr_params.mask & GPIO_IRQ_EDGE_FALL) {
            gpio_put(OUT_PIN, false);
        }
    }
}

int main(void)
{
    TaskHandle_t task;
    queue = xQueueCreate(10, sizeof(isr_params_t));

    stdio_init_all();

    gpio_init(IN_PIN);
    gpio_set_dir(IN_PIN, GPIO_IN);

    gpio_init(OUT_PIN);
    gpio_set_dir(OUT_PIN, GPIO_OUT);
    gpio_put(OUT_PIN, toggle);

    gpio_set_irq_enabled_with_callback(IN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL , true, irq_callback);
    
    xTaskCreate(read_task, "ReadTask",
                READ_TASK_STACK_SIZE, NULL, READ_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    
    while(1) __wfi();
    return 0;
}