#include "tasks/core1Tasks.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void tarefaCore1(void* parametro) {

    while (true) {

        // Responsabilidades do Core 1 serão executadas aqui.

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void iniciarTasksCore1() {

    xTaskCreatePinnedToCore(
        tarefaCore1,
        "Core1Task",
        4096,
        nullptr,
        1,
        nullptr,
        1
    );
}