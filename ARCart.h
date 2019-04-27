/**
 * @file ARCart.h
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief Firmware for AR Cart 
 */

#ifndef ARCART_h
#define ARCART_h

#include <stdint.h>
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define AR_DEBUG		(1)

#ifdef AR_DEBUG
#define debug_print(x)			Serial.print((x))
#else
#define debug_print(x)
#endif

#ifdef AR_DEBUG
#define debug_println(x)			Serial.println((x))
#else
#define debug_println(x)
#endif

// Pins
#define LED_PIN						(23)

#define LFM_IN1_PIN					(4)		// Left front motor
#define LFM_IN2_PIN					(16)
#define LFM_ENC_PIN					(27)

#define LRM_IN1_PIN					(5)		// Left rear motor
#define LRM_IN2_PIN					(17)
#define LRM_ENC_PIN					(26)

#define RFM_IN1_PIN					(12)	// Right front motor
#define RFM_IN2_PIN					(13)
#define RFM_ENC_PIN					(33)

#define RRM_IN1_PIN					(2)		// Right rear motor
#define RRM_IN2_PIN					(15)
#define RRM_ENC_PIN					(25)

// Task setup
#define MOTOR_TASK_STACK_SIZE		(500)
#define ESTIMATOR_TASK_STACK_SIZE	(500)
#define NETWORK_TASK_STACK_SIZE		(2000)

#define NETWORK_TASK_PRIORITY		(2)
#define MOTOR_TASK_PRIORITY			(2)
#define ESTIMATOR_TASK_PRIORITY		(3)


/**
 * @brief Command information
 */
typedef struct motor_command_t
{
	float lfm;
	float lrm;
	float rfm;
	float rrm;
} motor_command_t;

void start_cart(uint32_t cart_number_set);

uint32_t get_cart_number(void);

void motor_task(void * param);
void estimator_task(void * param);
void network_task(void * param);

#endif