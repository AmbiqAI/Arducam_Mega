#include "common_init.h"
#include "libcamera/ArducamCamera.h"
#include "controller/ArducamLink.h"
#include <stdio.h>
FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
uint8_t ReadBuffer(uint8_t* imagebuf, uint8_t length);
void stop_preivew();
FSP_CPP_FOOTER

ArducamCamera myCAM;
const int cs = BSP_IO_PORT_02_PIN_05;

uint8_t temp             = 0xff;
uint8_t commandBuff[20]  = {0};
uint8_t commandLength    = 0;
uint8_t sendFlag         = true;
uint32_t readImageLength = 0;
uint8_t jpegHeadFlag     = 0;

uint8_t ReadBuffer(uint8_t* imagebuf, uint8_t length)
{
    if (imagebuf[0] == 0xff && imagebuf[1] == 0xd8) {
        jpegHeadFlag    = 1;
        readImageLength = 0;
        arducamUartWrite(0xff);
        arducamUartWrite(0xAA);
        arducamUartWrite(0x01);
        arducamUartWriteBuff((uint8_t *)&myCAM.totalLength, 4);
        arducamUartWrite(myCAM.currentPixelFormat);
    }
    if (jpegHeadFlag == 1) {
        readImageLength += length;
        arducamUartWriteBuff(imagebuf, length);
    }
    if (readImageLength == myCAM.totalLength) {
        jpegHeadFlag = 0;
        arducamUartWrite(0xff);
        arducamUartWrite(0xBB);
    }
    return sendFlag;
}

void stop_preivew()
{
    readImageLength = 0;
    jpegHeadFlag    = 0;
    uint32_t len    = 9;

    arducamUartWrite(0xff);
    arducamUartWrite(0xBB);
    arducamUartWrite(0xff);
    arducamUartWrite(0xAA);
    arducamUartWrite(0x06);
    arducamUartWriteBuff((uint8_t*)&len, 4);
    arducamUartPrintf("streamoff");
    arducamUartWrite(0xff);
    arducamUartWrite(0xBB);
}

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    bool findstart = false;
    uint8_t *cmdbuf = 0;
    static uint8_t cmdline[READ_BUF_SIZE] = {0};
    uint8_t cmdidx = 0;
    arducamUartBegin(921600);
    send_data_pack(7,"Hello renesas!");
    myCAM = createArducamCamera(cs);
    begin(&myCAM);
    send_data_pack(8,"Mega start!");

    registerCallback(&myCAM, ReadBuffer, 200, stop_preivew);

    while(1)
    {
        if(usb_cdc_stask(&cmdbuf) == true)
        {
            cmdidx = 0;
            findstart = false;
            for(uint8_t i = 0; i < READ_BUF_SIZE;i++)
            {
                if(findstart == false ) {
                    if(cmdbuf[i] == 0x55)
                        findstart = true;
                } else {
                    if(cmdbuf[i] == 0xaa)
                    {
                        uartCommandProcessing(&myCAM, cmdline);
                        break;
                    }
                    cmdline[cmdidx++] = cmdbuf[i];
                }
            }

        }
        captureThread(&myCAM);
    }
#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open (&g_ioport_ctrl, g_ioport.p_cfg);
        R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_02_PIN_05, IOPORT_CFG_PORT_DIRECTION_OUTPUT);
        R_IOPORT_PinCfg(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_15, IOPORT_CFG_PORT_DIRECTION_OUTPUT);
    }
}

#if BSP_TZ_SECURE_BUILD

BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
#endif
