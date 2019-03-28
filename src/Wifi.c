/**
 * This file utilizes TI and other custom firmware to implement WiFi provisioning
 * and general WiFi event control flow. wifi_init is called by the main program at
 * startup, and does not return until the board has connected to an AP.
 * @file Wifi.c
 * @date 3/10/2019
 * @author: Marcus Mueller
 */

#include <stdio.h>
#include <string.h>
// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include "Wifi.h"
#include "Board.h"
#include "Misc_Timer.h"

#ifdef DEBUG_SESSION
#include "uart_term.h"
#endif

static int initSlDevice = 1;
static int wlanConnectToRouter = 1;

// Flag to signal user (phone app) provisioning
static int wlanNeedUserProvision = 0;
static int wlanConnectedToAP = 0;

// Flag to signal if board is undergoing a reset cycle - this cannot be interrupted!
static int boardRestarting = 0;
static uint8_t timeoutCount = 0;
static uint8_t timeoutMax = 20;

static void wifiStartWLANProvisioning();
static int32_t wifiProvisioning();
static void resetBoard();
static void WiFiProvisionTimeoutHandler(Timer_Handle handle);

/**
 * Initialize the WiFi subsystem and ensure a connection before returning
 */
void wifi_init()
{
    GPIO_init();
    SPI_init();

    // The NWP has not been initialized yet, do it now
    if (initSlDevice)
    {
        simplelink_init(0);
    }

    wlanConnectToRouter = 1;
    wlanNeedUserProvision = 0;

    wifiStartWLANProvisioning();
}

/**
 * A simple state machine that allows user provisioning or immediate
 * connection to an AP, depending on startup conditions
 *
 * STATE MACHINE:
 * 1) Initialize the WiFi subsystem
 * 2) If a connection is available, establish a connection
 * 3) If a connection is not available, start user provisioning by creating an AP on the board
 */
void wifiStartWLANProvisioning()
{
    while (wlanConnectToRouter || wlanNeedUserProvision)
    {
        //The SimpleLink host driver architecture mandate calling 'sl_task' in
        //a NO-RTOS application's main loop.
        //The purpose of this call, is to handle asynchronous events and get
        //flow control information sent from the NWP.
        //Every event is classified and later handled by the host driver
        //event handlers.
        sl_Task(NULL);

        // If the WiFi subsystem determines that the board needs to be user provisioned, it is started here
        if (wlanNeedUserProvision)
        {
            wifiProvisioning();
        }
    }

    // Set the flag that tells the WiFi system that the board was at one point connected to an AP
    wlanConnectedToAP = 1;
}

/**
 * This method was created by TI and slightly modified for this application. It initializes
 * the WiFi subsystem into a functional configuration before returning.
 * @param role - the role to set the SimpleLink chip (typically 0 for host station)
 * @return status - 0 for OK, negative value for error
 */
int32_t simplelink_init(uint8_t const role)
{
    int32_t retVal = -1;

    if((retVal = sl_Start(0, 0, 0)) == SL_ERROR_RESTORE_IMAGE_COMPLETE)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("sl_Start Failed\r\n");
        UART_PRINT(
            "\r\n**********************************\r\n"
            "Return to Factory Default been Completed\r\nPlease RESET the Board\r\n"
            "**********************************\r\n");
#endif
        while(1){}
    }

    if(SL_RET_CODE_PROVISIONING_IN_PROGRESS == retVal)
    {
#ifdef DEBUG_SESSION
        UART_PRINT(" [ERROR] Provisioning is already running, stopping current session...\r\n");
#endif
        retVal = sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP, 0, 0, NULL, 0);
        retVal = sl_Start(0, 0, 0);
    }

    if(retVal == role)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("SimpleLinkInitCallback: started in role %d\r\n", retVal);
#endif
    }
    else
    {
#ifdef DEBUG_SESSION
        UART_PRINT("SimpleLinkInitCallback: started in role %d, set the requested role %d\r\n", retVal, role);
#endif
        retVal = sl_WlanSetMode(role);
        retVal = sl_Stop(NWP_STOP_TIMEOUT);
        retVal = sl_Start(0, 0, 0);

        if(retVal != role)
        {
#ifdef DEBUG_SESSION
            UART_PRINT("SimpleLinkInitCallback: error setting role %d, status=%d\r\n", role, retVal);
#endif
        }

#ifdef DEBUG_SESSION
        UART_PRINT("SimpleLinkInitCallback: restarted in role %d\r\n", role);
#endif
    }

    initSlDevice = 0;
    return(retVal);
}

