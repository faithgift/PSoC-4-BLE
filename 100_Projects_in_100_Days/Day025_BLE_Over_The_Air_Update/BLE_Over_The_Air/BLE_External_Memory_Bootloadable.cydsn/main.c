/*******************************************************************************
* File Name: main.c
*
* Version: 1.0
*
* Description:
*  Simple BLE example project that demonstrates how to configure and use
*  Cypress's BLE component APIs and application layer callback. Device
*  Information service is used as an example to demonstrate configuring
*  BLE service characteristics in the BLE component.
*
* References:
*  BLUETOOTH SPECIFICATION Version 4.1
*
* Hardware Dependency:
*  CY8CKIT-042 BLE
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation. All rights reserved.
* This software is owned by Cypress Semiconductor Corporation and is protected
* by and subject to worldwide patent and copyright laws and treaties.
* Therefore, you may use this software only as provided in the license agreement
* accompanying the software package from which you obtained this software.
* CYPRESS AND ITS SUPPLIERS MAKE NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* WITH REGARD TO THIS SOFTWARE, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT,
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*******************************************************************************/

#include "main.h"


/* Buffer for received data */
uint8 packetRX[BLE_PACKET_SIZE_MAX];
uint32 packetRXSize;
uint32 packetRXFlag;

uint8 packetTX[BLE_PACKET_SIZE_MAX];
uint32 packetTXSize;

uint8 bootloadingMode = 0;

CYBLE_CONN_HANDLE_T connHandle;

