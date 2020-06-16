/***********************************************************************************************************
 * File Name       : rfEasyLinkTx_nortos.c
 * Created on      : 1-May-2020
 * Author          : AG
 * Last Modfied    :
 * Version History :
 * V0.1              : Initial Implementation
 ***********************************************************************************************************/

/*
 *  ======== rfEasyLinkTx_nortos.c ========
 */

/* Application header files */
#include "smartrf_settings/smartrf_settings.h"

/* Board Header files */
#include "Board.h"


/* Standard C Libraries */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* For sleep() */
#include <unistd.h>

/* TI Drivers */
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/rf/RF.h>
#include <ti/devices/DeviceFamily.h>
#include <ti/drivers/ADCBuf.h>

/* EasyLink API Header files */
#include "easylink/EasyLink.h"
#include "Board.h"

/* ADC buffer declaration */
#define ADCBUFFERSIZE    100u

uint16_t sampleBufferOne[ADCBUFFERSIZE];
uint16_t sampleBufferTwo[ADCBUFFERSIZE];
uint32_t microVoltBuffer[ADCBUFFERSIZE];
uint8_t mVoltBuffer[ADCBUFFERSIZE];


/*RF Payload declaration */
#define RFEASYLINKTX_BURST_SIZE         10
#define RFEASYLINKTXPAYLOAD_LENGTH      54

/*Function declaration */
extern void init_adpcm(void);
void ADPCMEncoder(uint16_t * p_ui16_inputsample,uint8_t * p_ui8_outsample, int16_t prevsample,uint8_t buffer_size);
void RF_Tx_Fn(void);

/* Undefine to not use async mode */
#define RFEASYLINKTX_ASYNC

/* Pin driver handle */
static PIN_Handle pinHandle;
static PIN_State pinState;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config pinTable[] = {
    Board_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_PIN_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/*ADC Initialization */
ADCBuf_Handle adcBuf;
ADCBuf_Params adcBufParams;
ADCBuf_Conversion continuousConversion;

extern int16_t Testsamplebuffer[100];
static uint16_t seqNumber;
unsigned int samplecounter = 0,index = 0;
int16_t  int16_prevSample = 0,counter = 0,RF_Tx_Fn_f=0;
uint16_t int16_prevSampleForRF = 0;

#ifdef RFEASYLINKTX_ASYNC
static volatile bool txDoneFlag;
static volatile uint8_t txSleepPeriodsElapsed;
#endif //RFEASYLINKTX_ASYNC



/**************************************************************************************************/
/* Function Name : adcBufCallback                                                  */
/* Description   : adcBufCallback
* This function is called whenever an DMA buffer is full.
* The content of the buffer is then converted into human-readable format and
* Applying ADPCM encoding after data buffering                                                  */
/* Parameters    : ADCBuf_Handle,  ADCBuf_Conversion   ,completedADCBuffer ,completedChannel                                                               */
/*                                                                                                */
/*                                                                                               */
/* Return        : void                                                                           */
/*                                                                                                */
/**************************************************************************************************/
void adcBufCallback(ADCBuf_Handle handle, ADCBuf_Conversion *conversion,
    void *completedADCBuffer, uint32_t completedChannel)
{
    /* Adjust raw ADC values and convert them to compressed form */
    ADCBuf_adjustRawValues(handle, completedADCBuffer, ADCBUFFERSIZE,
        completedChannel);

    if(completedADCBuffer==&sampleBufferOne)
      {

        ADPCMEncoder(&sampleBufferOne[0],&mVoltBuffer[0],int16_prevSampleForRF, ADCBUFFERSIZE);
        RF_Tx_Fn_f = 1;
      }
      else if(completedADCBuffer==&sampleBufferTwo)
      {
          ADPCMEncoder(&sampleBufferTwo[0],&mVoltBuffer[0],int16_prevSampleForRF, ADCBUFFERSIZE);
          RF_Tx_Fn_f = 1;
         }
}
/* RF Functions*/
/**************************************************************************************************/
/* Function Name : txDoneCb                                                  */
/* Description   : txDoneCb
* This function is called whenever an rf transmission has been completed .
* LED indication logic
*                                                 */
/* Parameters    : EasyLink_Status                                                            */
/*                                                                                                */
/*                                                                                               */
/* Return        : void                                                                           */
/*                                                                                                */
/**************************************************************************************************/

#ifdef RFEASYLINKTX_ASYNC
void txDoneCb(EasyLink_Status status)
{
    if (status == EasyLink_Status_Success)
    {
        /* Toggle LED1 to indicate TX */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1,!PIN_getOutputValue(Board_PIN_LED1));
    }
    else if(status == EasyLink_Status_Aborted)
    {
        /* Toggle LED2 to indicate command aborted */
        PIN_setOutputValue(pinHandle, Board_PIN_LED2,!PIN_getOutputValue(Board_PIN_LED2));
    }
    else
    {
        /* Toggle LED1 and LED2 to indicate error */
        PIN_setOutputValue(pinHandle, Board_PIN_LED1,!PIN_getOutputValue(Board_PIN_LED1));
        PIN_setOutputValue(pinHandle, Board_PIN_LED2,!PIN_getOutputValue(Board_PIN_LED2));
    }
    txDoneFlag = true;
    txSleepPeriodsElapsed = 0;
}
#endif //RFEASYLINKTX_ASYNC