/**
 * This method was created by TI and modified for this application. It sets up the WiFi subsystem
 * in order to initialize the provisioning process. Depending on defined constants, a different
 * WiFi access point SSID will be used to identify the board.
 * @return 0 on success, negative value on failure
 */
int32_t wifiProvisioning()
{
    int32_t retVal = 0;
    uint8_t configOpt = 0;
    uint16_t configLen = 0;
    SlDeviceVersion_t ver = {0};
    uint8_t simpleLinkMac[SL_MAC_ADDR_LEN];
    uint16_t macAddressLen;
    uint8_t provisioningCmd;

#ifdef DEBUG_SESSION
    UART_PRINT("\n\r\n\r\n\r==================================\n\r");
    UART_PRINT(" Provisioning WLAN \n\r");
    UART_PRINT("==================================\n\r");
#endif

    /* Get device's info */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DeviceGet(SL_DEVICE_GENERAL, &configOpt, &configLen, (uint8_t *)(&ver));

    if(SL_RET_CODE_PROVISIONING_IN_PROGRESS == retVal)
    {
#ifdef DEBUG_SESSION
        UART_PRINT(" [ERROR] Provisioning is already running, stopping current session...\r\n");
#endif
        return(0);
    }

#ifdef DEBUG_SESSION
    UART_PRINT(
        "\r\n CHIP 0x%x\r\n MAC  31.%d.%d.%d.%d\r\n PHY  %d.%d.%d.%d\r\n "
        "NWP%d.%d.%d.%d\r\n ROM  %d\r\n HOST %d.%d.%d.%d\r\n",
        ver.ChipId,
        ver.FwVersion[0],
        ver.FwVersion[1],
        ver.FwVersion[2],
        ver.FwVersion[3],
        ver.PhyVersion[0],
        ver.PhyVersion[1],
        ver.PhyVersion[2],
        ver.PhyVersion[3],
        ver.NwpVersion[0],
        ver.NwpVersion[1],
        ver.NwpVersion[2],
        ver.NwpVersion[3],
        ver.RomVersion,
        SL_MAJOR_VERSION_NUM,SL_MINOR_VERSION_NUM,SL_VERSION_NUM,
        SL_SUB_VERSION_NUM);
#endif

    macAddressLen = sizeof(simpleLinkMac);
    sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, NULL, &macAddressLen, (unsigned char *)simpleLinkMac);


    /**************************************************************************
     * CODE TO CHANGE THE ACCESS POINT SSID
     *************************************************************************/
    uint16_t devNameLen = DEVICE_NAME_LENGTH;
    char deviceName[devNameLen];
    memset(deviceName, 0, devNameLen);

    uint16_t devNameOpt = SL_WLAN_P2P_OPT_DEV_NAME;
    sl_WlanGet(SL_WLAN_CFG_P2P_PARAM_ID, &devNameOpt , &devNameLen, (uint8_t*)deviceName);

#ifdef DEBUG_SESSION
    UART_PRINT("\r\nDevice Name: %s\r\n", deviceName);
#endif

    char ncirSSID[DEVICE_SSID_LENGTH];

#ifdef USE_NCIR_SSID
    // The device name has not been set yet - default is "mysimplelink"
    if (strcmp(deviceName, (char *)DEFAULT_DEVICE_NAME) == 0)
    {
        snprintf(ncirSSID, DEVICE_SSID_LENGTH, "NCIR-%x%x%x", simpleLinkMac[3],
                                                              simpleLinkMac[4],
                                                              simpleLinkMac[5]);
    }
    // The new SSID format is NCIR-"device_name"
    else
    {
        snprintf(ncirSSID, DEVICE_SSID_LENGTH, "NCIR-%s", deviceName);
    }
#else
    // The device name has not been set yet - default is "mysimplelink"
    if (strcmp(deviceName, (char *)DEFAULT_DEVICE_NAME) == 0)
    {
        snprintf(ncirSSID, DEVICE_SSID_LENGTH, "mysimplelink-%x%x%x", simpleLinkMac[3],
                                                                      simpleLinkMac[4],
                                                                      simpleLinkMac[5]);
    }
    // The new SSID format is mysimplelink-"device_name"
    else
    {
        snprintf(ncirSSID, DEVICE_SSID_LENGTH, "mysimplelink-%s", deviceName);
    }
#endif

    sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SSID, strlen(ncirSSID), (const unsigned char*)ncirSSID);

#ifdef DEBUG_SESSION
    UART_PRINT("MAC address: %x:%x:%x:%x:%x:%x\r\n\r\n",
               simpleLinkMac[0],
               simpleLinkMac[1],
               simpleLinkMac[2],
               simpleLinkMac[3],
               simpleLinkMac[4],
               simpleLinkMac[5]);
