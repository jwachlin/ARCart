
/**
 * @file motor_control.cpp
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief motor control interface for AR Cart 
 */

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "esp_timer.h"

#include "ARCart.h"

#include "motor_control.h"

#define MOTOR_PWM_CHANNEL           (0)
#define MOTOR_FREQ                  (5000)
#define MOTOR_RES_BITS              (8)

static esp_timer_create_args_t motor_timer_args;
static esp_timer_handle_t motor_timer_handle;

static esp_timer_create_args_t motor_stop_timer_args;
static esp_timer_handle_t motor_stop_timer_handle;

static motor_command_t command;
static volatile uint8_t recent_command_received = 0;

/**
 * @brief Sets up ESP32 PWM for motor control
 */
void motor_init(void)
{
    // Motor left front
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LFM_IN1_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, LFM_IN2_PIN);

    // Motor left rear
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, LRM_IN1_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1B, LRM_IN2_PIN);
    
    // Motor right front
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2A, RFM_IN1_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2B, RFM_IN2_PIN);

    // Motor right rear
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, RRM_IN1_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, RRM_IN2_PIN);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = MOTOR_FREQ;    
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config);
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_2, &pwm_config);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);

    debug_println("Motor set up");
}

/**
 * @brief motor moves in forward or reverse, with duty cycle = duty %
 */
static void brushed_motor(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    if(duty_cycle > 0.0)
    {
        mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
        mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
        mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
    }
    else
    {
        mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
        mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);
        mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
    }
}


void vMotorControlCallback( void * args )
{
    // Sets the motor outputs to the command

    // Left front motor
    brushed_motor(MCPWM_UNIT_0, MCPWM_TIMER_0,command.lfm);
    // Left rear motor
    brushed_motor(MCPWM_UNIT_0, MCPWM_TIMER_1,command.lrm);
    // Right front motor
    brushed_motor(MCPWM_UNIT_0, MCPWM_TIMER_2,command.rfm);
    // Right rear motor
    brushed_motor(MCPWM_UNIT_1, MCPWM_TIMER_0,command.rrm);
}

void vMotorStopCallback( void * args )
{
    if(recent_command_received == 0)
    {
        // it has been a while since a command has been received, so set to zero 
        command.lfm = 0.0;
        command.lrm = 0.0;
        command.rfm = 0.0;
        command.rrm = 0.0;
    }
    recent_command_received = 0;
}

void motor_task(void * param)
{
    // Set up motor timer.
    // Note: FreeRTOS timers were starving IDLE task for some reason...
    motor_timer_args.callback = vMotorControlCallback;
    motor_timer_args.dispatch_method = ESP_TIMER_TASK;
    motor_timer_args.name = "speed";

    esp_timer_create(&motor_timer_args, &motor_timer_handle);
    esp_timer_start_periodic(motor_timer_handle, 5000); // in microseconds

    // Set up timer to set commands to zero if commands are stale 
    motor_stop_timer_args.callback = vMotorStopCallback;
    motor_stop_timer_args.dispatch_method = ESP_TIMER_TASK;
    motor_stop_timer_args.name = "mstop";

    // Causes consistent core 0 panic
    //esp_timer_create(&motor_stop_timer_args, &motor_stop_timer_handle);
    //esp_timer_start_periodic(motor_stop_timer_handle, 500000); // in microseconds

    for(;;)
    {
        // Don't do anything here
        vTaskDelay(portMAX_DELAY);
    }
}

void set_motor_command(motor_command_t * cmd)
{
    command = *cmd;
}

void get_motor_command(motor_command_t * cmd)
{
    *cmd = command;
}

void handle_controller_input(controller_input_t * cmd)
{
    // Map to control inputs. For now, basic tank driver
    motor_command_t motor_cmd;

    motor_cmd.lfm = cmd->left_stick_ud;
    motor_cmd.lrm = cmd->left_stick_ud;

    motor_cmd.rfm = cmd->right_stick_ud;
    motor_cmd.rrm = cmd->right_stick_ud;
    
    set_motor_command(&motor_cmd);

    recent_command_received = 1;
}