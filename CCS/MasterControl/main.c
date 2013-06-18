/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*  
 * ======== main.c ========
 * Packet Protocol Demo:
 *
 * This USB demo example is to be used with a PC application (e.g. HyperTerminal)
 * to get the demo running, open hyper terminal and type any number 'x' between
 * 1-9, followed by 'x' number of charaters. Once all the 'x' characters are
 * received, the phrase " I received your packet with size of 'x' bytes" is sent
 * to the hyper terminal.
 *
 * If any character other than 1-9 is entered, then the following phrase is
 * returned: "Enter a valid number between 1 and 9"
 *
 * NOTE: In order to see the characters that you are typing in theHyper Terminal
 * window goto File -> Propoerties -> Settings -> ASCII Setup and check the
 * "Echo Typed Characters Locally"
 *
 * ----------------------------------------------------------------------------+
 * Please refer to the MSP430 USB API Stack Programmer's Guide,located
 * in the root directory of this installation for more details.
 * ---------------------------------------------------------------------------*/
#include <intrinsics.h>
#include <string.h>
#include <stdlib.h>

#include "USB_config/descriptors.h"

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"               //Basic Type declarations
#include "USB_API/USB_Common/usb.h"                 //USB-specific functions

#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
#include "F5xx_F6xx_Core_Lib/HAL_PMM.h"

#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "usbConstructs.h"

VOID Init_Ports (VOID);
VOID Init_Clock (VOID);
volatile BYTE bDataReceiveCompleted_event = FALSE;
volatile BYTE bCommandBeingProcessed = FALSE;

WORD x,y;

#define outBufLen 11
char outBuffer[outBufLen];                     //Holds outgoing strings to be sent

#define inBufLen 3
BYTE inBuffer[inBufLen];


VOID randomFillOutbuffer(VOID)
{
	memset(outBuffer, '\0', sizeof(outBuffer));
	static uint16_t clientid = 1;
	uint8_t type = 55;
	static float timestamp = 55.55;
	static float data = 111.111;

	memcpy(outBuffer, &clientid, sizeof(clientid));
	memcpy(&outBuffer[2], &type, sizeof(type));
	memcpy(&outBuffer[3], &timestamp, sizeof(timestamp));
	memcpy(&outBuffer[7], &data, sizeof(data));
	clientid += 1;
	if(clientid >= 10){ clientid = 1;}
	data = rand() * 24;
	timestamp += 0.01;
	/*
	char type = 1;
	outBuffer[0] = (char)sensorID;
	outBuffer[1] = type;
	unsigned int i = 2;
	for (; i < 10; ++i){
		outBuffer[i] = (char) ((rand() / (RAND_MAX + 1.0)) * 24) + 1;
	}
	*/
	//strcpy(outBuffer + outBufLen-1, "\r\n");
}
/*  
 * ======== main ========
 */
VOID main (VOID)
{
    WDTCTL = WDTPW + WDTHOLD;                                   //Stop watchdog timer

    Init_Ports();                                               //Init ports (do first ports because clocks do change ports)
    SetVCore(3);
    Init_Clock();                                               //Init clocks
    srand(200);
	
    USB_init();                 //Init USB
    USB_setEnabledEvents(
        kUSB_VbusOnEvent + kUSB_VbusOffEvent + kUSB_receiveCompletedEvent
        + kUSB_dataReceivedEvent + kUSB_UsbSuspendEvent + kUSB_UsbResumeEvent +
        kUSB_UsbResetEvent);

    //Check if we're already physically attached to USB, and if so, connect to it
    //This is the same function that gets called automatically when VBUS gets attached.
    if (USB_connectionInfo() & kUSB_vbusPresent){
        USB_handleVbusOnEvent();
    }

	__enable_interrupt();                           //Enable interrupts globally
    while (1)
    {
        switch (USB_connectionState())
        {
            case ST_USB_DISCONNECTED:
                __bis_SR_register(LPM3_bits + GIE);                                     //Enter LPM3 w/interrupt
                _NOP();
                break;

            case ST_USB_CONNECTED_NO_ENUM:
                break;

            case ST_ENUM_ACTIVE:
				if (!(USBCDC_intfStatus(CDC0_INTFNUM,&x,							//Wait for packet request
						  &y) & kUSBCDC_waitingForReceive)){                        //Only open it if we haven't already done so
					if (USBCDC_receiveData(inBuffer, inBufLen,
							CDC0_INTFNUM) == kUSBCDC_busNotAvailable){              //Start a receive operation for three Bytes
																					//Exspect 'GET'
						USBCDC_abortReceive(&x,CDC0_INTFNUM);                       //Abort receive
						break;                                                      //If bus is no longer available, escape out
																					//of the loop
					}
				}

                __bis_SR_register(LPM0_bits + GIE);                                     //Wait in LPM0 until a receive operation has
                                                                                        //completed
                                                                                        //__bis_SR_register(LPM0_bits + GIE);
                if (bDataReceiveCompleted_event){
                    bDataReceiveCompleted_event = FALSE;
					if ((inBuffer[0] == 0x47) &&  (inBuffer[1] == 0x45 ) &&			// 'GET'
						 (inBuffer[2] == 0x54)){
						memset( inBuffer, '\0', sizeof(inBuffer) );					//Reset the buffer
						randomFillOutbuffer();
						if (cdcSendDataInBackground((BYTE*)outBuffer,
								outBufLen, CDC0_INTFNUM,0)){                 //Send the response over USB
							USBCDC_abortSend(&x,CDC0_INTFNUM);                      //Operation may still be open; cancel it
							break;                                                  //If the send fails, escape the main loop
						}
						break;
					} else {
						strcpy(
							outBuffer,
							"\r\nINVALID\r\n\r\n");    //Prepare the outgoing string
						if (cdcSendDataInBackground((BYTE*)outBuffer,
								strlen(outBuffer),CDC0_INTFNUM,0)){                 //Send the response over USB
							USBCDC_abortSend(&x,CDC0_INTFNUM);                      //Operation may still be open; cancel it
							break;                                                  //If the send fails, escape the main loop
						}
					}
                }

                break;
            case ST_ENUM_SUSPENDED:
                __bis_SR_register(LPM3_bits + GIE);             //Enter LPM3 until a resume or VBUS-off event
                break;

            case ST_ENUM_IN_PROGRESS:
                break;

            case ST_NOENUM_SUSPENDED:
                __bis_SR_register(LPM3_bits + GIE);
                break;

            case ST_ERROR:
                _NOP();
                break;

            default:;
        }
    }  //while(1)
} //main()

