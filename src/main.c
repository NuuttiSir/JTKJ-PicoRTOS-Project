#include <hardware/gpio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "projdefs.h"
#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX 1

#define WAITING_TRESHOLD_DPS 10
// Katso ideas.md missa puhutaan vasen oikea heilautuksista
#define DATA_REGISTER_TRESHOLD_DPS 200

typedef enum { WAITING, GOT_DATA } State;
State programState = GOT_DATA;

float ax, ay, az, gx, gy, gz, t;

#define MAX_MORSE_SIZE 5
#define TEXT_MAX 100
#define CONFIRM_CHAR (1u << 0)  // Unsigned ints
#define CONFIRM_SPACE (1u << 1) // Unsigned ints

typedef struct {
  const char *morse;
  char character;
} MorseCode;

const char dot = '.';
const char dash = '-';

static TaskHandle_t hSensorTask = NULL;

static void btn_fxn(uint gpio, uint32_t eventMask) {
  uint32_t bit = (gpio == BUTTON1) ? CONFIRM_CHAR : CONFIRM_SPACE;
  BaseType_t woken = pdFALSE;
  xTaskNotifyFromISR(hSensorTask, bit, eSetBits, &woken);
  portYIELD_FROM_ISR(woken);
}
static void gyro_task(void *arg) {
  static const MorseCode morseLookupTable[] = {
      {".-", 'A'},    {"-...", 'B'},  {"-.-.", 'C'},  {"-..", 'D'},
      {".", 'E'},     {"..-.", 'F'},  {"--.", 'G'},   {"....", 'H'},
      {"..", 'I'},    {".---", 'J'},  {"-.-", 'K'},   {".-..", 'L'},
      {"--", 'M'},    {"-.", 'N'},    {"---", 'O'},   {".--.", 'P'},
      {"--.-", 'Q'},  {".-.", 'R'},   {"...", 'S'},   {"-", 'T'},
      {"..-", 'U'},   {"...-", 'V'},  {".--", 'W'},   {"-..-", 'X'},
      {"-.--", 'Y'},  {"--..", 'Z'},  {"-----", '0'}, {".----", '1'},
      {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'},
      {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
  };

  static char symbols[MAX_MORSE_SIZE + 1] = {0};
  static int symLen = 0;

  static char text[TEXT_MAX + 1] = {0};
  static int textLen = 0;

  if (init_ICM42670() < 0) {
    printf("Init for IMU failed");
    vTaskDelete(NULL);
  }
  if (ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT) <
      0) {
    printf("Init for gyro failed");
    vTaskDelete(NULL);
  }
  if (ICM42670_enable_accel_gyro_ln_mode() < 0) {
    printf("Init for gyro accel in ln failed");
    vTaskDelete(NULL);
  }

  gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true,
                                     btn_fxn);
  gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_RISE, true);

  for (;;) {
    bool redraw = false;
    ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);
    switch (programState) {
    case WAITING:
      if (gz > DATA_REGISTER_TRESHOLD_DPS) {
        if (symLen < MAX_MORSE_SIZE) {
          symbols[symLen++] = dash;
          symbols[symLen] = '\0';
          redraw = true;
        }
        programState = GOT_DATA;
      } else if (gz < -DATA_REGISTER_TRESHOLD_DPS) {
        if (symLen < MAX_MORSE_SIZE) {
          symbols[symLen++] = dot;
          symbols[symLen] = '\0';
          redraw = true;
        }
        programState = GOT_DATA;
      }
      break;
    case GOT_DATA:
      if (gz < WAITING_TRESHOLD_DPS && gz > -WAITING_TRESHOLD_DPS) {
        programState = WAITING;
      }
      break;
    }

    uint32_t bits;
    if (xTaskNotifyWait(0, 0xFFFFFFFFu, &bits, 0) == pdTRUE) {
      if (bits & CONFIRM_CHAR) {
        for (size_t i = 0;
             i < sizeof(morseLookupTable) / sizeof(morseLookupTable[0]); i++) {
          if (strcmp(symbols, morseLookupTable[i].morse) == 0 &&
              textLen < TEXT_MAX) {
            text[textLen++] = morseLookupTable[i].character;
            text[textLen] = '\0';
            break;
          }
        }
        symLen = 0;
        symbols[0] = '\0';
        redraw = true;
      }
      if (bits & CONFIRM_SPACE) {
        if (textLen < TEXT_MAX) {
          text[textLen++] = ' ';
          text[textLen] = '\0';
          redraw = true;
        }
      }
    }
    // Redraw
    if (redraw) {
      clear_display();
      write_text_xy(0, 0, symbols);
      write_text_xy(0, 12, "----------");
      write_text_xy(0, 24, text);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

int main() {

  stdio_init_all();

  init_hat_sdk();
  sleep_ms(300); // Wait some time so initialization of USB and hat is done.

  init_button1();
  init_button2();
  init_display();

  // Create the tasks with xTaskCreate
  BaseType_t result = xTaskCreate(
      gyro_task,          // (en) Task function
      "sensor",           // (en) Name of the task
      DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words).
                          // Generally 1024 or 2048
      NULL,               // (en) Arguments of the task
      2,                  // (en) Priority of this task
      &hSensorTask);      // (en) A handle to control the execution of this task

  if (result != pdPASS) {
    printf("Sensor task creation failed\n");
    return 0;
  }

  // Start the scheduler (never returns)
  vTaskStartScheduler();

  // Never reach this line.
  return 0;
}
