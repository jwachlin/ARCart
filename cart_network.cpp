/**
 * @file cart_network.cpp
 * @author Jacob Wachlin
 * @date Feb 2019
 * @brief Network interface for AR Cart 
 */

#include <WiFi.h>
#include <esp_now.h>
#include "esp_timer.h"
#include <stdint.h>
#include <string.h>

#include "ARCart.h"
#include "freertos/queue.h"
#include "nvs_flash.h"

#include "cart_network.h"
#include "motor_control.h"

#define WIFI_CHANNEL                        (1)
#define NETWORK_COMMAND_QUEUE_SIZE          (5)

static esp_timer_create_args_t tele_args;
static esp_timer_handle_t tele_handle;
static QueueHandle_t network_command_queue;

// this is the MAC Address of the ESP interface with computer
static uint8_t remoteMac[] = {0xA0, 0xAA, 0xFC, 0x3F, 0xC8, 0x23};
static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void handle_error(esp_err_t err)
{
    switch (err)
    {
        case ESP_ERR_ESPNOW_NOT_INIT:
            Serial.println("Not init");
            break;
        
        case ESP_ERR_ESPNOW_ARG:
            Serial.println("Argument invalid");
            break;

        case ESP_ERR_ESPNOW_INTERNAL:
            Serial.println("Internal error");
            break;

        case ESP_ERR_ESPNOW_NO_MEM:
            Serial.println("Out of memory");
            break;

        case ESP_ERR_ESPNOW_NOT_FOUND:
            Serial.println("Peer is not found");
            break;

        case ESP_ERR_ESPNOW_IF:
            Serial.println("Current WiFi interface doesn't match that of peer");
            break;

        default:
            break;
    }
}

// ESP docs says this runs from high priority WiFi task, so avoid lengthy operations
// Does not seem to require ISR versions of FreeRTOS functions
static void msg_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if(len < 1){return;}
    // TODO check who its from?

    if(*data == MSG_CONTROLLER_INPUT && len == (sizeof(controller_input_t)+1))
    {
        controller_input_t command;
        memcpy(&command,(data+1),sizeof(controller_input_t));
        if(command.address == get_cart_number())
        {
            xQueueSend(network_command_queue, &command,0);
        }
    }
}

static void msg_send_cb(const uint8_t* mac, esp_now_send_status_t sendStatus)
{
    // TODO do anything here?
    //Serial.printf("send_cb, send done, status = %i\n", sendStatus);
}

bool network_init(void)
{       
    //Puts ESP in STATION MODE
    nvs_flash_init();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != 0)
    {
        return false;
    }

    network_command_queue = xQueueCreate(NETWORK_COMMAND_QUEUE_SIZE, sizeof(controller_input_t));

    esp_now_peer_info_t peer_info;
    peer_info.channel = WIFI_CHANNEL;
    memcpy(peer_info.peer_addr,broadcast_mac,6);
    peer_info.ifidx = ESP_IF_WIFI_STA;
    peer_info.encrypt = false;
    esp_err_t status = esp_now_add_peer(&peer_info);
    if(ESP_OK != status)
    {
        Serial.println("Could not add peer");
        handle_error(status);
    }

    // Set up receive callback
    status = esp_now_register_recv_cb(msg_recv_cb);
    if(ESP_OK != status)
    {
        Serial.println("Could not register receive callback");
        handle_error(status);
    }

    status = esp_now_register_send_cb(msg_send_cb);
    if(ESP_OK != status)
    {
        Serial.println("Could not register send callback");
        handle_error(status);
    }

    Serial.println("Network set up");

    return true;
}

void vTelemetryCallback( void * args  )
{
    cart_telemetry_t telemetry;

    telemetry.cart_number = get_cart_number();
    get_motor_command(&telemetry.motor_command);
    get_speed(&telemetry.speed);

    send_telemetry(&telemetry);
}

void send_telemetry(cart_telemetry_t * telemetry)
{
    // Pack
    uint16_t packet_size = sizeof(cart_telemetry_t)+1;
    uint8_t msg[packet_size];
    msg[0] = MSG_TELEMETRY;
    memcpy(&msg[1], telemetry, sizeof(cart_telemetry_t));

    esp_now_send(broadcast_mac, msg, packet_size); // send to all
}

void network_task(void * param)
{
    controller_input_t controller_command;
    
    // Set up telemetry timer.
    // Note: FreeRTOS timers were starving IDLE task for some reason...
    tele_args.callback = vTelemetryCallback;
    tele_args.dispatch_method = ESP_TIMER_TASK;
    tele_args.name = "tele";

    esp_timer_create(&tele_args, &tele_handle);
    esp_timer_start_periodic(tele_handle, 1000000); // in microseconds

    for(;;)
    {
        if(xQueueReceive(network_command_queue, &controller_command, portMAX_DELAY) == pdPASS)
        {

            // Handle command in motor controller task
            debug_print("Command Received, left stick up/down: ");
            debug_println(controller_command.left_stick_ud);
            debug_print("Command Received, left stick left/right: ");
            debug_println(controller_command.left_stick_lr);

            handle_controller_input(&controller_command);
            
        }
    }
}