#endif

    provisioningCmd = SL_WLAN_PROVISIONING_CMD_START_MODE_APSC;

    if(provisioningCmd <= SL_WLAN_PROVISIONING_CMD_START_MODE_APSC_EXTERNAL_CONFIGURATION)
    {
#ifdef DEBUG_SESSION
        UART_PRINT("\r\n Starting Provisioning! mode=%d (0-AP, 1-SC, 2-AP+SC, "
                   "3-AP+SC+WAC)\r\n\r\n", provisioningCmd);
#endif
    }
    else
    {
#ifdef DEBUG_SESSION
        UART_PRINT("\r\n Provisioning Command = %d \r\n\r\n", provisioningCmd);
#endif
    }

    // start provisioning
    retVal = sl_WlanProvisioning(provisioningCmd, ROLE_STA, PROVISIONING_INACTIVITY_TIMEOUT, NULL, 0);

    if(retVal < 0)
    {
#ifdef DEBUG_SESSION
        UART_PRINT(" Provisioning Command Error, num:%d\r\n",retVal);
#endif
    }

    wlanNeedUserProvision = 0;
    wlanConnectToRouter = 1;
    return(retVal);
}

/**
 * This method uses the board's power system to reset the board and restart the program
 * @note This is very useful when issues with WiFi connections arise. This allows users
 *       to re-provision the board in multiple failure instances
 */
static void resetBoard()
{
    if (boardRestarting == 0)
    {
        boardRestarting = 1;
        GPIO_write(Board_PAIRING_OUTPUT_PIN, 1);
        sl_Stop(NWP_STOP_TIMEOUT);

        // Reset the MCU in order allow the WiFi changes to take place
        MAP_PRCMHibernateCycleTrigger();
    }
}

/**
 * While provisioning, restart the program if provisioning has not completed for a certain amount of time
 * @param handle The timer handle (not used, but necessary for the timer callback)
 */
static void WiFiProvisionTimeoutHandler(Timer_Handle handle)
{
    // Want to time out after 10 minutes, so check that the count is < 20
    // (30 seconds * 20 = 600 seconds)
    if (timeoutCount < timeoutMax)
    {
        timeoutCount++;
#ifdef DEBUG_SESSION
        UART_PRINT("Provisioning timeouts left: %d\r\n", timeoutMax - timeoutCount);
#endif
        startMiscOneShotTimer();
    }
    else
    {
#ifdef DEBUG_SESSION
        UART_PRINT("Restarting Board!!\r\n");
#endif
        timeoutCount = 0;

        // Full timeout - reset the board
        resetBoard();
    }
}


/************************************************
 * These functions are required by the SDK.
 * Some are utilized for WiFi provisioning,
 * and some will be left unused by our app.
 ***********************************************/

/**
 *  \brief      This function handles WLAN async events
 *  \param[in]  pWlanEvent - Pointer to the structure containing WLAN event info
 *  \return     None
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    SlWlanEventData_u *pWlanEventData = NULL;

    if(NULL == pWlanEvent)
    {
        return;
    }

    pWlanEventData = &pWlanEvent->Data;

    switch(pWlanEvent->Id)
    {
    case SL_WLAN_EVENT_CONNECT:
    {
#ifdef DEBUG_SESSION
        UART_PRINT("STA connected to AP %s, ",
                   pWlanEvent->Data.Connect.SsidName);

        UART_PRINT("BSSID is %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                   pWlanEvent->Data.Connect.Bssid[0],
                   pWlanEvent->Data.Connect.Bssid[1],
                   pWlanEvent->Data.Connect.Bssid[2],
                   pWlanEvent->Data.Connect.Bssid[3],
                   pWlanEvent->Data.Connect.Bssid[4],
                   pWlanEvent->Data.Connect.Bssid[5]);
#endif
    }
    break;

    case SL_WLAN_EVENT_DISCONNECT:
    {
        SlWlanEventDisconnect_t *pDiscntEvtData = NULL;
        pDiscntEvtData = &pWlanEventData->Disconnect;

        /** If the user has initiated 'Disconnect' request, 'ReasonCode'
         * is SL_USER_INITIATED_DISCONNECTION
         */
        if(SL_WLAN_DISCONNECT_USER_INITIATED == pDiscntEvtData->ReasonCode)
        {
#ifdef DEBUG_SESSION
            UART_PRINT("Device disconnected from the AP on request\r\n");
#endif
        }
        else
        {
#ifdef DEBUG_SESSION
            UART_PRINT("Device disconnected from the AP on an ERROR\r\n");
#endif
        }
    }
    break;

    case SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED:
