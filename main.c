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
    send_blocks(res);
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
	ADC10CTL0 = ADC10SHT_2 + MSC + REFON + REF2_5V + SREF_1 + ADC10ON + ADC10IE; // ADC10ON, interrupt enable, reference voltage 2.5V, multiple conversion
	//Adjust the sampling time! For the moment 16x ADCCLKs
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
void send_byte(unsigned char data)
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

void send_blocks(unsigned int res_[2])
//<-- Send four 1 byte packages
{
	unsigned char buf_0, buf_1, buf_2, buf_3;
	buf_0 = 0b00000000;				//Send the lowest 6 bits of voltage ADC. 2 MSB=0
	buf_1 = 0b01000000;				//Send the highest 6 bits of voltage ADC. 2 MSB=1
	buf_2 = 0b10000000;				//Send the lowest 6 bits of current ADC. 2 MSB=2
	buf_3 = 0b11000000;				//Send the highest 6 bits of current ADC. 2 MSB=3
	int temp_0 = res_[1];			//Voltage buffer
	int temp_1 = res_[0];			//Current buffer
	volatile unsigned int i;
    for (i=0; i<10; i++)			//Convert to binary 6 lowest bits
    {
        if (i<6)
        	{
        	buf_0 += temp_0%2;
        	buf_2 += temp_1%2;
        	}
        else
        {
        	buf_1 += temp_0%2;
        	buf_3 += temp_1%2;
        }
        temp_0 /= 2;
        temp_1 /= 2;
    }
    send_byte(buf_0);
    send_byte(buf_1);
    send_byte(buf_2);
    send_byte(buf_3);
}
