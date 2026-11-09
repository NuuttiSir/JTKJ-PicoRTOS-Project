#include <hardware/gpio.h>
#include <math.h>
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
State programState = WAITING;

float ax, ay, az, gx, gy, gz, t;

#define MAX_MORSE_QUEUE_SIZE 5

// typedef struct {
//   char morse[MAX_MORSE_QUEUE_SIZE];
//   char character;
// } MorseCode;
//
// typedef enum {
//   SIGNAL_NONE,
//   SIGNAL_DOT,
//   SIGNAL_DASH,
//   SIGNAL_SPACE,
//   SIGNAL_END
// } MorseSignal;

// static const MorseCode morseLookupTable[] = {
//   {".-", 'A'},    {"-...", 'B'},  {"-.-.", 'C'},  {"-..", 'D'},
//   {".", 'E'},     {"..-.", 'F'},  {"--.", 'G'},   {"....", 'H'},
//   {"..", 'I'},    {".---", 'J'},  {"-.-", 'K'},   {".-..", 'L'},
//   {"--", 'M'},    {"-.", 'N'},    {"---", 'O'},   {".--.", 'P'},
//   {"--.-", 'Q'},  {".-.", 'R'},   {"...", 'S'},   {"-", 'T'},
//   {"..-", 'U'},   {"...-", 'V'},  {".--", 'W'},   {"-..-", 'X'},
//   {"-.--", 'Y'},  {"--..", 'Z'},  {"-----", '0'}, {".----", '1'},
//   {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'},
//   {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
// };

QueueHandle_t xMorseQueue;

static void btn_fxn(uint gpio, uint32_t eventMask) {
  char morseBuf[MAX_MORSE_QUEUE_SIZE + 1];
  int bufCounter = 0;
  while (bufCounter < MAX_MORSE_QUEUE_SIZE &&
         xQueueReceive(xMorseQueue, &morseBuf[bufCounter], 0) == pdTRUE) {
    bufCounter++;
  }
  morseBuf[bufCounter] = '\0';
  printf("%s\n", morseBuf);
}

static void gyro_task(void *arg) {
  if (init_ICM42670() < 0) {
    printf("Init for IMU failed");
    return;
  }
  if (ICM42670_startGyro(ICM42670_ACCEL_ODR_DEFAULT,
                         ICM42670_GYRO_FSR_DEFAULT) < 0) {
    printf("Init for gyro failed");
    return;
  }
  if (ICM42670_enable_accel_gyro_ln_mode() < 0) {
    printf("Init for gyro accel in ln failed");
    return;
  }

  xMorseQueue = xQueueCreate(MAX_MORSE_QUEUE_SIZE, sizeof(char));
  if (xMorseQueue == NULL) {
    printf("Queue was not created and must not be used");
    return;
  }

  gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true,
                                     btn_fxn);

  const char dot = '.';
  const char dash = '-';

  for (;;) {
    ICM42670_read_sensor_data(0, 0, 0, 0, 0, &gz, 0);
    switch (programState) {
    case WAITING:
      if (gz > 200) {
        if (xQueueIsQueueFullFromISR(xMorseQueue)) {
          printf("Queue is full\n");
          programState = GOT_DATA;
        }
        xQueueSendToBack(xMorseQueue, &dash, 0);
        programState = GOT_DATA;
      }
      if (gz < -200) {
        if (xQueueIsQueueFullFromISR(xMorseQueue)) {
          printf("Queue is full\n");
          programState = GOT_DATA;
        }
        xQueueSendToBack(xMorseQueue, &dot, 0);
        programState = GOT_DATA;
      }
      break;
    case GOT_DATA:
      if (gz < 10 && gz > -10) {
        programState = WAITING;
      }
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// static void sensor_task(void *arg) {
//   (void)arg;
//
//   init_veml6030();
//   init_display();
//   clear_display();
//   if (init_ICM42670() == 0) {
//     printf("ICM-42671P initialized successfully\n");
//     int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT,
//                                    ICM42670_GYRO_FSR_DEFAULT);
//     printf("Gyro return: %d\n", _gyro);
//     int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
//     printf("Enable gyro: %d\n", _enablegyro);
//   } else {
//     printf("Failed to initialize ICM-42671P\n");
//   }
//
//   // 1) Throw away the first samples while the gyro settles
//   for (int i = 0; i < SETTLE_SAMPLES; i++) {
//     ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);
//     vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
//   }
//
//   // 2) Measure the zero-rate offset (board must be perfectly still)
//   printf("Calibrating - keep the board still...\n");
//   float bias_z = 0.0f;
//   int good = 0;
//   for (int i = 0; i < CAL_SAMPLES; i++) {
//     if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
//       bias_z += gz;
//       good++;
//     }
//     vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
//   }
//   if (good > 0)
//     bias_z /= good;
//   printf("Gyro Z bias: %.4f deg/s\n", bias_z);
//
//   float angle_deg = 0.0f;
//   int print_cnt = 0;
//   int lcd_cnt = 0;  // refresh counter
//   char lcd_buf[24]; // buffer for lcd display
//   uint64_t prev_us = time_us_64();
//
//   for (;;) {
//     vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
//
//     if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
//       continue; // bad read: skip it, don't integrate garbage
//     }
//
//     uint64_t now = time_us_64();
//     float dt = (now - prev_us) * 1e-6f;
//     prev_us = now;
//
//     float rate = gz - bias_z; // deg/s with the offset removed
//
//     if (fabsf(rate) < DEADBAND_DPS) {
//       bias_z += BIAS_ALPHA * rate; // standing still: let bias follow slow
//       drift rate = 0.0f;                 // and don't integrate noise
//     }
//
//     angle_deg += rate * dt;
//     if (angle_deg < 0.0f)
//       angle_deg += 360.0f;
//     if (angle_deg >= 360.0f)
//       angle_deg -= 360.0f;
//
//     if (++lcd_cnt >= 10) { // every 10 samples = 10 Hz
//       lcd_cnt = 0;
//       // while (!isFull(&queue)) {
//       if (angle_deg > DEADZONE && angle_deg < 90) {
//         snprintf(lcd_buf, sizeof(lcd_buf), "Angle: %6.1f PISTE",
//         angle_deg); clear_display(); write_text_xy(0, 0, lcd_buf);
//       } else if (angle_deg < 360 - DEADZONE && angle_deg > 270) {
//         snprintf(lcd_buf, sizeof(lcd_buf), "Angle: %6.1f VIIVA",
//         angle_deg); clear_display(); write_text_xy(0, 0, lcd_buf);
//       } else if (angle_deg < DEADZONE || angle_deg > 360 - DEADZONE) {
//         clear_display();
//         write_text_xy(0, 0, lcd_buf);
//       }
//       // }
//     }
//   }
// }

int main() {

  stdio_init_all();

  init_hat_sdk();
  sleep_ms(300); // Wait some time so initialization of USB and hat is done.

  init_button1();
  init_led();

  TaskHandle_t hSensorTask = NULL;

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
