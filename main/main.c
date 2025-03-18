/************************************************************************************************
Copyright (c) 2022-2023, Laboratorio de Microprocesadores
Facultad de Ciencias Exactas y Tecnología, Universidad Nacional de Tucumán
https://www.microprocesadores.unt.edu.ar/

Copyright (c) 2022-2023, Esteban Volentini <evolentini@herrera.unt.edu.ar>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

SPDX-License-Identifier: MIT
*************************************************************************************************/

/** @file blinking.c
 **
 ** @brief Ejemplo de un led parpadeando
 **
 ** Ejemplo de un led parpadeando utilizando la capa de abstraccion de 
 ** hardware y sin sistemas operativos.
 ** 
 ** | RV | YYYY.MM.DD | Autor       | Descripción de los cambios              |
 ** |----|------------|-------------|-----------------------------------------|
 ** |  3 | 2025.03.12 | evolentini  | Adaptación para plataforma ESP-32       |
 ** |  2 | 2017.10.16 | evolentini  | Correción en el formato del archivo     |
 ** |  1 | 2017.09.21 | evolentini  | Version inicial del archivo             |
 ** 
 ** @defgroup ejemplos Proyectos de ejemplo
 ** @brief Proyectos de ejemplo de la Especialización en Sistemas Embebidos
 ** @{ 
 */


/* === Headers files inclusions =============================================================== */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* === Macros definitions ====================================================================== */

// #define LED_ROJO  GPIO_NUM_10
// #define LED_VERDE GPIO_NUM_3
#define LED_AZUL  GPIO_NUM_2
#define LED_VERDE GPIO_NUM_0
#define LED_ROJO  GPIO_NUM_1

/* === Private data type declarations ========================================================== */

/** @brief Estructura con los parametros para una tarea de baliza */
typedef struct {
    uint8_t led;    /** < Led que debe hacer parpadear la tarea */
    uint16_t delay; /** < Demora entre cada encendido y apagado */
} blinking_t;

/* === Private variable declarations =========================================================== */

/* === Private function declarations =========================================================== */

/** @brief Función que implementa una tarea de baliza
 **
 ** @parameter[in] parametros Puntero a una estructura que contiene el led
 **                           y la demora entre encendido y apagado.
 */
void Blinking(void * parametros);

/* === Public variable definitions ============================================================= */

/* === Private variable definitions ============================================================ */

static const blinking_t valores[] = {
    {.led = LED_ROJO, .delay = 500},
    {.led = LED_VERDE, .delay = 300},
    {.led = LED_AZUL, .delay = 700},
};

/* === Private function implementation ========================================================= */

void Blinking(void * parametros) {
    const blinking_t * argumentos = parametros;

    gpio_set_direction(argumentos->led, GPIO_MODE_OUTPUT);
    gpio_set_level(argumentos->led, 0);

    while (1) {
        // gpio_set_level(argumentos->led, (gpio_get_level(argumentos->led) != 0));
        // vTaskDelay(pdMS_TO_TICKS(argumentos->delay));

        gpio_set_level(argumentos->led, 1);
        vTaskDelay(pdMS_TO_TICKS(argumentos->delay));
    
        gpio_set_level(argumentos->led, 0);
        vTaskDelay(pdMS_TO_TICKS(argumentos->delay));
    }
}

/* === Public function implementation ========================================================== */

void app_main(void) {
    xTaskCreate(Blinking, "Rojo", configMINIMAL_STACK_SIZE, (void *)&valores[0],
                tskIDLE_PRIORITY + 1, NULL);

    xTaskCreate(Blinking, "Verde", configMINIMAL_STACK_SIZE, (void *)&valores[1],
                tskIDLE_PRIORITY + 1, NULL);

    xTaskCreate(Blinking, "Azul", configMINIMAL_STACK_SIZE, (void *)&valores[2],
                tskIDLE_PRIORITY + 1, NULL);

    while (1);
}

/* === End of documentation ==================================================================== */

/** @} End of module definition for doxygen */
