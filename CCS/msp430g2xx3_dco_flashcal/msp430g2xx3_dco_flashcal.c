/* --COPYRIGHT--,BSD_EX
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
 *
 *******************************************************************************
 * 
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430G2xx3 Demo - DCO Calibration Constants Programmer
//
//  NOTE: THIS CODE REPLACES THE TI FACTORY-PROGRAMMED DCO CALIBRATION
//  CONSTANTS LOCATED IN INFOA WITH NEW VALUES. USE ONLY IF THE ORIGINAL
//  CONSTANTS ACCIDENTALLY GOT CORRUPTED OR ERASED.
//
//  Description: This code re-programs the G2xx2 DCO calibration constants.
//  A software FLL mechanism is used to set the DCO based on an external
//  32kHz reference clock. After each calibration, the values from the
//  clock system are read out and stored in a temporary variable. The final
//  frequency the DCO is set to is 1MHz, and this frequency is also used
//  during Flash programming of the constants. The program end is indicated
//  by the blinking LED.
//  ACLK = LFXT1/8 = 32768/8, MCLK = SMCLK = target DCO
//  //* External watch crystal installed on XIN XOUT is required for ACLK *//
//
//           MSP430G2xx3
//         ---------------
//     /|\|            XIN|-
//      | |               | 32kHz
//      --|RST        XOUT|-
//        |               |
//        |           P1.0|--> LED
//        |           P1.4|--> SMLCK = target DCO
//
//  D. Dang
//  Texas Instruments Inc.
//  May 2010
//   Built with CCS Version 4.2.0 and IAR Embedded Workbench Version: 3.42A
//******************************************************************************
#include <msp430.h>

#define False 0x00;
#define True 0x01;

#define LOW_to_HIGH 0x00;
#define HIGH_to_LOW 0x01;

volatile unsigned int i;

volatile unsigned int lastTime;
volatile unsigned char isSyncing;
volatile unsigned char isSynced;
volatile unsigned char currentBit;
volatile unsigned char nextBit;

volatile unsigned char inBufferIndex;
volatile unsigned char inBuffer[8];

volatile unsigned char expectShortEdge;

volatile unsigned int INTERRUPTTRESHOLD = 32;


volatile unsigned int T = 128;	//Half Bit Rate = 4useconds
volatile unsigned int T2 = 256;  //Bit Rate = 0.000256 = 256useconds 125kHz / 32

//volatile unsigned char Ttolerance = 1;
volatile unsigned int T2Max = 320; //T2 + Ttolerance;
volatile unsigned int T2Min = 192; //T2 - Ttolerance;

volatile unsigned int TMax = 192; //T + Ttolerance;
volatile unsigned int TMin = 64; //T - Ttolerance;


inline void startSync(void);
void setupPins(void);

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  for (i = 0; i < 0xfffe; i++);             // Delay for XTAL stabilization

  inBufferIndex = 0;
  isSyncing = False;
  isSynced = False;
  expectShortEdge = False;

  BCSCTL1 = CALBC1_16MHZ;					// Set DCO to 16MHz
  DCOCTL = CALDCO_16MHZ;					// Set DCP to 16MHz

  BCSCTL2 = SELM_0 + DIVM_0 + DIVS_3;		// Select DCO as MCLK; MCLK / 1 + SMCLK / 8

  setupPins();

  __bis_SR_register(GIE);       			// enable interrupts
}

volatile unsigned int diff;
//edge detect interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
	TA0CCTL0 ^= CCIS0;							//Trigger time capture
	diff = TA0CCR0 - lastTime;

	//Start Timer
	if(!isSyncing)
	{
		isSyncing = True;
		//TEMPORALILY Enable LED OUTPUT
		P1OUT = 0x00;                             // Clear P1 output latches
		P1SEL = 0x10;                             // P1.4 SMCLK output
		P1DIR = 0x11;                             // P1.0,4 output
		//END TEMPORARY

		lastTime = 0;

		TACTL |= TASSEL_2 | ID_1 | MC_2;			// Enable capture timer /2 = 1uSecond
		TA0CCTL0 = CAP + SCS + CCIS1 + CM_3;
		TA0CCTL0 ^= CCIS0;
		lastTime = TA0CCR0;
		return;
	}

	// Early escape
	if( diff < INTERRUPTTRESHOLD){
		return;
	}

	P2IES ^= BIT0;								//Flip edge detect direction
	P2IFG &= ~BIT0;								//Clear edge interrupt
	P1OUT ^= BIT0;
	lastTime = TACCR0;

	if ( diff < TMin || diff > T2Max)
	{
		return;
	}

	if(!isSynced)
	{
		if (T2Min < diff && diff < T2Max)
		{
			isSynced = True;
			currentBit = P2IN;
		}
		lastTime = TA0CCR0;
		return;
	}

	if(isSynced){
		if (expectShortEdge)
		{
			expectShortEdge = False;
			if (T2Min < diff && diff < T2Max)
			{
				//TODO: Handle Error
				return;
			}else{
				//TODO: Actually check time, error bucket
				nextBit = currentBit;
			}
		}
		else
		{
			if (TMin < diff && diff < TMax)
			{
				expectShortEdge = True;
				return;
			}
			else if (T2Min < diff && diff < T2Max)
			{
				nextBit ^= 0x01;
			}
			else
			{
				isSynced = False;
				isSyncing = False;
				return;
			}
		}
	} //End if(isSynced)

	inBuffer[inBufferIndex++] = nextBit;
	inBufferIndex = inBufferIndex % 8;
	currentBit = nextBit;
}

void setupPins(void)
{
	P2IES = HIGH_to_LOW;
	P2IE = BIT0;
	P2REN = BIT0;
	P2DIR = 0x0;
}
