/*==================================================================
  Function name: display_note()         , display_cents(),
                 Timer1IntHandler()     , ADC0_SS3_IntHandler(),
                 store_in_yin_buffer()  , find_nearest_note(),
                 init_gpio_peripherals(), init_timer0_for_adc(),
                 init_timer1()          , main()
  File name    : main.c
  CVS ID       : $Id: $
  Author       : E. vd Logt
  Date         : 2013-10-20
  ------------------------------------------------------------------
  Purpose : This file contains the guitar-tuner functions.
            The purpose of this is to read a number of ADC0 samples
            and input this to the Yin algorithm (pitch detection)
            to identify the note played, including the deviation from
            this note. This is displayed on the 7-segment displays
            and the CENTS LEDs (that display the actual deviation).
  ------------------------------------------------------------------*/
#include "main.h"

const float note_freq[MAX_NOTES] = {  20.602,  21.827,  23.125,  24.500,  25.957,  27.500,  29.135,  30.868,
  32.703,  34.648,  36.708,  38.891,  41.203,  43.654,  46.249,  48.999,  51.913,  55.000,  58.270,  61.735,
  65.406,  69.296,  73.416,  77.782,  82.407,  87.307,  92.499,  97.999, 103.826, 110.000, 116.541, 123.471,
 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220.000, 233.082, 246.942,
 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883,
 523.251, 554.365, 587.330, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880.000, 932.328, 987.767,
1046.502,1108.731,1174.659,1244.508,1318.510,1396.913 };

const char note_name[MAX_NOTES][4] = {"E0","F0","F#0","G0","G#0","A0","A#0","B0",
                "C1","C#1","D1","D#1","E1","F1","F#1","G1","G#1","A1","A#1","B1",
                "C2","C#2","D2","D#2","E2","F2","F#2","G2","G#2","A2","A#2","B2",
                "C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3",
                "C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4",
                "C5","C#5","D5","D#5","E5","F5","F#5","G5","G#5","A5","A#5","B5",
                "C6","C#6","D6","D#6","E6","F6"};

// Constants for the letters        A,   B,   C,   D,   E,   F,   G    -  and empty characters
const int seg7_notes_PE[NOTES]  = {0x03,0x03,0x01,0x02,0x03,0x03,0x01,0x02,0x00}; // PE0 .. PE2 : f,g,dp
const int seg7_notes_PF[NOTES]  = {0x17,0x1c,0x19,0x1e,0x19,0x11,0x1d,0x00,0x00}; // PF0 .. PF4 : a,b,c,d,e
// Constants for the scale numbers  0,   1,   2,   3,   4,   5,   6 and empty characters
const int seg7_scale_PE[SCALES] = {0x01,0x00,0x02,0x02,0x03,0x03,0x03,0x00}; // PE0 .. PE2 : f,g,dp
const int seg7_scale_PF[SCALES] = {0x1f,0x06,0x1b,0x0f,0x06,0x0d,0x1d,0x00}; // PF0 .. PF4 : a,b,c,d,e

// Constants for the CENTS LEDs, one LED at a time
//                               0+5%,  5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50% Empty
const int cents_neg_PA[CENTS] = {0x04,0x04,0x08,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const int cents_neg_PB[CENTS] = {0x80,0x00,0x00,0x00,0x04,0x08,0x40,0x00,0x00,0x00,0x00,0x00};
const int cents_neg_PC[CENTS] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x40,0x80,0x00};
const int cents_pos_PA[CENTS] = {0x00,0x00,0x00,0x20,0x40,0x00,0x00,0x80,0x00,0x00,0x00,0x00};
const int cents_pos_PB[CENTS] = {0x81,0x01,0x02,0x00,0x00,0x10,0x20,0x00,0x00,0x00,0x00,0x00};
const int cents_pos_PD[CENTS] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x04,0x00};

