/**
 * @file motor_control.h
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief motor control interface for AR Cart 
 */

#ifndef MOTOR_CONTROL_h
#define MOTOR_CONTROL_h

#include "ARCart.h"
#include "cart_network.h"

void motor_init(void);
void set_motor_command(motor_command_t * cmd);
void get_motor_command(motor_command_t * cmd);
void handle_controller_input(controller_input_t * cmd);

#endif