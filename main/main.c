#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ili9341.h"
#include "digitos.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "button_events.h"    // donde están xButtonEventGroup y los EV_BIT_…


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
// #define PB1   GPIO_NUM_35
// #define PB2   GPIO_NUM_22
// #define PB3   GPIO_NUM_21

// Variables sincronización
SemaphoreHandle_t semDecimas;
SemaphoreHandle_t semParciales;

extern SemaphoreHandle_t semDecimas;

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
// Inicialmente, se parte de la cuenta detenida (arrancar = 0) y sin congelar (congelar = 0).
botones_state_t botonesEstado = {0, 0, 0};

// Paneles para mostrar el tiempo: minutos, segundos y décimas (2 dígitos cada uno)
typedef struct {
    panel_t panel_minutes;
    panel_t panel_seconds;
    panel_t panel_decimas;
} panel_mt;
panel_mt PanelPPL;

// Tarea que incrementa "decimas" cada 10 ms utilizando vTaskDelayUntil.
// Si se detecta reset, se reinicia la cuenta.
// Solo se incrementa si "arrancar" está activo.
void decimasTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        if (xSemaphoreTake(semDecimas, portMAX_DELAY) == pdTRUE) {
            if (botonesEstado.reset && !botonesEstado.arrancar)
                decimas = 0;
            else if (botonesEstado.arrancar)
                decimas++;
            xSemaphoreGive(semDecimas);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

// Tarea que actualiza los paneles de tiempo cada 45 ms.
void displayTask(void *pvParameters) {
    static uint32_t total = 0;
    static uint32_t parciales[4];
    static uint8_t actualizo = 0;

    while (1) {

        if (botonesEstado.congelar)
                actualizo = 0;
       
        // Actualiza parciales si aplica
        if ((botonesEstado.congelar == 0) && (actualizo == 0)) {
            if (xSemaphoreTake(semParciales, portMAX_DELAY) == pdTRUE) {
                parciales[3] = parciales[2];
                parciales[2] = parciales[1];
                parciales[1] = parciales[0];
                parciales[0] = total;
                xSemaphoreGive(semParciales);
            }
            actualizo = 1;
        }


// Obtiene valor de decimas protegido
        if (xSemaphoreTake(semDecimas, portMAX_DELAY) == pdTRUE) {
            if (!botonesEstado.congelar)
                total = decimas;
            xSemaphoreGive(semDecimas);
        }


        // Cálculo de tiempo para display
        uint32_t mins = total / 6000;
        uint32_t secs = (total / 100) % 60;
        uint32_t d    = total % 100;

        // Actualiza el panel de minutos (2 dígitos)
        DibujarDigito(PanelPPL.panel_minutes, 0, mins / 10);
        DibujarDigito(PanelPPL.panel_minutes, 1, mins % 10);

        // Determina el color de los círculos según la paridad de los segundos
        uint16_t circleColor = (secs % 2 > 0) ? DIGITO_APAGADO : DIGITO_ENCENDIDO;
        ILI9341DrawFilledCircle(160 + OFFSET_X, 90 + 20, 5, circleColor);
        ILI9341DrawFilledCircle(160 + OFFSET_X, 130 + 20, 5, circleColor);

        // Actualiza el panel de segundos (2 dígitos)
        DibujarDigito(PanelPPL.panel_seconds, 0, secs / 10);
        DibujarDigito(PanelPPL.panel_seconds, 1, secs % 10);

        ILI9341DrawFilledCircle(300 + OFFSET_X, 90 + 20, 5, circleColor);
        ILI9341DrawFilledCircle(300 + OFFSET_X, 130 + 20, 5, circleColor);

        // Actualiza el panel de décimas (2 dígitos)
        DibujarDigito(PanelPPL.panel_decimas, 0, d / 10);
        DibujarDigito(PanelPPL.panel_decimas, 1, d % 10);

        // Carga valores parciales protegidos
        uint32_t local_parciales[3] = {0};
        if (xSemaphoreTake(semParciales, portMAX_DELAY) == pdTRUE) {
            local_parciales[0] = parciales[0];
            local_parciales[1] = parciales[1];
            local_parciales[2] = parciales[2];
            xSemaphoreGive(semParciales);
        }

        if (decimas == 0)
            if (xSemaphoreTake(semParciales, portMAX_DELAY) == pdTRUE) {
                parciales[3] = 0;
                parciales[2] = 0;
                parciales[1] = 0;
                parciales[0] = 0;
               xSemaphoreGive(semParciales);
            }

        // Muestra parciales en pantalla
        char buf[16];
        for (int i = 0; i < 3; i++) {
            uint32_t pmin = (local_parciales[i] / 6000) % 100;
            uint32_t psec = (local_parciales[i] / 100) % 60;
            uint32_t pde  = local_parciales[i] % 100;
            snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", pmin, psec, pde);
            ILI9341DrawString(30 + OFFSET_X, 180 + 36 * i, buf, &font_16x26,
                              ILI9341_WHITE, DIGITO_APAGADO);
        }

        vTaskDelay(pdMS_TO_TICKS(45));
    }
}

// Tarea que lee los botones cada 30 ms y alterna (toggle) el estado de los LEDs correspondientes.

void toggleTask(void *pvParameters) {
    // Estados internos que replican la lógica anterior
    static int PB1State = 1, PB2State = 1, PB3State = 1;
    EventBits_t bits;

    for (;;) {
        // Espera bloqueante a cualquiera de los 3 eventos y limpia el bit capturado
        bits = xEventGroupWaitBits(
            xButtonEventGroup,
            EV_BIT_START_STOP   |  // PB1
            EV_BIT_RESET        |  // PB2
            EV_BIT_FUNC_CHANGE,    // PB3
            pdTRUE,                // limpiar bit al tomarlo
            pdFALSE,               // basta cualquiera, no todos
            portMAX_DELAY         // sin tiempo de espera
        );

        // --- Start/Stop (PB1) ---
        if (bits & EV_BIT_START_STOP) {
            PB1State = !PB1State;
            botonesEstado.arrancar = !PB1State;
            // cuando se detiene, habilita “congelar”
            botonesEstado.congelar = (botonesEstado.arrancar == 0);
        }

        // --- Reset (PB2) ---
        if (bits & EV_BIT_RESET) {
            PB2State = !PB2State;
            // resetea el cronómetro a cero de inmediato
            if (xSemaphoreTake(semDecimas, portMAX_DELAY) == pdTRUE) {
                decimas = 0;
                xSemaphoreGive(semDecimas);
            }  
            // y despeja el flag de “congelar”
            botonesEstado.congelar = 0;
        }

        // --- Función (PB3) ###
        if (bits & EV_BIT_FUNC_CHANGE) {
            PB3State = !PB3State;
            // Aquí puede rotar entre modos: cronómetro, reloj, alarma…
            // e.g. botonesEstado.modo = PB3State;
        }
    }
}


// Tarea que actualiza el estado de los LEDs de estatus.
void LedStatusTask(void *pvParameters) {
    static uint8_t ledVerde = 1;
    while (1) {
        if (botonesEstado.arrancar)
            ledVerde = !ledVerde;
        else
            ledVerde = 1;

        gpio_set_level(LED_VERDE, (botonesEstado.congelar == 0) ? ledVerde : 1);
        gpio_set_level(LED_ROJO, (botonesEstado.congelar == 0) ? 1 : 0);

        vTaskDelay(pdMS_TO_TICKS(500));
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
StaticTask_t xLedStatusTaskBuffer;
StackType_t xLedStatusStack[STACK_SIZE];
StaticTask_t xButtonEventsTaskBuffer;
StackType_t xButtonEventsStatusStack[STACK_SIZE];


void app_main(void) {
    // Inicializa LCD
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    // Crea semáforos
    semDecimas = xSemaphoreCreateMutex();
    semParciales = xSemaphoreCreateMutex();
    if (!semDecimas || !semParciales) {
        ESP_LOGE("SEM", "Error creando semáforos");
    }

    // Crea paneles de dígitos
    PanelPPL.panel_minutes = CrearPanel(30 + OFFSET_X, 60, 2,
                                        DIGITO_ALTO, DIGITO_ANCHO,
                                        DIGITO_ENCENDIDO, DIGITO_APAGADO,
                                        DIGITO_FONDO);
    PanelPPL.panel_seconds = CrearPanel(170 + OFFSET_X, 60, 2,
                                        DIGITO_ALTO, DIGITO_ANCHO,
                                        DIGITO_ENCENDIDO, DIGITO_APAGADO,
                                        DIGITO_FONDO);
    PanelPPL.panel_decimas = CrearPanel(310 + OFFSET_X, 60, 2,
                                        DIGITO_ALTO, DIGITO_ANCHO,
                                        DIGITO_ENCENDIDO, DIGITO_APAGADO,
                                        DIGITO_FONDO);

    // Configura pines LEDs
    gpio_set_direction(LED_ROJO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_AZUL, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_ROJO, 1);
    gpio_set_level(LED_VERDE, 1);
    gpio_set_level(LED_AZUL, 1);

    ButtonEvents_Init();

    // Configura pines botones
    // gpio_set_direction(PB1, GPIO_MODE_INPUT);
    // gpio_pullup_en(PB1);
    // gpio_set_direction(PB2, GPIO_MODE_INPUT);
    // gpio_pullup_en(PB2);
    // gpio_set_direction(PB3, GPIO_MODE_INPUT);
    // gpio_pullup_en(PB3);

    // Crea tareas
    xTaskCreateStatic(decimasTask, "DecimasTask", STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 3,
                      xDecimasStack, &xDecimasTaskBuffer);
    xTaskCreateStatic(displayTask, "DisplayTask", STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 1,
                      xDisplayStack, &xDisplayTaskBuffer);
    xTaskCreateStatic(toggleTask, "ToggleTask", STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 2,
                      xToggleStack, &xToggleTaskBuffer);
    xTaskCreateStatic(LedStatusTask, "LedStatusTask", STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 1,
                      xLedStatusStack, &xLedStatusTaskBuffer);


    
    xTaskCreateStatic(ButtonEvents_Task, "BtnEvt", STACK_SIZE, NULL,
                     tskIDLE_PRIORITY+2, 
                     xButtonEventsStatusStack, &xButtonEventsTaskBuffer);                  

    // Termina tarea principal
    vTaskDelete(NULL);
}
