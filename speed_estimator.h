/**
 * @file speed_estimator.h
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief speed estimator for AR Cart 
 */

#ifndef SPEED_ESTIMATOR_h
#define SPEED_ESTIMATOR_h

#include "ARCart.h"

/**
 * @brief Speed information. Note no direction information possible
 */
typedef struct cart_speed_t
{
	float lfm;
	float lrm;
	float rfm;
	float rrm;
	float avg;
} cart_speed_t;

void speed_estimator_init(void);
void get_speed(cart_speed_t * speed);

#endif