#ifdef DEBUG_SESSION
        UART_PRINT(" [Provisioning] Profile Added: SSID: %s\r\n", pWlanEvent->Data.ProvisioningProfileAdded.Ssid);
#endif
        if(pWlanEvent->Data.ProvisioningProfileAdded.ReservedLen > 0)
        {
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile Added: PrivateToken:%s\r\n",
                       pWlanEvent->Data.ProvisioningProfileAdded.Reserved);
#endif
        }

        // Stop the provisioning timeout timer
        stopMiscOneShotTimer();

        break;

    case SL_WLAN_EVENT_PROVISIONING_STATUS:
    {
        switch(pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus)
        {
        case SL_WLAN_PROVISIONING_GENERAL_ERROR:
        case SL_WLAN_PROVISIONING_ERROR_ABORT:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_INVALID_PARAM:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_HTTP_SERVER_DISABLED:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_PROFILE_LIST_FULL:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_PROVISIONING_ALREADY_STARTED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Provisioning Error status=%d\r\n",
                       pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
#endif
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_NETWORK_NOT_FOUND:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile confirmation failed: network not found\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_CONNECTION_FAILED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile confirmation failed: Connection failed\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_CONNECTION_SUCCESS_IP_NOT_ACQUIRED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile confirmation failed: IP address not acquired\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS_FEEDBACK_FAILED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile Confirmation failed"
                       "(Connection Success, feedback to Smartphone app failed)\r\n");
#endif
            resetBoard();
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile Confirmation Success!\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_AUTO_STARTED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Auto-Provisioning Started\r\n");
#endif

            // Auto-provisioning has been started outside of the WiFi initialization loop. This means the
            // board has been disconnected for a long period of time. If this happens, reset the board
            // to attempt to reconnect, or allow the user to re-provision this device
            if (wlanConnectedToAP == 1 || timeoutCount != 0)
            {
                resetBoard();
            }
            else
            {
                // Let the WiFi system know that the board needs a user provision
                wlanNeedUserProvision = 1;

                // Stop auto-provisioning in order to start the custom provisioning provided by TI code in this file
                sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP, 0, 0, NULL, 0);

                // Start a provisioning timer in order to cycle the device if the limit is reached
                initMiscOneShotTimer();
                setMiscOneShotTimerCallback(WiFiProvisionTimeoutHandler);
                setMiscOneShotTimeout(30*1000000);
                startMiscOneShotTimer();
            }

            break;

        case SL_WLAN_PROVISIONING_STOPPED:
#ifdef DEBUG_SESSION
            UART_PRINT("\r\n Provisioning stopped:");
#endif
            if(ROLE_STA == pWlanEvent->Data.ProvisioningStatus.Role)
            {
                if(SL_WLAN_STATUS_CONNECTED == pWlanEvent->Data.ProvisioningStatus.WlanStatus)
                {
#ifdef DEBUG_SESSION
                    UART_PRINT("Connected to SSID: %s\r\n", pWlanEvent->Data.ProvisioningStatus.Ssid);
#endif

                    // Stop the provisioning timeout timer
                    stopMiscOneShotTimer();

                    // Successful provisioning: move on to main program
                    wlanConnectToRouter = 0;
                    timeoutCount = 0;
                }
            }
            break;

        case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNCED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Smart Config Synced!\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNC_TIMEOUT:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Smart Config Sync Timeout!\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_WLAN_CONNECT:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile confirmation: WLAN Connected!\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_CONFIRMATION_IP_ACQUIRED:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Profile confirmation: IP Acquired!\r\n");
#endif
            break;

        case SL_WLAN_PROVISIONING_EXTERNAL_CONFIGURATION_READY:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] External configuration is ready! \r\n");
#endif
            break;

        default:
#ifdef DEBUG_SESSION
            UART_PRINT(" [Provisioning] Unknown Provisioning Status: %d\r\n",
                       pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
#endif
            break;
        }
    }
    break;

    default:
    {
#ifdef DEBUG_SESSION
        UART_PRINT("Unexpected WLAN event with Id [0x%x]\r\n", pWlanEvent->Id);
#endif
    }
    break;
    }
}

