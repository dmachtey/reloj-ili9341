// button_events.c
#include "button_events.h"
#include "freertos/task.h"
#include "esp_log.h"

// Definición del EventGroup
EventGroupHandle_t xButtonEventGroup = NULL;

void ButtonEvents_Init(void) {
    // Crear EventGroup para botones
    xButtonEventGroup = xEventGroupCreate();
    if (xButtonEventGroup == NULL) {
        ESP_LOGE("BUTTON_EV", "Error al crear EventGroup");
    }
    // Configurar pines de botones como entrada con pull-up
    gpio_set_direction(BUTTON_PIN_START_STOP, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_START_STOP);
    gpio_set_direction(BUTTON_PIN_RESET, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_RESET);
    gpio_set_direction(BUTTON_PIN_FUNC, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_FUNC);
}

void ButtonEvents_Task(void *pvParameters) {
    int prevStart = 1, prevReset = 1, prevFunc = 1;
    while (1) {
        int curStart = gpio_get_level(BUTTON_PIN_START_STOP);
        int curReset = gpio_get_level(BUTTON_PIN_RESET);
        int curFunc  = gpio_get_level(BUTTON_PIN_FUNC);

        // Flanco descendente: Start/Stop
        if (curStart == 0 && prevStart == 1) {
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_START_STOP);
        }
        // Flanco descendente: Reset
        if (curReset == 0 && prevReset == 1) {
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_RESET);
        }
        // Flanco descendente: Función
        if (curFunc == 0 && prevFunc == 1) {
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_FUNC_CHANGE);
        }
        prevStart = curStart;
        prevReset = curReset;
        prevFunc  = curFunc;

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
