#include "tasks/core0Tasks.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void tarefaCore0(void* parametro) {

    while (true) {

        // Responsabilidades do Core 0 serão executadas aqui.

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void iniciarTasksCore0() {

    xTaskCreatePinnedToCore(
        tarefaCore0,
        "Core0Task",
        4096,
        nullptr,
        1,
        nullptr,
        0
    );
}