// Constants for the LED-show, when the deviation is less than 1 %
//                                   50%, 45%, 40%, 35%, 30%, 25%, 20%, 15%, 10%, 5%,  0%
const int in_tune_PA[MAX_IDX_IT] = {0x00,0x00,0x00,0x80,0x00,0x00,0x40,0x30,0x08,0x04,0x00};
const int in_tune_PB[MAX_IDX_IT] = {0x00,0x00,0x00,0x00,0x60,0x18,0x04,0x00,0x02,0x01,0x80};
const int in_tune_PC[MAX_IDX_IT] = {0x80,0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const int in_tune_PD[MAX_IDX_IT] = {0x04,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// Global variables
tRingBufObject     O_ringbuf;                 // Ringbuffer object
UINT               adc_ringbuf[ADC_BUF_SIZE]; // Ringbuf for 1st storage of ADC samples
UINT  			   buf_int[ADC_BUF_SIZE];	  // Temporary buffer
float              yin_buf[ADC_BUF_SIZE];     // Buffer for Yin auto-correlation function
Yin                yin;                       // Struct for Yin pitch detection algorithm
short int          sampled_new = false;       // true: new samples are read
int                nearest_note;              // The note-index of the closest note to the frequency found
float 			   cents;                     // The deviation in cents
int                flat_tuning_mode = 0;      // [0..6] Flat Tuning Mode

/*-------------------------------------------------------------------------
 Purpose   : This function is called from within the TIMER1 interrupt and
             performs the following functions:
             - display the note found on the 1st (big) 7-segment display
             - (re)set the LED for the 'sharp-note' indication (PE4)
             - display the scale-number on the 2nd (small) 7-segment display

            The 7-segment display is connected in the following way:
              a         PF0
             f b     PE0   PF1
              g         PE1
             e c     PF4   PF2
              d         PF3
               dp          PE2
  Variables: -
  Returns  : -
  -------------------------------------------------------------------------*/
void display_note(void)
{
    static int tmr     = 0;           // timer for dp blinking
	static int set_led = 0;           // set/clear dp indicator
	static int mpx     = 0;           // 0 = display note name ; 1 = display scale-number
	int        idxn;                  // index in seg7_notes_PE and seg7_notes_PF array
	int        idxs;                  // index in seg7_scale_PE and seg7_scale_PF array
	int        sharp;                 // 1 = note is sharp, 0 = normal note

	if (++tmr > (TIMER1_FREQ/2))
	{	// blink every second
		set_led = !set_led;
		tmr     = 0;
	} // if

	if (flat_tuning_active())
	{   // Handle Flat-Tuning on Display when Flat-Tuning switch is pressed
		idxn  = MINUS;
		idxs  = flat_tuning_mode;
		sharp = 0;
	}
	else if ((nearest_note < 0) || (nearest_note  + flat_tuning_mode > MAX_NOTES-1))
	{
		idxn  = NOTES - 1;
		idxs  = SCALES - 1;
		sharp = 0;
	} // if
	else
	{
		idxn = (int)(note_name[nearest_note + flat_tuning_mode][0] - 'A');
		if ((idxn < 0) || (idxn > 6)) // A is note 0, G is note 6
		{
			idxn = NOTES-1; // empty character
		} // if
		sharp = (note_name[nearest_note + flat_tuning_mode][1] == '#') ? 1 : 0;
		idxs  = (int)(note_name[nearest_note + flat_tuning_mode][1+sharp] - '0');
		if ((idxs < 0) || (idxs > SCALES-1))
		{
			idxs = SCALES-1;
		} // if
	} // if

	if (mpx)
	{   //display scale-number on 2nd (small, right) 7-segment display
		GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_7, 0x0040);       // Set 7-segment multiplexer
		GPIOPinWrite(GPIO_PORTE_BASE, SEG7_PORTE, seg7_scale_PE[idxs]);     // Set 7-segment display
		GPIOPinWrite(GPIO_PORTF_BASE, SEG7_PORTF, seg7_scale_PF[idxs]);     // Set 7-segment display
		if (set_led) GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2); // blink dp only on right display
		else         GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0x00);	    // clear dp
	} // if
	else
	{   //display note-name on 1st (big, left) 7-segment display and set dp and LED for sharp-indication
		GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_7, 0x0080);     // Set 7-segment multiplexer
		GPIOPinWrite(GPIO_PORTE_BASE, SEG7_PORTE, seg7_notes_PE[idxn]);   // Set 7-segment display
		GPIOPinWrite(GPIO_PORTF_BASE, SEG7_PORTF, seg7_notes_PF[idxn]);   // Set 7-segment display
		if (sharp) GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2); // Set dp on left display
		else       GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0x00);	      // clear dp
	} // else

	if (sharp)
	{
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4); // Set LED for Sharp Note Indication
	} // if
	else
	{
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);       // Clear LED for Sharp Note Indication
	} // else
	mpx = !mpx; // switch to other 7-segment display
} // display_note()