/**
 *  \brief      This function handles network events such as IP acquisition, IP
 *              leased, IP released etc.
 * \param[in]   pNetAppEvent - Pointer to the structure containing acquired IP
 * \return      None
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(NULL == pNetAppEvent)
    {
        return;
    }

#ifdef DEBUG_SESSION
    SlNetAppEventData_u *pNetAppEventData = NULL;
    pNetAppEventData = &pNetAppEvent->Data;
#endif

    switch(pNetAppEvent->Id)
    {
    case SL_NETAPP_EVENT_IPV6_ACQUIRED:
    case SL_NETAPP_EVENT_IPV4_ACQUIRED:
    {
#ifdef DEBUG_SESSION
        UART_PRINT("IPv4 acquired: IP = %d.%d.%d.%d\r\n", \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,3), \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,2), \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,1), \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Ip,0));
        UART_PRINT("Gateway = %d.%d.%d.%d\r\n", \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,
                                         3), \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,
                                         2), \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,
                                         1), \
                   (uint8_t)SL_IPV4_BYTE(pNetAppEventData->IpAcquiredV4.Gateway,
                                         0));
#endif
        // Stop the provisioning timeout timer
        stopMiscOneShotTimer();

        // When we acquire an IP, let the WiFi connection loop know the board is connected to an AP
        wlanConnectToRouter = 0;
        timeoutCount = 0;
    }
    break;

    case SL_NETAPP_EVENT_IPV4_LOST:
    case SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT:
    {
#ifdef DEBUG_SESSION
        UART_PRINT("IPv4 lost Id or timeout, Id [0x%x]!!!\r\n", pNetAppEvent->Id);
#endif
    }
    break;

    default:
    {
#ifdef DEBUG_SESSION
        UART_PRINT("Unexpected NetApp event with Id [0x%x] \r\n", pNetAppEvent->Id);
#endif
    }
    break;
    }
}

/**
 *  \brief      This function handles ping init-complete event from SL
 *  \param[in]  status - Mode the device is configured in..!
 *  \param[in]  DeviceInitInfo - Device initialization information
 *  \return     None
 */
void SimpleLinkInitCallback(uint32_t status, SlDeviceInitInfo_t *DeviceInitInfo)
{
    switch(status)
    {
    case 0:
#ifdef DEBUG_SESSION
        UART_PRINT("Device started in Station role\r\n");
#endif
        break;

    case 1:
#ifdef DEBUG_SESSION
        UART_PRINT("Device started in P2P role\r\n");
#endif
        break;

    case 2:
#ifdef DEBUG_SESSION
        UART_PRINT("Device started in AP role\r\n");
#endif
        break;
    }

#ifdef DEBUG_SESSION
    UART_PRINT("Device Chip ID:   0x%08X\r\n", DeviceInitInfo->ChipId);
    UART_PRINT("Device More Data: 0x%08X\r\n", DeviceInitInfo->MoreData);
#endif
}

/**
   \brief          This function handles general events
   \param[in]      pDevEvent - Pointer to structure containing general event info
   \return         None
 */
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    // Unused in this application
}

/**
 *  \brief       The Function Handles the Fatal errors
 *  \param[in]  pFatalErrorEvent - Contains the fatal error data
 *  \return     None
 */
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
    // Unused in this application
}

/**
 *  \brief      This function handles ping report events
 *  \param[in]  pPingReport - Pointer to the structure containing ping report
 *  \return     None
 */
void SimpleLinkPingReport(SlNetAppPingReport_t *pPingReport)
{
    // Unused in this application
}

/**
 *  \brief      This function gets triggered when HTTP Server receives
 *              application defined GET and POST HTTP tokens.
 *  \param[in]  pHttpServerEvent Pointer indicating HTTP server event
 *  \param[in]  pHttpServerResponse Pointer indicating HTTP server response
 *  \return     None
 */
void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pHttpEvent, SlNetAppHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

/**
 *  \brief      This function handles resource request
 *  \param[in]  pNetAppRequest - Contains the resource requests
 *  \param[in]  pNetAppResponse - Should be filled by the user with the
 *                                relevant response information
 *  \return     None
 */
void SimpleLinkNetAppRequestHandler(SlNetAppRequest_t  *pNetAppRequest,
                                    SlNetAppResponse_t *pNetAppResponse)
{
    // Unused in this application
}

/**
 *  \brief      This function handles socket events indication
 *  \param[in]  pSock - Pointer to the structure containing socket event info
 *  \return     None
 */
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    // Unused in this application
}

void SimpleLinkSocketTriggerEventHandler(SlSockTriggerEvent_t *pSlTriggerEvent)
{
    // Unused in this application
}

void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{
    // Unused in this application
}

void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
                                         SlNetAppResponse_t *pNetAppResponse)
{
    // Unused in this application
}
