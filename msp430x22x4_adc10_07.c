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
//  MSP430F22x4 Demo - ADC10, DTC Sample A0 64x, AVcc, Repeat Single, DCO
//
//  Description: Use DTC to sample A0 64 times with reference to AVcc. Software
//  writes to ADC10SC to trigger sample burst. In Mainloop MSP430 waits in LPM0
//  to save power until ADC10 conversion burst complete, ADC10_ISR(DTC) will
//  force exit from LPM0 in Mainloop on reti. ADC10 internal oscillator
//  times sample period (16x) and conversion (13x). DTC transfers conversion
//  code to RAM 200h - 280h. P1.0 set at start of conversion burst, reset on
//  completion.
//
//                MSP430F22x4
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//        >---|P2.0/A0      P1.0|-->LED
//
//  A. Dannenberg
//  Texas Instruments Inc.
//  April 2006
//  Built with CCE Version: 3.2.0 and IAR Embedded Workbench Version: 3.41A
//******************************************************************************
#include <msp430.h>
#include <string.h>

unsigned int res[2]; //for holding the results
int main(void)
{
  reset();
  setup_ADC();
  setup_UART();
  println("Welcome to the read-out of the ADC");

  for (;;)
  {
    ADC10CTL0 &= ~ENC;
    while (ADC10CTL1 & BUSY);               // Wait if ADC10 core is active
    ADC10SA = (int)res;                     // Data buffer start
    P1OUT |= 0x01;                          // Set P1.0 LED on
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
    P1OUT &= ~0x01;                         // Clear P1.0 LED off
  }
}

// ADC10 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
  UARTWriteRes(res);
	volatile unsigned a = 50000;								  // SW Delay
	do a--;
	while(a != 0);
	__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

void reset(void)
//<-- Disable WatchDog and blink the green led
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	P1DIR |= BIT0 + BIT1; 					  // Set P1.0 and P1.1 to output direction
	volatile unsigned int i;	           	  // volatile to prevent optimization
	P1OUT &= ~BIT0;							  // Switch off P1.0
	P1OUT |= BIT1;							  // Switch on P1.1
	i = 50000;								  // SW Delay
	do i--;
	while(i != 0);
	P1OUT &= ~BIT1;							  // Switch off P1.1
}
void setup_ADC(void)
/*
 * <-- Set the analog inputs for ADC to pins 3(current) and 4(voltage)
 * <-- Configure ADC for ...
*/
{
	ADC10CTL0 &= ~ENC;						  // Disable sampling
	ADC10CTL1 = INCH_1 + CONSEQ_3;            // Repeat multiple channels, channels 0 and 1 selected
	ADC10CTL0 = ADC10SHT_0 + MSC + ADC10ON + ADC10IE; // ADC10ON, interrupt enable
	//Adjust the sampling time! For the moment 4x ADCCLKs
	ADC10DTC1 = BIT1;                         // 2 conversions
	ADC10AE0 |= BIT0;                         // P2.0 ADC option select
	ADC10AE1 |= BIT0;                         // P2.1 ADC option select
}
//void setup_I2C
//{

//}
void setup_UART(void)
/*
 * <-- Setup the UART to look for the ADC outputs
 */
{
	BCSCTL1 = CALBC1_1MHZ; // Basic Clock System.
	DCOCTL = CALDCO_1MHZ;
	P3SEL = BIT4 + BIT5;					  // Choose P1.4 and P1.5 as UART outputs
	UCA0CTL1 |= UCSWRST; // Switch off UART for adjustment
	UCA0CTL1 |= UCSSEL_2; // Set clock to submain (SMCLK), 1MHz
	UCA0BR0 = 104; // 9600 bauds
	UCA0BR1 = 0x00;
	UCA0MCTL = UCBRS0; // Modulation mask
	UCA0CTL1 &= ~UCSWRST; // Switch UART on
}
void UARTWriteRes(unsigned int res_[2])
{
    while (!(IFG2&UCA0TXIFG)); // Check the buffer
    print("Current I = ");
    send_int(res_[0]);
    print(", Voltage V = ");
    send_intln(res_[1]);

}
void send_byte(int data)
{
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = data;
}
void print(char *data)
{
    while(*data)
    {
        send_byte(*data);
        data++;
    }
}
void println(char *data)
{
    while(*data)
    {
        send_byte(*data);
        data++;
    }
    send_byte('\n');
    send_byte('\r');
}
void send_int(int a)
{
    int temp;
    int rev=0;
    int dummy =a;
     while (dummy)
       {
          rev = rev * 10;
          rev = rev + dummy%10;
          dummy = dummy/10;
       }
    while(rev)
    {
        temp=rev%10;
        send_byte(0x30+temp);
        rev /=10;
    }
}
void send_intln(int a)
{
    int temp;
    int rev=0;
    int dummy =a;
     while (dummy)
       {
          rev = rev * 10;
          rev = rev + dummy%10;
          dummy = dummy/10;
       }
    while(rev)
    {
        temp=rev%10;
        send_byte(0x30+temp);
        rev /=10;
    }
     send_byte('\n');
     send_byte('\r');
}
