#include <math.h>
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "morse_queue.c"
#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX 1

#define SAMPLE_MS 10      // 100 Hz
#define PRINT_EVERY 100   // print once per second
#define SETTLE_SAMPLES 50 // ~0.5 s discarded while the gyro starts up
#define CAL_SAMPLES 300   // ~3 s bias calibration
#define DEADBAND_DPS 0.3f // |rate| below this counts as "not rotating"
#define BIAS_ALPHA 0.001f // bias follow speed (~10 s time constant)

#define DEADZONE 15 // TODO: VAIHDA

enum state { WAITING = 1, DATA_READY = 2 };
enum state programState = WAITING;

enum viivapiste { PISTE = 0, VIIVA = 1 };

float ax, ay, az, gx, gy, gz, t;

MORSE_QUEUE queue;

static void sensor_task(void *arg) {
  (void)arg;

  init_veml6030();
  init_display();
  clear_display();
  if (init_ICM42670() == 0) {
    printf("ICM-42671P initialized successfully\n");
    int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT,
                                   ICM42670_GYRO_FSR_DEFAULT);
    printf("Gyro return: %d\n", _gyro);
    int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
    printf("Enable gyro: %d\n", _enablegyro);
  } else {
    printf("Failed to initialize ICM-42671P\n");
  }

  // 1) Throw away the first samples while the gyro settles
  for (int i = 0; i < SETTLE_SAMPLES; i++) {
    ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
  }

  // 2) Measure the zero-rate offset (board must be perfectly still)
  printf("Calibrating - keep the board still...\n");
  float bias_z = 0.0f;
  int good = 0;
  for (int i = 0; i < CAL_SAMPLES; i++) {
    if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
      bias_z += gz;
      good++;
    }
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
  }
  if (good > 0)
    bias_z /= good;
  printf("Gyro Z bias: %.4f deg/s\n", bias_z);

  float angle_deg = 0.0f;
  int print_cnt = 0;
  int lcd_cnt = 0;  // refresh counter
  char lcd_buf[24]; // buffer for lcd display
  uint64_t prev_us = time_us_64();

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));

    if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
      continue; // bad read: skip it, don't integrate garbage
    }

    uint64_t now = time_us_64();
    float dt = (now - prev_us) * 1e-6f;
    prev_us = now;

    float rate = gz - bias_z; // deg/s with the offset removed

    if (fabsf(rate) < DEADBAND_DPS) {
      bias_z += BIAS_ALPHA * rate; // standing still: let bias follow slow drift
      rate = 0.0f;                 // and don't integrate noise
    }

    angle_deg += rate * dt;
    if (angle_deg < 0.0f)
      angle_deg += 360.0f;
    if (angle_deg >= 360.0f)
      angle_deg -= 360.0f;

    if (++lcd_cnt >= 10) { // every 10 samples = 10 Hz
      lcd_cnt = 0;
      // while (!isFull(&queue)) {
        if (angle_deg > DEADZONE && angle_deg < 90) {
          snprintf(lcd_buf, sizeof(lcd_buf), "Angle: %6.1f PISTE", angle_deg);
          enqueue(&queue, PISTE);
      clear_display();
      write_text_xy(0, 0, lcd_buf);
        } else if (angle_deg < 360 - DEADZONE && angle_deg > 270) {
          snprintf(lcd_buf, sizeof(lcd_buf), "Angle: %6.1f VIIVA", angle_deg);
          enqueue(&queue, VIIVA);
      clear_display();
      write_text_xy(0, 0, lcd_buf);
        } else if (angle_deg < DEADZONE || angle_deg > 360 - DEADZONE) {
          printQueue(&queue);
      clear_display();
      write_text_xy(0, 0, lcd_buf);
        }
      // }
    }
  }
}

int main() {

  stdio_init_all();

  init_hat_sdk();
  sleep_ms(300); // Wait some time so initialization of USB and hat is done.

  init_button1();
  init_led();

  TaskHandle_t hSensorTask = NULL;

  // Create the tasks with xTaskCreate
  BaseType_t result = xTaskCreate(
      sensor_task,        // (en) Task function
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
