/*
 * wifi.h
 *
 *  Created on: Jan 3, 2019
 *      Author: Marcus
 */

#ifndef INC_WIFI_H_
#define INC_WIFI_H_

// This is what we think is the error code for the SimpleLink driver is,
// it is in the documentation, but it is not declared
#define SL_EAGAIN -11
#define PROVISIONING_INACTIVITY_TIMEOUT 1800
#define NWP_STOP_TIMEOUT 1000

// Uncomment this line to use our SSID for the Access Point
//#define USE_NCIR_SSID

void wifi_init();
void wifiStartWLANProvisioning();
int32_t wifiProvisioning();

#endif /* INC_WIFI_H_ */
