/**
 * @file speed_estimator.cpp
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief speed estimator for AR Cart 
 */
#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_timer.h"

#include "ARCart.h"

#include "speed_estimator.h"

static esp_timer_create_args_t est_timer_args;
static esp_timer_handle_t est_timer_handle;

static volatile cart_speed_t est_speed;

static volatile uint32_t lfm_ticks;
static volatile uint32_t lrm_ticks;
static volatile uint32_t rfm_ticks;
static volatile uint32_t rrm_ticks;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// TODO debounce needed?
void IRAM_ATTR handleLFMInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  lfm_ticks++;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR handleLRMInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  lrm_ticks++;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR handleRFMInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  rfm_ticks++;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR handleRRMInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  rrm_ticks++;
  portEXIT_CRITICAL_ISR(&mux);
}

void speed_estimator_init(void)
{
    // Turn on LED at correct frequency, square wave, half duty cycle
    ledcSetup(0, 40000, 8);     // Receiver tuned to 40khz
    ledcAttachPin(LED_PIN, 0);
    ledcWrite(0, 128);

    pinMode(LFM_ENC_PIN, INPUT_PULLUP);
    pinMode(LRM_ENC_PIN, INPUT_PULLUP);
    pinMode(RFM_ENC_PIN, INPUT_PULLUP);
    pinMode(RRM_ENC_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(LFM_ENC_PIN), handleLFMInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(LRM_ENC_PIN), handleLRMInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(RFM_ENC_PIN), handleRFMInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(RRM_ENC_PIN), handleRRMInterrupt, FALLING);
}

void vEstimatorCallback( void * args )
{
    static uint32_t last_lfm_ticks, last_lrm_ticks, last_rfm_ticks, last_rrm_ticks;

    uint32_t lfm_change, lrm_change, rfm_change, rrm_change;
    // Wrap in critical section
    lfm_change = lfm_ticks - last_lfm_ticks;
    lrm_change = lrm_ticks - last_lrm_ticks;
    rfm_change = rfm_ticks - last_rfm_ticks;
    rrm_change = rrm_ticks - last_rrm_ticks;
    
    last_lfm_ticks = lfm_ticks;
    last_lrm_ticks = lrm_ticks;
    last_rfm_ticks = rfm_ticks;
    last_rrm_ticks = rrm_ticks;

    // Calculate outside of critical section

    // Low pass filter change
    // TODO 
    est_speed.lfm = 0.7 * est_speed.lfm + 0.3 * lfm_change;
    est_speed.lrm = 0.7 * est_speed.lrm + 0.3 * lrm_change;
    est_speed.rfm = 0.7 * est_speed.rfm + 0.3 * rfm_change;
    est_speed.rrm = 0.7 * est_speed.rrm + 0.3 * rrm_change;
    est_speed.avg = 0.25 * (est_speed.lfm + est_speed.lrm + est_speed.rfm + est_speed.rrm);
}

void estimator_task(void * param)
{
    // Set up estimator timer.
    // Note: FreeRTOS timers were starving IDLE task for some reason...
    est_timer_args.callback = vEstimatorCallback;
    est_timer_args.dispatch_method = ESP_TIMER_TASK;
    est_timer_args.name = "speed";

    esp_timer_create(&est_timer_args, &est_timer_handle);
    esp_timer_start_periodic(est_timer_handle, 10000); // in microseconds

    for(;;)
    {
        // Don't do anything here
        vTaskDelay(portMAX_DELAY);
    }
}

void get_speed(cart_speed_t * speed)
{
    speed->lfm = est_speed.lfm;
    speed->lrm = est_speed.lrm;
    speed->rfm = est_speed.rfm;
    speed->rrm = est_speed.rrm;
    speed->avg = est_speed.avg;
}