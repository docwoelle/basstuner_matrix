/*==================================================================
  Function name: -
  File name    : main.h
  CVS ID       : $Id: $
  Author       : E. vd Logt
  Date         : 2013-10-20
  ------------------------------------------------------------------
  Purpose : This file contains the header file for the guitar-tuner.

          +3V3 VBUS +5V             C_7s PF2  GND
     CP30 PB5  GND  GND             D_7s PF3  PB2   CM20
     CP05 PB0  PD0  CP40            CM25 PB3  PE0   F_7s
     CP10 PB1  PD1  CP45            CM35 PC4  PF0   A_7s
     LDsh PE4  PD2  CP50            CM40 PC5  RESET
   Flat_T PE5  PD3                  CM45 PC6  PB7   C000
     CP25 PB4  PE1  G_7s            CM50 PC7  PB6   CM30
     CP15 PA5  PE2  dp_7s        MPX1_7s PD6  PA4   CM15
     CP20 PA6  PE3  AIN0         MPX2_7s PD7  PA3   CM10
     CP35 PA7  PF1  B_7s            E_7s PF4  PA2   CM05

       Usage: PA2..PA7: CM05, CM10, CM15, CP15, CP20, CP35
              PB0..PB7: CP05, CP10, CM20, CM25, CP25, CP30, CM30, C000
              PC4..PC7: CM35, CM40, CM45, CM50
              PD0..PD2: CP40, CP45, CP50
              PD6..PD7: MPX1_7s, MPX2_7s
              PE0..PE2: 7 segment f,g,dp
              PE4     : Led for indication of Sharp Note
              PE5     : Flat Tuning switch (0 = pressed)
              PF0..PF4: 7 segment a,b,c,d,e

       CM05..CM50: PA2, PA3, PA4, PB2, PB3, PB6, PC4, PC5, PC6, PC7
       C000      : PB7
       CP05..CP50: PB0, PB1, PA5, PA6, PB4, PB5, PA7, PD0, PD1, PD2
       LDsh      : PE4
       7-segment : PF0, PF1, PF2, PF3, PF4, PE0, PE1, PE2

       Description: CM05, CM10 etc.: Minus 5 cents, Minus 10 cents, etc.
                    CP05, CP10 etc.: Plus  5 cents, Plus  10 cents, etc.
                    C000           : In tune (+- 0 cents)
 ------------------------------------------------------------------*/
#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "../tivaware/inc/hw_ints.h"
#include "../tivaware/inc/hw_memmap.h"
#include "../tivaware/inc/hw_types.h"
#include "../tivaware/inc/hw_gpio.h"

#include "../tivaware/driverlib/adc.h"
#include "../tivaware/driverlib/fpu.h"
#include "../tivaware/driverlib/gpio.h"
#include "../tivaware/driverlib/interrupt.h"
#include "../tivaware/driverlib/pin_map.h"
#include "../tivaware/driverlib/sysctl.h"
#include "../tivaware/driverlib/uart.h"
#include "../tivaware/utils/uartstdio.h"
#include "../tivaware/driverlib/timer.h"

#include "ringbuf.h"
#include "Yin.h"

#include "../tivaware/inc/hw_i2c.h"
#include "../tivaware/driverlib/i2c.h"
#include "../tivaware/driverlib/pin_map.h"

//---------------------------------------------------------------------
// There are 7 notes in a scale: A, B, C, D, E, F, G + '-' + 1 empty
// There are a total of 7 scales for this guitar-tuner: from E0 to D#6 + 1 empty
// There are a total of 74 notes in the range for this guitar-tuner
//---------------------------------------------------------------------
#define NOTES     (9)
#define MINUS	  (NOTES-2)
#define SCALES    (8)
#define MAX_NOTES (74)

#ifndef SYSCTL_ADCSPEED_125KSPS
#define SYSCTL_ADCSPEED_125KSPS (0x00000000)
#endif

//---------------------------------------------------------------------
// The 7-segment displays are connected to PORTE and PORTF
// The 7-segment multiplexing is connected to PORTD
//---------------------------------------------------------------------
#define SEG7_PORTE    (GPIO_PIN_0|GPIO_PIN_1)
#define SEG7_PORTF    (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4)
#define SEG7_PORTD    (GPIO_PIN_6|GPIO_PIN_7)
#define SEG7_DP_PORTE (GPIO_PIN_2)

//---------------------------------------------------------------------
// There are 12 deviations (including an empty one) for the CENTS LEDs
// The CENTS LEDs are connected to PORTA, PORTB, PORTC and PORTD
//---------------------------------------------------------------------
#define CENTS (12)
#define LEDS_PORTA (GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)
#define LEDS_PORTB (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)
#define LEDS_PORTC (GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)
#define LEDS_PORTD (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2)
#define LED_SHARP_PORTE (GPIO_PIN_4)

//---------------------------------------------------------------------
// MAX_IDT_IT: 'LED-show' #steps when the deviation is less than 1 %.
// STD_LEDS_DELAY_CNT: #5 msec. delay steps
// STD_LEDS_xx       : State Transition Diagram states for CENTS lEDs
//---------------------------------------------------------------------
#define MAX_IDX_IT             (11)
#define STD_LEDS_DELAY_CNT     (10)

#define STD_LEDS_CLEAR_DISPLAY (0)
#define STD_LEDS_POSITIVE      (1)
#define STD_LEDS_NEGATIVE      (2)
#define STD_LEDS_SHOW_IN_TUNE  (3)
#define STD_LEDS_SHOW_END      (4)
#define STD_LEDS_SHOW_DELAY    (5)

//---------------------------------------------------------------------
// STD_FT_TIME: #times x 5 msec. for Flat-Tuning time active
//---------------------------------------------------------------------
#define STD_FT_TIME           (400)
#define STD_FT_NOT_ACTIVE	    (0)
#define STD_FT_ACTIVE           (1)
#define STD_FT_CHANGE		    (2)

//---------------------------------------------------------------------
// STD_SW_TIME: #times x 5 msec. for switch debouncing
//---------------------------------------------------------------------
#define NOT_PRESSED             (0)
#define PRESSED                 (1)
#define STD_SW_TIME            (10)
#define STD_SW_NOT_PRESSED	    (0)
#define STD_SW_DEBOUNCING       (1)
#define STD_SW_PRESSED		    (2)

//-----------------------------------------------------
// TIMER1_FREQ  : Frequency for 7-segment multiplexing
// FS           : Sample-frequency of guitar sound [Hz]
// TIME_WINDOW  : Time in msec. for calculation of Yin
// SAMPLES      : The number of samples in TIME_WINDOW
// ADC_BUF_SIZE : The size of adc_ringbuf[]
// YIN_THRESHOLD: Yin lower limit detection parameter
//-----------------------------------------------------
#define TIMER1_FREQ  (200)
#define FS           (6000)
#define TIME_WINDOW  (100)
#define KEEP_ALIVE   (1000)
#define SAMPLES      ((FS * TIME_WINDOW) / 1000)
#define ADC_BUF_SIZE (((SAMPLES * 12) + 5) / 10)
#define YIN_THRESHOLD (0.10)

void display_note(void);
void display_cents(void);
int  flat_tuning_active(void);
int  is_switch_pressed(void);
void store_in_yin_buffer(void);
int  find_nearest_note(float pitch, float *cents);
void init_gpio_peripherals(void);
void init_timer0_for_adc(void);
void init_timer1(void);

#endif /* MAIN_H_ */
