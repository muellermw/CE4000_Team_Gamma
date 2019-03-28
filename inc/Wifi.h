/**
 * This header file utilizes TI and other custom firmware to implement WiFi provisioning
 * and general WiFi event control flow. wifi_init is called by the main program at
 * startup, and does not return until the board has connected to an AP.
 * @file wifi.h
 * @date 3/10/2019
 * @author: Marcus Mueller
 */

#ifndef INC_WIFI_H_
#define INC_WIFI_H_

// This is what we think is the error code for the SimpleLink driver is,
// it is in the documentation, but it is not declared
#define SL_EAGAIN -11
#define PROVISIONING_INACTIVITY_TIMEOUT 3600
#define NWP_STOP_TIMEOUT 1000
#define DEVICE_NAME_LENGTH 33 // +1 for NULL char
#define DEVICE_SSID_LENGTH 32
#define DEFAULT_DEVICE_NAME "mysimplelink"

// Uncomment this line to use our SSID for the Access Point
//#define USE_NCIR_SSID

void wifi_init();
int32_t simplelink_init(uint8_t const role);

#endif /* INC_WIFI_H_ */