/*-------------------------------------------------------------------------
 Purpose   : This function displays the LEDs for the cents display.
             The following LEDs are available:
             +05, +10, +15, +20, +25, +30, +35, +40, +45, +50
             -05, -10, -15, -20, -25, -30, -35, -40, -45, -50
             Center LED
  Variables: nearest_note (global)
             cents (global)
  Returns  : -
  -------------------------------------------------------------------------*/
void display_cents(void)
{
	int        idxc;     // index in cents arrays
	int        in_tune;  // 1 = deviation is < 1 % (note is in-tune)
	static int idx_it;   // index in in_tune arrays
	static int std_leds = STD_LEDS_CLEAR_DISPLAY; // STD state number
	static int std_leds_delay; // delay for LED-show

	if (nearest_note < 0)   idxc = CENTS-1; // Clear CENTS LEDs
	else if (cents > +47.5) idxc = 10;
	else if (cents > +42.5) idxc =  9;
	else if (cents > +37.5) idxc =  8;
	else if (cents > +32.5) idxc =  7;
	else if (cents > +27.5) idxc =  6;
	else if (cents > +22.5) idxc =  5;
	else if (cents > +17.5) idxc =  4;
	else if (cents > +12.5) idxc =  3;
	else if (cents >  +7.5) idxc =  2;
	else if (cents >  +2.5) idxc =  1;
	else if (cents >  -2.5) idxc =  0;
	else if (cents >  -7.5) idxc = -1;
	else if (cents > -12.5) idxc = -2;
	else if (cents > -17.5) idxc = -3;
	else if (cents > -22.5) idxc = -4;
	else if (cents > -27.5) idxc = -5;
	else if (cents > -32.5) idxc = -6;
	else if (cents > -37.5) idxc = -7;
	else if (cents > -42.5) idxc = -8;
	else if (cents > -47.5) idxc = -9;
	else                    idxc = -10;

	if ((cents < 1.0) && (cents > -1.0))
		 in_tune = 1; // in-tune
	else in_tune = 0; // not in-tune

	switch (std_leds)
	{
	case STD_LEDS_CLEAR_DISPLAY: // Clear display if no note found
		GPIOPinWrite(GPIO_PORTA_BASE, LEDS_PORTA, cents_pos_PA[CENTS-1]);
		GPIOPinWrite(GPIO_PORTB_BASE, LEDS_PORTB, cents_pos_PB[CENTS-1]);
		GPIOPinWrite(GPIO_PORTC_BASE, LEDS_PORTC, cents_neg_PC[CENTS-1]);
		GPIOPinWrite(GPIO_PORTD_BASE, LEDS_PORTD, cents_pos_PD[CENTS-1]);
		if ((idxc > 0) || ((idxc == 0) && !in_tune))
		{
			std_leds = STD_LEDS_POSITIVE;
		} // if
		else if (idxc < 0)
		{
			std_leds = STD_LEDS_NEGATIVE;
		} // else if
		else
		{	// idxc == 0 && in_tune
			idx_it   = 0;
			std_leds = STD_LEDS_SHOW_IN_TUNE;
		} // else
		break;

	case STD_LEDS_POSITIVE:  // Set Positive LED cents
		GPIOPinWrite(GPIO_PORTA_BASE, LEDS_PORTA, cents_pos_PA[idxc]);
		GPIOPinWrite(GPIO_PORTB_BASE, LEDS_PORTB, cents_pos_PB[idxc]);
		GPIOPinWrite(GPIO_PORTC_BASE, LEDS_PORTC, 0x00);
		GPIOPinWrite(GPIO_PORTD_BASE, LEDS_PORTD, cents_pos_PD[idxc]);
		if (idxc == CENTS-1)
		{
			std_leds = STD_LEDS_CLEAR_DISPLAY;
		}
		else if (idxc < 0)
		{
			std_leds = STD_LEDS_NEGATIVE;
		} // else if
		else if ((idxc == 0) && in_tune)
		{
			idx_it   = 0;
			std_leds = STD_LEDS_SHOW_IN_TUNE;
		} // else
		// else ((idxc > 0) || ((idxc == 0) && !in_tune)) remain in this state
		break;

	case STD_LEDS_NEGATIVE:  // Set Negative LED cents
		GPIOPinWrite(GPIO_PORTA_BASE, LEDS_PORTA, cents_neg_PA[-idxc]);
		GPIOPinWrite(GPIO_PORTB_BASE, LEDS_PORTB, cents_neg_PB[-idxc]);
		GPIOPinWrite(GPIO_PORTC_BASE, LEDS_PORTC, cents_neg_PC[-idxc]);
		GPIOPinWrite(GPIO_PORTD_BASE, LEDS_PORTD, 0x00);
		if (idxc == CENTS-1)
		{
			std_leds = STD_LEDS_CLEAR_DISPLAY;
		}
		if ((idxc > 0) || ((idxc == 0) && !in_tune))
		{
			std_leds = STD_LEDS_POSITIVE;
		} // else if
		else if ((idxc == 0) && in_tune)
		{
			idx_it   = 0;
			std_leds = STD_LEDS_SHOW_IN_TUNE;
		} // else
		// else (idx_it < 0) remain in this state
		break;

	case STD_LEDS_SHOW_IN_TUNE: // idxc == 0 && in_tune
		GPIOPinWrite(GPIO_PORTA_BASE, LEDS_PORTA, in_tune_PA[idx_it]);
		GPIOPinWrite(GPIO_PORTB_BASE, LEDS_PORTB, in_tune_PB[idx_it]);
		GPIOPinWrite(GPIO_PORTC_BASE, LEDS_PORTC, in_tune_PC[idx_it]);
		GPIOPinWrite(GPIO_PORTD_BASE, LEDS_PORTD, in_tune_PD[idx_it]);
		if (++idx_it >= MAX_IDX_IT)
		{
			std_leds = STD_LEDS_SHOW_END;
		} // if
		else
		{
			std_leds_delay = STD_LEDS_DELAY_CNT;
			std_leds = STD_LEDS_SHOW_DELAY;
		} // if
		break;

	case STD_LEDS_SHOW_END:
		if (idxc == CENTS-1)
		{
			std_leds = STD_LEDS_CLEAR_DISPLAY;
		}
		else if ((idxc > 0) || ((idxc == 0) && !in_tune))
		{
			std_leds = STD_LEDS_POSITIVE;
		} // if
		else if (idxc < 0)
		{
			std_leds = STD_LEDS_NEGATIVE;
		} // else if
		break;

	case STD_LEDS_SHOW_DELAY:
		if (--std_leds_delay <= 0)
		{
			std_leds = STD_LEDS_SHOW_IN_TUNE;
		} // if
		break;

	default:
		std_leds = STD_LEDS_CLEAR_DISPLAY;
		break;
	} // switch
} // display_cents()