/*******************************************************************************
* Function Name: AppCallBack()
********************************************************************************
*
* Summary:
*   This finction handles events that are generated by BLE stack.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void AppCallBack(uint32 event, void* eventParam)
{
    CYBLE_API_RESULT_T apiResult;
    uint32  i = 0u;
   
    switch (event)
    {
        /**********************************************************
        *                       General Events
        ***********************************************************/
        case CYBLE_EVT_STACK_ON: /* This event received when component is Started */
            /* Enter in to discoverable mode so that remote can search it. */
            apiResult = CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            if(apiResult != CYBLE_ERROR_OK)
            {
            }
            break;
        case CYBLE_EVT_HARDWARE_ERROR:    /* This event indicates that some internal HW error has occurred. */
            DBG_PRINTF("CYBLE_EVT_HARDWARE_ERROR\r\n");
            break;
            

        /**********************************************************
        *                       GAP Events
        ***********************************************************/
        case CYBLE_EVT_GAP_AUTH_REQ:
            DBG_PRINTF("EVT_AUTH_REQ: security=%x, bonding=%x, ekeySize=%x, err=%x \r\n",
                (*(CYBLE_GAP_AUTH_INFO_T *)eventParam).security,
                (*(CYBLE_GAP_AUTH_INFO_T *)eventParam).bonding,
                (*(CYBLE_GAP_AUTH_INFO_T *)eventParam).ekeySize,
                (*(CYBLE_GAP_AUTH_INFO_T *)eventParam).authErr);
            break;
        case CYBLE_EVT_GAP_PASSKEY_ENTRY_REQUEST:
            DBG_PRINTF("EVT_PASSKEY_ENTRY_REQUEST press 'p' to enter passkey \r\n");
            break;
        case CYBLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            DBG_PRINTF("EVT_PASSKEY_DISPLAY_REQUEST %6.6ld \r\n", *(uint32 *)eventParam);
            break;
        case CYBLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT:
            DBG_PRINTF("EVT_GAP_KEYINFO_EXCHNGE_CMPLT \r\n");
            break;
        case CYBLE_EVT_GAP_AUTH_COMPLETE:
            DBG_PRINTF("AUTH_COMPLETE");
            break;
        case CYBLE_EVT_GAP_AUTH_FAILED:
            DBG_PRINTF("EVT_AUTH_FAILED: %x \r\n", *(uint8 *)eventParam);
            break;
        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            DBG_PRINTF("EVT_GAP_DEVICE_CONNECTED: %d \r\n", connHandle.bdHandle);
            LED_WRITE_MACRO(LED_OFF);
            break;
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            DBG_PRINTF("EVT_GAP_DEVICE_DISCONNECTED\r\n");
            apiResult = CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            if(apiResult != CYBLE_ERROR_OK)
            {
                DBG_PRINTF("StartAdvertisement API Error: %d \r\n", apiResult);
            }
            break;
        case CYBLE_EVT_GAP_ENCRYPT_CHANGE:
            DBG_PRINTF("EVT_GAP_ENCRYPT_CHANGE: %x \r\n", *(uint8 *)eventParam);
            break;
        case CYBLE_EVT_GAPC_CONNECTION_UPDATE_COMPLETE:
            DBG_PRINTF("EVT_CONNECTION_UPDATE_COMPLETE: %x \r\n", *(uint8 *)eventParam);
            break;
        case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            if(CYBLE_STATE_DISCONNECTED == CyBle_GetState())
            {   
                /* Fast and slow advertising period complete, go to low power  
                 * mode (Hibernate mode) and wait for an external
                 * user event to wake up the device again */
                DBG_PRINTF("Entering low power mode...\r\n");
                Bootloading_LED_Write(LED_ON);
                Advertising_LED_1_Write(LED_ON);
                Advertising_LED_2_Write(LED_ON);
                Bootloader_Service_Activation_ClearInterrupt();
                Wakeup_Interrupt_ClearPending();
                Wakeup_Interrupt_Start();
                CySysPmHibernate();
            }
            break;

            
        /**********************************************************
        *                       GATT Events
        ***********************************************************/
        case CYBLE_EVT_GATTS_WRITE_REQ:
            DBG_PRINTF("EVT_GATT_WRITE_REQ: %x = ",((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->handleValPair.attrHandle);
            for(i = 0; i < ((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->handleValPair.value.len; i++)
            {
                DBG_PRINTF("%2.2x ", ((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->handleValPair.value.val[i]);
            }
            DBG_PRINTF("\r\n");
            CyBle_GattsWriteAttributeValue(&((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->handleValPair, 0u, \
                        &((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->connHandle, CYBLE_GATT_DB_PEER_INITIATED);

            (void)CyBle_GattsWriteRsp(((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->connHandle);

            break;
        case CYBLE_EVT_GATT_CONNECT_IND:
            connHandle = *(CYBLE_CONN_HANDLE_T *)eventParam;
            break;
        case CYBLE_EVT_GATT_DISCONNECT_IND:
            connHandle.bdHandle = 0;
            break;
        case CYBLE_EVT_GATTS_WRITE_CMD_REQ:
            /* Pass packet to bootloader emulator */
            packetRXSize = ((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->handleValPair.value.len;
            memcpy(&packetRX[0], ((CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam)->handleValPair.value.val, packetRXSize);
            packetRXFlag = 1u;

            break;
        case CYBLE_EVT_GATTS_PREP_WRITE_REQ:
            (void)CyBle_GattsPrepWriteReqSupport(CYBLE_GATTS_PREP_WRITE_NOT_SUPPORT);
            break;
        case CYBLE_EVT_HCI_STATUS:
            DBG_PRINTF("CYBLE_EVT_HCI_STATUS\r\n");
        default:
            break;
        }
}

/*******************************************************************************
* Function Name: HandleLeds()
********************************************************************************
*
* Summary:
*   This function handles LEDs operation depending on the project operation
*   mode.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void HandleLeds(void)
{
    static uint32 ledTimer = LED_TIMEOUT;
    static uint8 advLed = LED_OFF;

    /* Blink blue LED to indicate that device advertises */
    if(connHandle.bdHandle == 0u)
    {
        if(--ledTimer == 0u)
        {
            ledTimer = LED_TIMEOUT;
            advLed ^= LED_OFF;
            if (bootloadingMode == 0u)
            {
                LED_WRITE_MACRO(advLed);
            }
            else
            {
                Bootloading_LED_Write(advLed);
            }
            CyDelay(1u);
        }
    }
    else
    {
        if (bootloadingMode == 0u)
        {
            LED_WRITE_MACRO(LED_ON);
        }
        else
        {
            Bootloading_LED_Write(LED_ON);
        }
    }
}

/*******************************************************************************
* Function Name: Timer_Interrupt
********************************************************************************
*
* Summary:
*  Handles the Interrupt Service Routine for the WDT timer.
*
*******************************************************************************/
CY_ISR(Timer_Interrupt)
{
    if(CySysWdtGetInterruptSource() & WDT_INTERRUPT_SOURCE)
    {
        HandleLeds();
        
        /* Clears interrupt request  */
        CySysWdtClearInterrupt(WDT_INTERRUPT_SOURCE);
    }
}

/*******************************************************************************
* Function Name: WDT_Start
********************************************************************************
*
* Summary:
*  Configures WDT to trigger an interrupt.
*
*******************************************************************************/

void WDT_Start(void)
{
    /* Unlock the WDT registers for modification */
    CySysWdtUnlock(); 
    /* Setup ISR */
    WDT_Interrupt_StartEx(&Timer_Interrupt);
    /* Write the mode to generate interrupt on match */
    CySysWdtWriteMode(WDT_COUNTER, CY_SYS_WDT_MODE_INT);
    /* Configure the WDT counter clear on a match setting */
    CySysWdtWriteClearOnMatch(WDT_COUNTER, WDT_COUNTER_ENABLE);
    /* Configure the WDT counter match comparison value */
    CySysWdtWriteMatch(WDT_COUNTER, WDT_TIMEOUT);
    /* Reset WDT counter */
    CySysWdtResetCounters(WDT_COUNTER);
    /* Enable the specified WDT counter */
    CySysWdtEnable(WDT_COUNTER_MASK);
    /* Lock out configuration changes to the Watchdog timer registers */
    CySysWdtLock();    
}


/*******************************************************************************
* Function Name: WDT_Stop
********************************************************************************
*
* Summary:
*  This API stops the WDT timer.
*
*******************************************************************************/
void WDT_Stop(void)
{
    /* Unlock the WDT registers for modification */
    CySysWdtUnlock(); 
    /* Disable the specified WDT counter */
    CySysWdtDisable(WDT_COUNTER_MASK);
    /* Locks out configuration changes to the Watchdog timer registers */
    CySysWdtLock();    
}


/*******************************************************************************
* Function Name: WriteAttrServChanged()
********************************************************************************
*
* Summary:
*   Sets serviceChangedHandle for enabling or disabling hidden service.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void WriteAttrServChanged(void)
{
    uint32 value;
    CYBLE_GATT_HANDLE_VALUE_PAIR_T    handleValuePair;
    
    /* Force client to rediscover services in range of bootloader service */
    value = (cyBle_customs[0u].customServiceHandle)<<16u|\
                                (cyBle_customs[0u].customServiceInfo[0u].customServiceCharDescriptors[0u]);
    handleValuePair.value.val = (uint8 *)&value;
    handleValuePair.value.len = sizeof(value);

    handleValuePair.attrHandle = cyBle_gatts.serviceChangedHandle;
    CyBle_GattsWriteAttributeValue(&handleValuePair, 0u, NULL,CYBLE_GATT_DB_LOCALLY_INITIATED);
}

/******************************************************************************/
int main()
{
    const char8 serialNumber[] = SERIAL_NUMBER;
    CYBLE_LP_MODE_T lpMode;
    CYBLE_BLESS_STATE_T blessState;

    packetRXFlag = 0u;

    DBG_PRINT_TEXT("\r\n");
    DBG_PRINT_TEXT("\r\n");
    DBG_PRINT_TEXT("===============================================================================\r\n");
    DBG_PRINT_TEXT("=              BLE_External_Memory_Bootloadable Application Started            \r\n");
    DBG_PRINT_TEXT("=              Version: 1.0                                                    \r\n");
    #if (LED_ADV_COLOR == LED_GREEN)
        DBG_PRINT_TEXT("=              Code: LED_GREEN                                                 \r\n");
    #else
        DBG_PRINT_TEXT("=              Code: LED_BLUE                                                 \r\n");
    #endif /*LED_ADV_COLOR == LED_GREEN*/
    DBG_PRINTF    ("=              Compile Date and Time : %s %s                                   \r\n", __DATE__,__TIME__);
    #if (ENCRYPTION_ENABLED == YES)
        DBG_PRINT_TEXT("=              ENCRYPTION OPTION : ENABLED                                                \r\n");
    #else
        DBG_PRINT_TEXT("=              ENCRYPTION OPTION : DISABLED                                               \r\n");
    #endif /*LED_ADV_COLOR == LED_GREEN*/
    #if (CI_PACKET_CHECKSUM_CRC == YES)
        DBG_PRINT_TEXT("=              PACKET CHECKSUM TYPE: CRC-16-CCITT                                         \r\n");
    #else
        DBG_PRINT_TEXT("=              PACKET CHECKSUM TYPE: BASIC SUMMATION                                      \r\n");
    #endif /*LED_ADV_COLOR == LED_GREEN*/
    
    DBG_PRINT_TEXT("===============================================================================\r\n");
    DBG_PRINT_TEXT("\r\n"); 
    DBG_PRINT_TEXT("\r\n");   

    CyGlobalIntEnable;

    Bootloading_LED_Write(LED_OFF);
    Advertising_LED_1_Write(LED_OFF);
    Advertising_LED_2_Write(LED_OFF);

    
    CyBle_Start(AppCallBack);
    
    /*Initialization of encryption in BLE stack*/
    #if (ENCRYPTION_ENABLED == YES)
        CR_Initialization();
    #endif /*(ENCRYPTION_ENABLED == YES)*/
    
    
    /* Set Serial Number string not initialised in GUI */
    CyBle_DissSetCharacteristicValue(CYBLE_DIS_SERIAL_NUMBER, sizeof(serialNumber), (uint8 *)serialNumber);

    /* Disable bootloader service */
    CyBle_GattsDisableAttribute(cyBle_customs[0].customServiceHandle);

    /* Force client to rediscover services in range of bootloader service */
    WriteAttrServChanged();
    
    WDT_Start();

    while(1u == 1u)
    {
        if(CyBle_GetState() != CYBLE_STATE_INITIALIZING)
        {
            /* Enter DeepSleep mode between connection intervals */
            lpMode = CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);
            CyGlobalIntDisable;
            blessState = CyBle_GetBleSsState();

            if(lpMode == CYBLE_BLESS_DEEPSLEEP) 
            {   
                if(blessState == CYBLE_BLESS_STATE_ECO_ON || blessState == CYBLE_BLESS_STATE_DEEPSLEEP)
                {
                    CySysPmDeepSleep();
                }
            }
            else
            {
                if(blessState != CYBLE_BLESS_STATE_EVENT_CLOSE)
                {
                    CySysPmSleep();
                }
            }
            CyGlobalIntEnable;
        }

        CyBle_ProcessEvents();

        /* If key press event was detected - debounce it and switch to bootloader emulator mode */
        if (Bootloader_Service_Activation_Read() == 0u)
        {
            CyDelay(100u);
            
            if (Bootloader_Service_Activation_Read() == 0u)
            {
                DBG_PRINTF("Bootloader service activated!\r\n");
                CyBle_GattsEnableAttribute(cyBle_customs[0u].customServiceHandle);
                LED_WRITE_MACRO(LED_OFF);
                bootloadingMode = 1u;

                /* Force client to rediscover services in range of bootloader service */
                WriteAttrServChanged();

                BootloaderEmulator_Start();
            }
        }
    }
}

/* [] END OF FILE */