/**************************************************************************************************/
/* Function Name : mainThread                                                  */
/* Description   : main thread
* initialization and while function

*                                                 */
/* Parameters    :                                                             */
/*                                                                                                */
/*                                                                                               */
/* Return        : void                                                                           */
/*                                                                                                */
/**************************************************************************************************/
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    ///*ADC Initialization */
    ADCBuf_init();
    ADCBuf_Params_init(&adcBufParams);
    adcBufParams.callbackFxn = adcBufCallback;
    /* Set up an ADCBuf peripheral in ADCBuf_RECURRENCE_MODE_CONTINUOUS */
    adcBufParams.recurrenceMode =  ADCBuf_RECURRENCE_MODE_CONTINUOUS;
    adcBufParams.returnMode = ADCBuf_RETURN_MODE_CALLBACK;
    adcBufParams.samplingFrequency = 8000;
    adcBuf = ADCBuf_open(Board_ADCBUF0, &adcBufParams);

    /* Configure the conversion struct */
    continuousConversion.arg = NULL;
    continuousConversion.adcChannel = Board_ADCBUF0CHANNEL0;
    continuousConversion.sampleBuffer = sampleBufferOne;
    continuousConversion.sampleBufferTwo = sampleBufferTwo;
    continuousConversion.samplesRequestedCount = ADCBUFFERSIZE;
    if (adcBuf == NULL){
        /* ADCBuf failed to open. */
        while(1);
    }

    /* Start converting. */
    if (ADCBuf_convert(adcBuf, &continuousConversion, 1) !=
        ADCBuf_STATUS_SUCCESS) {
        /* Did not start conversion process correctly. */
        while(1);
    }
    /*
     * Go to sleep in the foreground thread forever. The ADC hardware will
     * perform conversions and invoke the callback function when a buffer is
     * full.
     */
    /* Open LED pins */
    pinHandle = PIN_open(&pinState, pinTable);
    if (pinHandle == NULL)
    {
        while(1);
    }

    /* Clear LED pins */
    PIN_setOutputValue(pinHandle, Board_PIN_LED1, 0);
    PIN_setOutputValue(pinHandle, Board_PIN_LED2, 0);



#ifdef RFEASYLINKTX_ASYNC
    /* Reset the sleep period counter */
    txSleepPeriodsElapsed = 0;
    /* Set the transmission flag to its default state */
    txDoneFlag = false;
#endif //RFEASYLINKTX_ASYNC

    // Initialize the EasyLink parameters to their default values
    EasyLink_Params easyLink_params;
    EasyLink_Params_init(&easyLink_params);

    /*
     * Initialize EasyLink with the settings found in easylink_config.h
     * Modify EASYLINK_PARAM_CONFIG in easylink_config.h to change the default
     * PHY
     */
    if (EasyLink_init(&easyLink_params) != EasyLink_Status_Success){
        while(1);
    }


    /*
     * If you wish to use a frequency other than the default, use
     * the following API:
     * EasyLink_setFrequency(868000000);
     */
    init_adpcm(); /* ADPCM initialization*/
    while(1) {
        if(RF_Tx_Fn_f)
        {
            RF_Tx_Fn();
            RF_Tx_Fn_f = 0;
        }

    }// while
}

/**************************************************************************************************/
/* Function Name : RF_Tx_Fn
Description   : RF frame preparation
initialization and while function
Parameters    :   void */

/**************************************************************************************************/
void RF_Tx_Fn(void)
{
    uint32_t absTime;
    static uint8_t txBurstSize = 0;
    EasyLink_TxPacket txPacket =  { {0}, 0, 0, {0} };

    /* Create packet with incrementing sequence number and random payload */
    txPacket.payload[0] = (uint8_t)(seqNumber >> 8);
    txPacket.payload[1] = (uint8_t)(seqNumber++);
    uint8_t i;
    for (i = 2; i < (RFEASYLINKTXPAYLOAD_LENGTH-2); i++)
    {
        txPacket.payload[i] = mVoltBuffer[i-2];
    }
    txPacket.payload[i] = int16_prevSampleForRF & 0x00ff;
    txPacket.payload[i+1] = (int16_prevSampleForRF>>8) & 0x00ff;

    txPacket.len = RFEASYLINKTXPAYLOAD_LENGTH;

    /*
     * Address filtering is enabled by default on the Rx device with the
     * an address of 0xAA. This device must set the dstAddr accordingly.
     */
    txPacket.dstAddr[0] = 0xaa;

    /* Add a Tx delay for > 500ms, so that the abort kicks in and brakes the burst */
    if(EasyLink_getAbsTime(&absTime) != EasyLink_Status_Success)
    {
        // Problem getting absolute time
    }
    if(txBurstSize++ >= RFEASYLINKTX_BURST_SIZE)
    {
        /* Set Tx absolute time to current time + 1s */

        txPacket.absTime = absTime ;//+ EasyLink_ms_To_RadioTime(3000);//1000 // commented because not needed delayed data transmission
        txBurstSize = 0;
    }
    /* Else set the next packet in burst to Tx in 100ms */
    else
    {
        /* Set Tx absolute time to current time + 100ms */
        txPacket.absTime = absTime ;//+ EasyLink_ms_To_RadioTime(100);
    }
    /*
     * Set the Transmit done flag to false, callback will set it to true
     * Also set the sleep counter to 0
     */
    txDoneFlag = false;
    txSleepPeriodsElapsed = 0;

    /* Transmit the packet */
    EasyLink_transmitAsync(&txPacket, txDoneCb);

}
