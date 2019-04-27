/**
 * @file ARCart.cpp
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief Firmware for AR Cart 
 */

#include <Arduino.h>

#include "esp_timer.h"

#include "motor_control.h"
#include "speed_estimator.h"
#include "cart_network.h"

#include "ARCart.h"

/*
	TODO
		-Mesh networking setup
			-Handle commands, build network 
		-
*/

static uint32_t cart_number;


void start_cart(uint32_t cart_number_set)
{
	//#ifdef AR_DEBUG
	Serial.begin(115200);
	delay(10);
	//#endif
	
	cart_number = cart_number_set;

	// Set up timer
	esp_timer_init();

	network_init();
	motor_init();
	speed_estimator_init();

	debug_println("Booting...");

	/*
	*	Use pinned to core versions here since we may use floating point math
	*	Should also avoid using floats in ISRs
	*	See: https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/freertos-smp.html#floating-point-aritmetic
	*/
	xTaskCreatePinnedToCore(	 motor_task,
	 				"motor",
					 MOTOR_TASK_STACK_SIZE,
					 NULL,
					 MOTOR_TASK_PRIORITY,
					 NULL,
					 0);

	xTaskCreatePinnedToCore(	 estimator_task,
	 				"est",
					 ESTIMATOR_TASK_STACK_SIZE,
					 NULL,
					 ESTIMATOR_TASK_PRIORITY,
					 NULL,
					 0);

	xTaskCreatePinnedToCore(	 network_task,
	 				"net",
					 NETWORK_TASK_STACK_SIZE,
					 NULL,
					 NETWORK_TASK_PRIORITY,
					 NULL,
					 0);

	vTaskStartScheduler();
}

uint32_t get_cart_number(void)
{
	return cart_number;
}