/*  
 * ======== Init_Clock ========
 */
VOID Init_Clock (VOID)
{
    //Initialization of clock module
    if (USB_PLL_XT == 2){
		#if defined (__MSP430F552x) || defined (__MSP430F550x)
			P5SEL |= 0x0C;                                      //enable XT2 pins for F5529
		#elif defined (__MSP430F563x_F663x)
			P7SEL |= 0x0C;
		#endif

        //use REFO for FLL and ACLK
        UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__REFOCLK);
        UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);

        //MCLK will be driven by the FLL (not by XT2), referenced to the REFO
        Init_FLL_Settle(USB_MCLK_FREQ / 1000, USB_MCLK_FREQ / 32768);   //Start the FLL, at the freq indicated by the config
                                                                        //constant USB_MCLK_FREQ
        XT2_Start(XT2DRIVE_0);                                          //Start the "USB crystal"
    } 
	else {
		#if defined (__MSP430F552x) || defined (__MSP430F550x)
			P5SEL |= 0x10;                                      //enable XT1 pins
		#endif
        //Use the REFO oscillator to source the FLL and ACLK
        UCSCTL3 = SELREF__REFOCLK;
        UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);

        //MCLK will be driven by the FLL (not by XT2), referenced to the REFO
        Init_FLL_Settle(USB_MCLK_FREQ / 1000, USB_MCLK_FREQ / 32768);   //set FLL (DCOCLK)

        XT1_Start(XT1DRIVE_0);                                          //Start the "USB crystal"
    }
}

/*  
 * ======== Init_Ports ========
 */
VOID Init_Ports (VOID)
{
    //Initialization of ports (all unused pins as outputs with low-level
    P1OUT = 0x00;
    P1DIR = 0xFF;
	P2OUT = 0x00;
    P2DIR = 0xFF;
    P3OUT = 0x00;
    P3DIR = 0xFF;
    P4OUT = 0x00;
    P4DIR = 0xFF;
    P5OUT = 0x00;
    P5DIR = 0xFF;
    P6OUT = 0x00;
    P6DIR = 0xFF;
	P7OUT = 0x00;
    P7DIR = 0xFF;
    P8OUT = 0x00;
    P8DIR = 0xFF;
    #if defined (__MSP430F563x_F663x)
		P9OUT = 0x00;
		P9DIR = 0xFF;
    #endif
}

/*  
 * ======== UNMI_ISR ========
 */
#pragma vector = UNMI_VECTOR
__interrupt VOID UNMI_ISR (VOID)
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG ))
    {
        case SYSUNIV_NONE:
            __no_operation();
            break;
        case SYSUNIV_NMIIFG:
            __no_operation();
            break;
        case SYSUNIV_OFIFG:
            UCSCTL7 &= ~(DCOFFG + XT1LFOFFG + XT2OFFG); //Clear OSC flaut Flags fault flags
            SFRIFG1 &= ~OFIFG;                  //Clear OFIFG fault flag
            break;
        case SYSUNIV_ACCVIFG:
            __no_operation();
            break;
        case SYSUNIV_BUSIFG:
            SYSBERRIV = 0;                                      //clear bus error flag
            USB_disable();                                      //Disable
    }
}