/*-------------------------------------------------------------------------
 Purpose   : This is the function for setting the Flat-Tuning mode. This
 	 	     mode is entered when the Flat-Tuning switch is pressed.
  Variables: -
  Returns  : true = flat tuning is active, false = flat tuning not active
  -------------------------------------------------------------------------*/
int flat_tuning_active(void)
{
	static int std_ft = STD_FT_NOT_ACTIVE;
	static int std_ft_tmr; // 'flat-tuning active' timer
	int sw_pressed;        // Flat-Tuning switch is pressed?
	int rval;              // return-value

	sw_pressed = (is_switch_pressed() == PRESSED); // Switch Debouncing filter
	switch (std_ft)
	{
        case STD_FT_NOT_ACTIVE:
        	if (sw_pressed)
        	{
        		std_ft     = STD_FT_ACTIVE;
        		std_ft_tmr = STD_FT_TIME;   // set timer
        	}
        	rval = false; // flat-tuning is not active
        	break;

        case STD_FT_ACTIVE:
			if (--std_ft_tmr <= 0)
			{
				std_ft     = STD_FT_NOT_ACTIVE;
			}
			else if (!sw_pressed)
        	{
        		std_ft     = STD_FT_CHANGE;
        	}
        	rval = true; // flat-tuning is active
        	break;

        case STD_FT_CHANGE:
			if (--std_ft_tmr <= 0)
			{
				std_ft     = STD_FT_NOT_ACTIVE;
			}
			else if (sw_pressed)
        	{
        		std_ft     = STD_FT_ACTIVE;
        		std_ft_tmr = STD_FT_TIME;   // reset timer
        		if (++flat_tuning_mode > 6)
        		{	// adjust flat tuning
        			flat_tuning_mode = 0;
        		} // if
        	} // else if
        	rval = true; // flat-tuning is active
        	break;

        default:
        	std_ft = STD_FT_NOT_ACTIVE;
        	break;
	} // switch
    return rval;
} // flat_tuning_active()

