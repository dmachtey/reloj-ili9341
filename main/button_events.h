// button_events.h
#ifndef BUTTON_EVENTS_H
#define BUTTON_EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

// Pines de botones
#define BUTTON_PIN_START_STOP   GPIO_NUM_35  // PB1: iniciar/detener cronómetro
#define BUTTON_PIN_RESET        GPIO_NUM_22  // PB2: resetear cronómetro
#define BUTTON_PIN_FUNC         GPIO_NUM_21  // PB3: cambiar función (cronómetro, reloj, alarma)

// Bits de evento
#define EV_BIT_START_STOP       (1 << 0)
#define EV_BIT_RESET            (1 << 1)
#define EV_BIT_FUNC_CHANGE      (1 << 2)

#ifdef __cplusplus
extern "C" {
#endif

// Grupo de eventos expuesto para captura de botones
extern EventGroupHandle_t xButtonEventGroup;

// Inicializa GPIO y crea el EventGroup
void ButtonEvents_Init(void);

// Tarea que monitorea los botones y pone bits
void ButtonEvents_Task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_EVENTS_H



