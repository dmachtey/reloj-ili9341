/************************************************************************************************
 * Ejemplo modificado para ESP32-3248S035C
 * 
 * - La variable global "decimas" se incrementa cada 10 ms mediante la tarea decimasTask,
 *   la cual utiliza vTaskDelayUntil para mayor precisión.
 * - Se cuenta tiempo en minutos, segundos y décimas (décimas de segundo: de 0 a 100).
 *   La conversión es:
 *      • 1 segundo = 100 décimas
 *      • 1 minuto = 6000 décimas
 * - La tarea displayTask actualiza tres paneles: uno para minutos, otro para segundos y 
 *   otro para décimas. Además, se dibujan separadores (círculos) cuyo color depende de la
 *   paridad de los segundos.
 * - Cada 300 ms se ejecuta la tarea toggleTask, que lee tres botones (PB1, PB2 y PB3), 
 *   actualiza una variable global con su estado actual (con campos "arrancar", "reset" y 
 *   "congelar") y alterna (toggle) el estado de los LEDs correspondientes (LED_ROJO, LED_VERDE 
 *   y LED_AZUL) en cada pulsación.
 *
 * Funciones de dibujo utilizadas: 
 *    - DibujarDigito(panel, posición, dígito)
 *    - ILI9341DrawFilledCircle(x, y, radio, color)
 *
 * Se asume que las funciones de inicialización del LCD y de creación de paneles están definidas
 * en "ili9341.h" y "digitos.h".
 *************************************************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ili9341.h"
#include "digitos.h"
#include "esp_log.h"
#include "driver/gpio.h"

// Parámetros de dibujo de dígitos
#define DIGITO_ANCHO     60
#define DIGITO_ALTO      100
#define DIGITO_ENCENDIDO ILI9341_RED
#define DIGITO_APAGADO   0x1800
#define DIGITO_FONDO     ILI9341_BLACK

// Definición de offset en mayúsculas
#define OFFSET_X 10

// Definición de pines para LEDs
#define LED_ROJO   GPIO_NUM_4
#define LED_VERDE  GPIO_NUM_16
#define LED_AZUL   GPIO_NUM_17

// Definición de pines para botones (configurados con pull-up)
#define PB1   GPIO_NUM_35
#define PB2   GPIO_NUM_22
#define PB3   GPIO_NUM_21

// Variable global: cada incremento representa 1 décima de segundo (10 ms)
uint32_t decimas = 0;

// Definición de una estructura para almacenar el estado actual de los botones
// Los campos se renombran a "arrancar", "reset" y "congelar"
typedef struct {
    int arrancar;
    int reset;
    int congelar;
} botones_state_t;

// Variable global que almacena el estado de los botones.
// Inicialmente, los botones no se presionan (valor lógico alto)
botones_state_t botonesEstado = {1, 1, 1};

// Paneles para mostrar el tiempo: minutos, segundos y décimas (2 dígitos cada uno)
panel_t panel_minutes;   // Panel para minutos
panel_t panel_seconds;   // Panel para segundos
panel_t panel_decimas;   // Panel para décimas

// Tarea que incrementa "decimas" cada 10 ms utilizando vTaskDelayUntil.
// Si se detecta reset en la variable global, se reinicia la cuenta.
// Solo se incrementa si "arrancar" está activo.
void decimasTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        if (botonesEstado.reset)
            decimas = 0;
        else if (botonesEstado.arrancar)
            decimas++;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

// Tarea que actualiza los paneles de tiempo cada 45 ms.
// Se calcula el tiempo a partir de la variable "decimas":
//   • minutos = decimas / 6000
//   • segundos = (decimas / 100) % 60
//   • décimas = decimas % 100
// Se dibujan separadores (círculos) cuyo color depende de la paridad de los segundos.
void displayTask(void *pvParameters) {
    while (1) {
        static uint32_t total = 0;
        if (!botonesEstado.congelar)
            total = decimas;

        uint32_t mins = total / 6000;
        uint32_t secs = (total / 100) % 60;
        uint32_t d    = total % 100;

        // Actualiza el panel de minutos (2 dígitos)
        DibujarDigito(panel_minutes, 0, mins / 10);
        DibujarDigito(panel_minutes, 1, mins % 10);

        // Determina el color de los círculos según la paridad de los segundos
        uint16_t circleColor = (secs % 2 == 0) ? DIGITO_APAGADO : DIGITO_ENCENDIDO;

        // Dibuja separador entre minutos y segundos
        ILI9341DrawFilledCircle(160 + OFFSET_X, 90 + 20, 5, circleColor);
        ILI9341DrawFilledCircle(160 + OFFSET_X, 130 + 20, 5, circleColor);

        // Actualiza el panel de segundos (2 dígitos)
        DibujarDigito(panel_seconds, 0, secs / 10);
        DibujarDigito(panel_seconds, 1, secs % 10);

        // Dibuja separador entre segundos y décimas
        ILI9341DrawFilledCircle(300 + OFFSET_X, 90 + 20, 5, circleColor);
        ILI9341DrawFilledCircle(300 + OFFSET_X, 130 + 20, 5, circleColor);

        // Actualiza el panel de décimas (2 dígitos)
        DibujarDigito(panel_decimas, 0, d / 10);
        DibujarDigito(panel_decimas, 1, d % 10);

        vTaskDelay(pdMS_TO_TICKS(45));
    }
}

// Tarea que lee los botones cada 300 ms y alterna (toggle) el estado de los LEDs correspondientes.
// Además, actualiza la variable global "botonesEstado" con el estado actual de cada botón.
void toggleTask(void *pvParameters) {
    int prevPB1 = 1, prevPB2 = 1, prevPB3 = 1;
    int ledRojoState = 1, ledVerdeState = 1, ledAzulState = 1; // Inicialmente, LEDs apagados (nivel 1)
    
    while (1) {
        int currPB1 = gpio_get_level(PB1);
        int currPB2 = gpio_get_level(PB2);
        int currPB3 = gpio_get_level(PB3);

        // Actualiza la variable global con el estado actual de los botones.
        // Se invierte el valor de PB2 y PB3 según la lógica de toggle implementada.
        botonesEstado.reset    = !currPB2;
        
        // Si se detecta un flanco de bajada en PB1, se alterna el estado del LED_ROJO.
        if (currPB1 == 0 && prevPB1 == 1) {
            ledRojoState = (ledRojoState == 1) ? 0 : 1;
            botonesEstado.arrancar = (ledRojoState == 1) ? 0 : 1;
            gpio_set_level(LED_ROJO, ledRojoState);
        }
        // Si se detecta un flanco de bajada en PB2, se alterna el estado del LED_VERDE.
        if (currPB2 == 0 && prevPB2 == 1) {
            ledVerdeState = (ledVerdeState == 1) ? 0 : 1;
            gpio_set_level(LED_VERDE, ledVerdeState);
        }
        // Si se detecta un flanco de bajada en PB3, se alterna el estado del LED_AZUL.
        if (currPB3 == 0 && prevPB3 == 1) {
            ledAzulState = (ledAzulState == 1) ? 0 : 1;
            botonesEstado.congelar = (ledAzulState == 1) ? 0 : 1;
            gpio_set_level(LED_AZUL, ledAzulState);
        }

        prevPB1 = currPB1;
        prevPB2 = currPB2;
        prevPB3 = currPB3;

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// Definición del tamaño de pila para cada tarea
#define STACK_SIZE 2048

// Variables para la creación estática de tareas
StaticTask_t xDecimasTaskBuffer;
StackType_t xDecimasStack[STACK_SIZE];

StaticTask_t xDisplayTaskBuffer;
StackType_t xDisplayStack[STACK_SIZE];

StaticTask_t xToggleTaskBuffer;
StackType_t xToggleStack[STACK_SIZE];

void app_main(void) {
    // Inicializa el LCD y lo rota en modo Landscape
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    // Crea paneles para mostrar el tiempo: minutos, segundos y décimas (2 dígitos cada uno)
    panel_minutes = CrearPanel(30 + OFFSET_X, 80, 2, DIGITO_ALTO, DIGITO_ANCHO,
                                DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_seconds = CrearPanel(170 + OFFSET_X, 80, 2, DIGITO_ALTO, DIGITO_ANCHO,
                                DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_decimas = CrearPanel(310 + OFFSET_X, 80, 2, DIGITO_ALTO, DIGITO_ANCHO,
                                DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    // Configura los pines de los LEDs como salida y los inicializa apagados (nivel 1)
    gpio_set_direction(LED_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_AZUL, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_ROJO, 1);
    gpio_set_level(LED_VERDE, 1);
    gpio_set_level(LED_AZUL, 1);

    // Configura los pines de los botones como entrada con pull-up
    gpio_set_direction(PB1, GPIO_MODE_INPUT);
    gpio_pullup_en(PB1);
    gpio_set_direction(PB2, GPIO_MODE_INPUT);
    gpio_pullup_en(PB2);
    gpio_set_direction(PB3, GPIO_MODE_INPUT);
    gpio_pullup_en(PB3);

    // Crea las tareas usando xTaskCreateStatic
    xTaskCreateStatic(decimasTask, "DecimasTask", STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, 
                      xDecimasStack, &xDecimasTaskBuffer);
    xTaskCreateStatic(displayTask, "DisplayTask", STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, 
                      xDisplayStack, &xDisplayTaskBuffer);
    xTaskCreateStatic(toggleTask, "ToggleTask", STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, 
                      xToggleStack, &xToggleTaskBuffer);

    // Inicializa manualmente los estados de "congelar" y "arrancar"
    botonesEstado.congelar = 0;
    botonesEstado.arrancar = 0; 

    // Se elimina la tarea principal, ya que todo se gestiona en las tareas creadas
    vTaskDelete(NULL);
}