/*-------------------------------------------------------------------------
 Purpose   : This is the function for debouncing and reading a switch.
             The switch to read is connected to PE5 and serves as the
             Flat-Tuning switch.
  Variables: -
  Returns  : 1 = switch pressed, 0 = switch is not pressed
  -------------------------------------------------------------------------*/
int is_switch_pressed(void)
{
	static int std_sw = STD_SW_NOT_PRESSED; // STD state number
	static int std_sw_tmr;                  // debouncing timer
    int        pressed;                     // actual switch value
    int        rval = NOT_PRESSED;          // return-value

    pressed = !(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_5) == GPIO_PIN_5);
    switch (std_sw)
	{
        case STD_SW_NOT_PRESSED:
			if (pressed)
			{
				std_sw     = STD_SW_DEBOUNCING;
				std_sw_tmr = STD_SW_TIME;
			}
			break;

        case STD_SW_DEBOUNCING:
			if (--std_sw_tmr <= 0)
			{
				std_sw     = STD_SW_PRESSED;
			}
			break;

        case STD_SW_PRESSED:
			if (!pressed)
			{
				std_sw     = STD_SW_NOT_PRESSED;
			}
			rval = PRESSED;
			break;

        default:
        	std_sw = STD_SW_NOT_PRESSED;
        	break;
	} // switch
    return rval;
} // is_switch_pressed()

/*-------------------------------------------------------------------------
 Purpose   : This is the Interrupt handler for Timer 1 (200 Hz). It is used
             to multiplex the two 7-segment display and to set the LEDs
             for the indication of CENTS deviation.
  Variables: -
  Returns  : -
  -------------------------------------------------------------------------*/
void Timer1IntHandler(void)
{
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT); // Clear timer interrupt
	display_note();  // update 7-segment displays
//	display_cents(); // update cents-LEDs
} // Timer1IntHandler()

/*-------------------------------------------------------------------------
 Purpose   : This is the Interrupt that is called every time when
             sequencer 3 (SS3) of ADC0 is full (1 sample).
             The Sample Frequency is FS Hz, this is controlled by Timer 0.

             The sample is stored in O_ringbuf[]
  Variables: -
  Returns  : -
  -------------------------------------------------------------------------*/
void ADC0_SS3_IntHandler(void)
{
	uint32_t ui32ADC0Value; // holds ADC sample
	static int    irq_cnt = 0; // interrupt counter

	ADCIntClear(ADC0_BASE, 3); // clear interrupt flag
	ADCSequenceDataGet(ADC0_BASE, 3, &ui32ADC0Value); // copy sequencer contents to buffer
	RingBufWriteOne(&O_ringbuf, ui32ADC0Value); // write data into ringbuffer

	if (++irq_cnt >= SAMPLES)
	{  // required number of samples stored in adc_ringbuf[] ?
		irq_cnt     = 0;
		sampled_new = true; // flag for main() to start Yin calculation
	} // if
} // ADC0_SS3_IntHandler()

/*-------------------------------------------------------------------------
  Purpose  : This function fills a buffer with samples for use with YIN
  Variables: O_ringbuf (global): the ringbuffer object with ADC samples
             buf_int (global)  : temporary buffer
             yin_buf (global)  : the buffer with ADC samples for Yin
  Returns  : -
  -------------------------------------------------------------------------*/
void store_in_yin_buffer(void)
{
	int i;

	// Store the data read from the ADC into yin_buf[]
	RingBufRead(&O_ringbuf, buf_int, SAMPLES);
	for (i = 0; i < SAMPLES; i++)
	{
		yin_buf[i] = (float)(buf_int[i]);
	} // for i
} // store_in_yin_buffer()

/*-------------------------------------------------------------------------
  Purpose   : This function finds the note that is nearest to the note
             found by the Yin Pitch detection algorithm
  Variables :
       pitch: the frequency found by the Pitch detector
       cents: the deviation (+ or -) in cents of pitch from the real note
  Returns   : the note index or -1 in case of an error
  -------------------------------------------------------------------------*/
int find_nearest_note(float pitch, float *cents)
{
	int   i;
	int   note_found = -1;     // index in note_freq[]
	float m          = 9999.0; // min. difference
	float m2;
	float range;     // 100 % range

	for (i = 0; i < MAX_NOTES; i++)
	{
		m2 = fabs(pitch - note_freq[i]);
		if (m2 < m)
		{
			note_found = i;
			m          = m2;
		} // if
	} // for
	if (note_found == MAX_NOTES-1)
	{
		note_found = -1;
	}
	else
	{
		// calculate 100 % range in Hz (always positive!)
		if (pitch >= note_freq[note_found])
		{	// note is sharp
			range  = note_freq[note_found+1] - note_freq[note_found];
		} // if
		else
		{  // note is bes
			range  =  note_freq[note_found] - note_freq[note_found-1];
		}
		*cents  = 100.0 * (pitch - note_freq[note_found]); // 100 %
		*cents /= range;
	} // else
	return note_found;
} // find_nearest_note()

/*-------------------------------------------------------------------------
  Purpose   : Set up all GPIO peripherals for use with the guitar-tuner.
              See main.h for a precise description of all IO used.
              There are 2 pitfalls here:
              1) PD7 and PF0 are multiplexed with the NMI pin. By default
                 this is locked, so we have to unlock these pins before
                 we can assign them here.
              2) The Launchpad has (for some reason) PB7 connected to PD1
                 (resistor R10 of 0 Ohm) and has also connected PB6 to PD0
                 (resistor R9 of 0 Ohm). Remove these 2 resistors from the
                 launchpad PCB prior to using these pins!
  Variables : -
  Returns   : -
  -------------------------------------------------------------------------*/
void init_gpio_peripherals(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);  // Enable UART0 on PA0 and PA1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, LEDS_PORTA); // Enable PA2 .. PA7
	GPIOPadConfigSet(GPIO_PORTA_BASE, LEDS_PORTA, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	//GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, LEDS_PORTB); // Enable PB0 .. PB7
	//GPIOPadConfigSet(GPIO_PORTB_BASE, LEDS_PORTB, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, LEDS_PORTC); // Enable PC4 .. PC7
	GPIOPadConfigSet(GPIO_PORTC_BASE, LEDS_PORTC, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);  // Enable PD0 .. PD2 + PD6 + PD7
	// StellarisWare GPIO_LOCK_KEY_DD -> TivaWare GPIO_LOCK_KEY (hw_gpio.h)
	HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY; // Unlock PD7 (which is locked as NMI pin)
	HWREG(GPIO_PORTD_BASE + GPIO_O_CR)   = GPIO_PIN_7;
	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, LEDS_PORTD|SEG7_PORTD);
	GPIOPadConfigSet(GPIO_PORTD_BASE, LEDS_PORTD, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // Enable PE0, PE1, PE2 and PE4
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);  // ADC0 is used with AIN0 on port E3
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, SEG7_PORTE|SEG7_DP_PORTE|LED_SHARP_PORTE);
	GPIOPadConfigSet(GPIO_PORTE_BASE, LED_SHARP_PORTE, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5); // Enable Flat_Tuning switch on port E5
	GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // enable Pull-Up

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	// StellarisWare GPIO_LOCK_KEY_DD -> TivaWare GPIO_LOCK_KEY (hw_gpio.h)
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY; // Unlock PF0 (which is multiplexed with NMI pin)
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR)   = GPIO_PIN_0;
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, SEG7_PORTF); // Enable PF0 .. PF4
} // init_gpio_peripherals()

/*-------------------------------------------------------------------------
  Purpose   : Set up Timer 0, which is used as the source for triggering
              the AD-Converter. 1 sample (ADC0 SS3) with HW Averaging (16 x)
              results in a conversion time of 1/7812.5 sec = 0.128 msec.

              The sample-frequency for the guitar-tuner is FS Hz or 1/FS sec.
              Make sure that this sample-period is always larger than the
              conversion time! Or in a formula: FS < (ADC-CLK / HW_AVERAGING)
  Variables : -
  Returns   : -
  -------------------------------------------------------------------------*/
void init_timer0_for_adc(void)
{
	unsigned long ulPeriod;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);        // Enable ADC0 peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);      // Enable Timer 0

	// StellarisWare TIMER_CFG_32_BIT_PER -> TivaWare TIMER_CFG_PERIODIC
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);   // Timer 0: 32 bit periodic mode
	ulPeriod = (SysCtlClockGet() / FS);                // Set sample-frequency to FS Hz
	TimerLoadSet(TIMER0_BASE, TIMER_A, ulPeriod - 1);  // Load value in timer 0
	TimerControlStall(TIMER0_BASE, TIMER_A, true);     // Stop Timer 0
	TimerControlTrigger(TIMER0_BASE, TIMER_A, true);   // Timer 0 controls ADC0

	// SysCtlADCSpeed in Tivaware no longer needed
	//SysCtlADCSpeedSet(SYSCTL_ADCSPEED_125KSPS);        // Set ADC-Clock to 125 kHz
	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_EIGHTH, 1);
	ADCHardwareOversampleConfigure(ADC0_BASE, 16);     // Average 16 samples together in SS3 (1 sample)

	ADCSequenceDisable(ADC0_BASE, 3);                  // Disable Sample Sequencer 3 (SS3)
	ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0); // Use ADC0 with SS3, Timer triggers, highest priority
	ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END); // Sample AIN0 on Port E3
	ADCSequenceEnable(ADC0_BASE, 3);				   // Enable SS3
	ADCIntRegister(ADC0_BASE, 3, ADC0_SS3_IntHandler); // Register ADC0 - SS3 Interrupt
	ADCIntEnable(ADC0_BASE, 3);						   // Enable ADC0 - SS3 Interrupt
	TimerEnable(TIMER0_BASE, TIMER_A);                 // Enable Timer 0
} // init_timer0_for_adc()

/*-------------------------------------------------------------------------
  Purpose   : Set up TIMER1, which is used as an interrupt source for
              multiplexing the 7-segment displays and for blinking the LEDs
              that indicate the CENTS deviation.
			  The Interrupt frequency is set to 200 Hz.
  Variables : -
  Returns   : -
  -------------------------------------------------------------------------*/
void init_timer1(void)
{
	unsigned long ulPeriod;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);      // Enable Timer 1
	// StellarisWare TIMER_CFG_32_BIT_PER -> TivaWare TIMER_CFG_PERIODIC
	TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);   // Timer 1: 32 bit periodic mode
	ulPeriod = (SysCtlClockGet() / (TIMER1_FREQ));     // Set multiplexing frequency
	TimerLoadSet(TIMER1_BASE, TIMER_A, ulPeriod -1);   // Load value in timer 1
	IntEnable(INT_TIMER1A);							   // Enable Timer 1 Interrupt in NVIC
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);   // Enable Timer 1 Interrupt
	TimerEnable(TIMER1_BASE, TIMER_A);                 // Enable Timer 1
} // init_timer1()














void initI2C0(void)
{
   SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

   //reset I2C module
   SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

   //enable GPIO peripheral that contains I2C
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

   // Configure the pin muxing for I2C0 functions on port B2 and B3.
   GPIOPinConfigure(GPIO_PB2_I2C0SCL);
   GPIOPinConfigure(GPIO_PB3_I2C0SDA);

   // Select the I2C function for these pins.
   GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
   GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

   // Enable and initialize the I2C0 master module.  Use the system clock for
   // the I2C0 module.  The last parameter sets the I2C data transfer rate.
   // If false the data rate is set to 100kbps and if true the data rate will
   // be set to 400kbps.
   I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);

   //clear I2C FIFOs
   HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;
}



void I2CSendString(uint32_t slave_addr, char array[])
{
    // Tell the master module what address it will place on the bus when
    // communicating with the slave.
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);

    //put data to be sent into FIFO
    I2CMasterDataPut(I2C0_BASE, array[0]);

    //if there is only one argument, we only need to use the
    //single send I2C function
    if(array[1] == '\0')
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));
    }

    //otherwise, we start transmission of multiple bytes on the
    //I2C bus
    else
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //initialize index into array
        uint8_t i = 1;

        //send num_of_args-2 pieces of data, using the
        //BURST_SEND_CONT command of the I2C module
        while(array[i + 1] != '\0')
        {
            //put next piece of data into I2C FIFO
            I2CMasterDataPut(I2C0_BASE, array[i++]);

            //send next data that was just placed into FIFO
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

            // Wait until MCU is done transferring.
            while(I2CMasterBusy(I2C0_BASE));
        }

        //put last piece of data into I2C FIFO
        I2CMasterDataPut(I2C0_BASE, array[i]);

        //send next data that was just placed into FIFO
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));
    }
}










/*-------------------------------------------------------------------------
  Purpose   : Entry-point for the Guitar-Tuner program.
  Variables : -
  Returns   : -
  -------------------------------------------------------------------------*/
int main(void)
{
	float pitch;        // the frequency found by Yin
	int   i;            // temp. variable
	int   nnote = 0;    // index of note found
	int   centsint = 0;
	int   s1, s2, s3, s4, rem;
	int   sn = 0;

	FPULazyStackingEnable(); // Enable lazy-stacking for FPU
	FPUEnable();             // Enable FPU

	// Initialise all buffers
	for (i = 0; i < ADC_BUF_SIZE; i++) adc_ringbuf[i] = 0;
	RingBufInit(&O_ringbuf, adc_ringbuf, sizeof(adc_ringbuf)/sizeof(adc_ringbuf[0]));
	Yin_init(&yin, SAMPLES, YIN_THRESHOLD); // Init. struct for Yin Pitch detection algorithm

	SysCtlClockSet(SYSCTL_SYSDIV_3|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); // Clock = 200/3 = 66.67 MHz
	init_gpio_peripherals(); // Initialise all GPIO peripherals
	init_timer0_for_adc();   // Init. timer0 to control the sample-frequency for the ADC peripheral
	init_timer1();           // Init. timer1 at 200 Hz for 7-segment displays and the LEDs

	IntMasterEnable(); // master interrupt enable

	// StellarisWare UARTStdioInit() -> TivaWare UARTStdioConfig()
    //UARTStdioInit(0); //tells uartstdio functions to use UART0
	UARTStdioConfig(0, 115200, SysCtlClockGet());
    UARTprintf("\033[2J\033[HWelcome to Guitar-Tuner Yin!\n"); // erase screen, put cursor at home position (0,0), prompt

    initI2C0();


	while(1) // endless loop
	{

		if (sampled_new) // set by ADC0_SS3_IntHandler()
		{   // This should occur every TIME_WINDOW msec.
			store_in_yin_buffer(); // Read samples and store in Yin buffer
			pitch = Yin_getPitch(&yin, yin_buf); // calculate best pitch

			if (pitch >= 0.0)
			{
				nnote = find_nearest_note(pitch, &cents);

			}
			else nnote = -1;

			IntMasterDisable();   // master interrupt disable
			nearest_note = nnote; // copy for TIMER1 interrupt

			if (nnote>-1) {
			                centsint=(cents+50) * 100;
			                char out[10]={'\0\0\0\0\0\0\0\0\0\0'};
			                s1 = centsint/1000;
			                rem = centsint-(s1*1000);
			                s2 = rem/100;
			                rem = rem-(s2*100);
			                s3 = rem/10;
			                s4 = rem-(s3*10);
			                out[0]=s1+0x30;
			                out[1]=s2+0x30;
			                out[2]=s3+0x30;
			                out[3]=s4+0x30;
			                out[4]='|';
			                strcat(out,note_name[nnote]);
			                I2CSendString(8,out);
			                }
			sn = sn + 1;
			if (sn> 10 ) {
			 sn = 0;
			 char wd[2]={'\0\0'};
			 wd[0]=0x57;
			 I2CSendString(8,wd);
			}
			IntMasterEnable();    // master interrupt enable
			sampled_new  = false;
		} // if
	} // while
} // main()
