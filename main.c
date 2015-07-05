/* ******************************************************************************
 * 	VSCP (Very Simple Control Protocol) 
 * 	http://www.vscp.org
 *
 *  Beijing I/O module
 * 	akhe@grodansparadis.com
 *
 *  Copyright (C) 1995-2015 Ake Hedman, Grodans Paradis AB
 *                          <akhe@grodansparadis.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 
 *	This file is part of VSCP - Very Simple Control Protocol 	
 *	http://www.vscp.org
 *
 * ******************************************************************************
 */

#include "vscp_compiler.h"
#include "vscp_projdefs.h"

#include <p18cxxx.h>
#include <timers.h>
#include <delays.h>
#include <inttypes.h>
#include <string.h>
#include <ecan.h>
#include <vscp_firmware.h>
#include <vscp_class.h>
#include <vscp_type.h>
#include "beijing.h"
#include "version.h"

#if defined(RELEASE)

#pragma config WDT = ON, WDTPS = 128
#pragma config OSC = HSPLL
#pragma config BOREN = BOACTIVE
#pragma config STVREN = ON
#pragma config BORV = 3
#pragma config LVP = ON
#pragma config CPB = ON
#pragma config BBSIZ = 2048
#pragma config WRTD  = OFF

#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF

#pragma config EBTRB = OFF

#else

#pragma config WDT = OFF
#pragma config OSC = HSPLL
#pragma config PWRT = ON
#pragma config BOREN = BOACTIVE
#pragma config STVREN = ON
#pragma config BORV = 3
#pragma config LVP = OFF
#pragma config CPB = OFF
#pragma config WRTD  = OFF

#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF

#pragma config EBTRB = OFF

#endif

// Calculate and st required filter and mask
// for the current decision matrix
void calculateSetFilterMask( void );

// The device URL (max 32 characters including null termination)
const uint8_t vscp_deviceURL[] = "www.eurosource.se/beijing_2.xml";

volatile unsigned long measurement_clock_sec;  // Clock for one second work


volatile unsigned long measurement_clock_10ms;  // Debounce clock
uint8_t debounce_cnt[ 10 ];                     // Debaunce counter
uint8_t current_debounce;                       // I/O channel to check

uint8_t sendTimer;  // Timer for CAN send
uint8_t seconds;    // counter for seconds
uint8_t minutes;    // counter for minutes
uint8_t hours;      // Counter for hours

// Bit set for channel that should pulse
// bit 0 - channel 0 etc
uint16_t channel_pulse_flags;

// An array with 16-bit timers one
// for each channel
uint16_t channel_pulse_timer[10];

// An array with 16-bit timers one
// for each channel
uint16_t channel_protection_timer[10];

// This is the I/O-state when the last one second work
// was carried out
uint16_t current_iostate;



//__EEPROM_DATA(0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88);

///////////////////////////////////////////////////////////////////////////////
// Isr() 	- Interrupt Service Routine
//      	- Services Timer0 Overflow
//      	- Services GP3 Pin Change
//////////////////////////////////////////////////////////////////////////////

#ifdef RELOCATE

#else

#endif

void interrupt low_priority  interrupt_at_low_vector( void )
{
    // Clock
    if ( INTCONbits.TMR0IF ) { // If a Timer0 Interrupt, Then...

        // Reload value for 1 ms resolution
        WriteTimer0(TIMER0_RELOAD_VALUE);
        
        vscp_timer++;
        vscp_configtimer++;
        measurement_clock_10ms++;
        measurement_clock_sec++;

        // Check for init. button
        if ( INIT_BUTTON ) {
            vscp_initbtncnt = 0;
        } 
        else {
            // Active
            vscp_initbtncnt++;
        }

        // Status LED
        vscp_statuscnt++;
        if ( ( VSCP_LED_BLINK1 == vscp_initledfunc ) &&
                ( vscp_statuscnt > 100 ) ) {

            if ( STATUS_LED ) {
                STATUS_LED = 0;
            }
            else {
                STATUS_LED = 1;
            }

            vscp_statuscnt = 0;

        }
        else if (VSCP_LED_ON == vscp_initledfunc) {
            STATUS_LED = 1;
            vscp_statuscnt = 0;
        }
        else if (VSCP_LED_OFF == vscp_initledfunc) {
            STATUS_LED = 0;
            vscp_statuscnt = 0;
        }

        INTCONbits.TMR0IF = 0; // Clear Timer0 Interrupt Flag

    }

    return;
}


//***************************************************************************
// Main() - Main Routine
//***************************************************************************


void main()
{

    init();         // Initialize Micro controller
    
    // Check VSCP persistent storage and
    // restore if needed
    if ( !vscp_check_pstorage() ) {

        // Spoiled or not initialized - reinitialize
        init_app_eeprom();

    }
    
    // Initialize data
    init_app_ram();
    
    // Initialize the VSCP functionality
    vscp_init();    
            
    while ( 1 ) {   // Loop Forever

        ClrWdt();   // Feed the dog

        if ( ( vscp_initbtncnt > 250 ) &&
                ( VSCP_STATE_INIT != vscp_node_state ) ) {

            // Init. button pressed
            vscp_nickname = VSCP_ADDRESS_FREE;
            eeprom_write( VSCP_EEPROM_NICKNAME, VSCP_ADDRESS_FREE );
            vscp_init();
            
        }

        // Check for a valid  event
        vscp_imsg.flags = 0;
        vscp_getEvent();

        switch ( vscp_node_state ) {

            case VSCP_STATE_STARTUP: // Cold/warm reset

                // Get nickname from EEPROM
                if (VSCP_ADDRESS_FREE == vscp_nickname) {
                    // new on segment need a nickname
                    vscp_node_state = VSCP_STATE_INIT;
                } 
                else {
                    // been here before - go on
                    vscp_node_state = VSCP_STATE_ACTIVE;
                    vscp_goActiveState();
                }
                break;

            case VSCP_STATE_INIT: // Assigning nickname
                vscp_handleProbeState();
                break;

            case VSCP_STATE_PREACTIVE:  // Waiting for host initialization
                vscp_goActiveState();
                break;

            case VSCP_STATE_ACTIVE:     // The normal state

                // Check for incoming event?
                if (vscp_imsg.flags & VSCP_VALID_MSG) {

                    if ( VSCP_CLASS1_PROTOCOL == vscp_imsg.vscp_class  ) {

                        // Handle protocol event
                        vscp_handleProtocolEvent();

                    }
                    else {    
                        doDM();
					}
                    
                }
                
                if ( measurement_clock_10ms >= 10 ) {
                    
                    uint8_t dir = 
                        ( eeprom_read( VSCP_EEPROM_END +
                                        REG0_BEIJING_IO_DIRECTION_MSB ) << 8 ) +
                        eeprom_read( VSCP_EEPROM_END +
                                        REG0_BEIJING_IO_DIRECTION_LSB );

                    measurement_clock_10ms = 0;

                    if ( ( dir & ( 1 << current_debounce ) ) &&         // Input))
                            ( eeprom_read( VSCP_EEPROM_END +            // debounce
                                            REG0_BEIJING_CH0_INPUT_CTRL +
                                            current_debounce ) &
                                                INPUT_CTRL_DEBOUNCE ) &&
                            ( eeprom_read( VSCP_EEPROM_END +            // Enabled
                                            REG0_BEIJING_CH0_INPUT_CTRL +
                                            current_debounce ) &
                                                INPUT_CTRL_ENABLE ) ) {
      
                        switch (current_debounce) {

                            case 0:
                                if (!CHANNEL0) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 1:
                                if (!CHANNEL1) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 2:
                                if (!CHANNEL2) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 3:
                                if (!CHANNEL3) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 4:
                                if (!CHANNEL4) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 5:
                                if (!CHANNEL5) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 6:
                                if (!CHANNEL6) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 7:
                                if (!CHANNEL7) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 8:
                                if (!CHANNEL8) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            case 9:
                                if (!CHANNEL9) {
                                    debounce_cnt[ current_debounce ]++;
                                    if (debounce_cnt[ current_debounce ] > 3) {

                                        debounce_cnt[ current_debounce ] = 0;

                                        // Send On/TurnOn event
                                        if (eeprom_read(VSCP_EEPROM_END +
                                                REG0_BEIJING_CH0_OUTPUT_CTRL +
                                                current_debounce) &
                                                INPUT_CTRL_EVENT_SELECT) {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON);
                                        } else {
                                            SendInformationEvent(current_debounce,
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON);
                                        }
                                    }
                                } else {
                                    debounce_cnt[ current_debounce ] = 0;
                                }
                                break;

                            default:
                                current_debounce = 0;
                                break;
                        }

                    }
                    
                    current_debounce++;
                    if ( current_debounce > 9 ) current_debounce = 0;
                    
                }
                
                break;

            case VSCP_STATE_ERROR: // Everything is *very* *very* bad.
                vscp_error();
                break;

            default: // Should not be here...
                vscp_node_state = VSCP_STATE_STARTUP;
                break;

        }

        // do a measurement if needed
        if ( measurement_clock_sec > 1000 ) {

            measurement_clock_sec = 0;
            sendTimer++;
 
            // Do VSCP one second jobs
            vscp_doOneSecondWork();

            // Temperature report timers are only updated if in active
            // state GUID_reset
            if ( VSCP_STATE_ACTIVE == vscp_node_state ) {
                // Do VSCP one second jobs
                doApplicationOneSecondWork();
            }

        }

        // Timekeeping
        if ( seconds > 59 ) {

            seconds = 0;
            minutes++;

            if ( minutes > 59 ) {
                minutes = 0;
                hours++;
            }

            if ( hours > 23 ) hours = 0;

        }

        doWork();

    } // while
}

///////////////////////////////////////////////////////////////////////////////
// Init. - Initialization Routine
//  

void init()
{   
    //uint8_t msgdata[ 8 ];

    // Initialize the uP
    
    // Deselect comparators and ADC's so PORTA/PORTB
    // can be used as inputs.
    ADCON1 = 0x0f;
    //CMCON = 0x07;     // Not implemented on PIC18F2580

    // PORTA
    // RA0/AN0  - channel 9
    // RA1/AN1  - channel 8
    // RA2/AN2  - channel 7
    // RA3/AN3  - output
    // RA4      - output
    // RA5/AN4  - output
    TRISA = 0b00000000;
    PORTA = 0x00;  // Default off

    // PortB

    // RB0/AN10     - Channel 0
    // RB1/AN8      - Channel 1
    // RB2 CAN TX   - output
    // RB3 CAN RX   - input
    // RB4/AN9      - Not used = output.
    // RB5/LVPGM    - Not used = output.
    // RB&/PGC      - Not used = output.
    // RB7/PGD      - Not used = output.
    TRISB = 0b00001000;
    PORTB = 0x00;  // Default off

    // RC0 - Input  - Init. button
    // RC1 - Output - Status LED - Default off
    // RC2 - Output - Not used = output.
    // RC3 - Output - Channel 2.
    // RC4 - Output - Channel 3.
    // RC5 - Output - Channel 4.
    // RC6 - Output - Channel 5.
    // RC7 - Output - Channel 6.
    TRISC = 0b00000001;
    PORTC = 0x00;   // Default off

/*
    // Sensor 0 timer
    OpenTimer0( TIMER_INT_OFF &
                    T0_16BIT &
                    T0_PS_1_2 &
                    T0_SOURCE_INT);

    // Sensor 1 timer
    OpenTimer1( TIMER_INT_OFF &
                    T1_16BIT_RW &
                    T1_SOURCE_INT &
                    T1_PS_1_1 &
                    T1_OSC1EN_OFF &
                    T1_SYNC_EXT_OFF);

    // Timer 2 is used as a 1 ms clock
    // 156 is loaded eight time to give ~1250 cycles
    PR2 = TIMER2_RELOAD_VALUE; // Set reload value
    OpenTimer2( TIMER_INT_ON &
                    T2_PS_1_4 &
                    T2_POST_1_16 );

 */

    OpenTimer0( TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_8 );
    WriteTimer0( TIMER0_RELOAD_VALUE );

    // Set channels to there off position
    CHANNEL0 = 0;
    CHANNEL1 = 0;
    CHANNEL2 = 0;
    CHANNEL3 = 0;
    CHANNEL4 = 0;
    CHANNEL5 = 0;
    CHANNEL6 = 0;
    CHANNEL7 = 0;
    CHANNEL8 = 0;
    CHANNEL9 = 0;

    // Initialize CAN
    ECANInitialize();

    // Must be in config. mode to change many of settings.
    //ECANSetOperationMode(ECAN_OP_MODE_CONFIG);

    // Return to Normal mode to communicate.
    //ECANSetOperationMode(ECAN_OP_MODE_NORMAL);

    /*
        msgdata[ 0 ] = 1;
            msgdata[ 1 ] = 2;
            msgdata[ 2 ] = 3;
            msgdata[ 3 ] = 4;

            if ( vscp18f_sendMsg( 0x123,  msgdata, 4, CAN_TX_PRIORITY_0 & CAN_TX_XTD_FRAME & CAN_TX_NO_RTR_FRAME ) ) {
                    ;
            }

     */
    
    // Enable peripheral interrupt
    INTCONbits.PEIE = 1;

    // Enable global interrupt
    INTCONbits.GIE = 1;

    return;
}

///////////////////////////////////////////////////////////////////////////////
// init_app_ram
//

void init_app_ram( void )
{
    uint8_t i;
    uint8_t dir;

    measurement_clock_sec = 0;      // start a new measurement cycle

    seconds = 0;
    minutes = 0;
    hours = 0;

    channel_pulse_flags = 0;      // No pulse outputs yet

    // Clear timers
    memset( channel_pulse_timer, 0, sizeof( channel_pulse_timer ) );
    memset( channel_protection_timer, 0, sizeof( channel_protection_timer ) );

    for ( i=0; i<10; i++ ) {

        // Init. pulsed channels
        if ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + i ) &
                                OUTPUT_CTRL_PULSE ) {

            channel_pulse_flags |= (1<<i); // Enable pulse output
            channel_pulse_timer[ i ] =
                eeprom_read( VSCP_EEPROM_END + REG0_COUNT + REG1_BEIJING_CH0_TIMING_PULSE_MSB + i ) * 256 +
                eeprom_read( VSCP_EEPROM_END + REG0_COUNT + REG1_BEIJING_CH0_TIMING_PULSE_LSB + i );
        }

        // Init. protection timers
        if ( eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH5_OUTPUT_CTRL + i ) & OUTPUT_CTRL_PROTECTION) {
            channel_protection_timer[ i ] =
                eeprom_read(VSCP_EEPROM_END + REG0_COUNT + REG1_BEIJING_CH0_TIMING_PULSE_MSB + i ) * 256 +
                eeprom_read(VSCP_EEPROM_END + REG0_COUNT + REG1_BEIJING_CH0_TIMING_PULSE_LSB + i );
        }

    }
    
    dir = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_MSB );
    TRISA = ( dir & 0x03 ) | 0b00000000;

    dir = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_LSB );
    TRISB = ( dir & 0x03 ) | 0b00001000;  // Channel 0/1
    
    // Should the weak pull-ups be activated?
    if ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_MODULE_CTRL ) & MODULE_CTRL_PULLUP ) {
        INTCON2bits.RBPU = 0;
    }

    dir = ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_LSB ) << 1 ) & 0b11111000;
    TRISC = dir | 0b00000001;
    
    memset( debounce_cnt, 0, sizeof( debounce_cnt ) );
    current_debounce = 0;   // Check debounce on channel 0
    
    // Save the current I/O state
    current_iostate = ( CHANNEL9 << 9 ) +
                        ( CHANNEL8 << 8 ) +
                        ( CHANNEL7 << 7 ) +
                        ( CHANNEL6 << 6 ) +
                        ( CHANNEL5 << 5 ) +
                        ( CHANNEL4 << 4 ) +
                        ( CHANNEL3 << 3 ) +
                        ( CHANNEL2 << 2 ) +
                        ( CHANNEL1 << 1 ) +
                        CHANNEL0;
}


///////////////////////////////////////////////////////////////////////////////
// init_app_eeprom
//

void init_app_eeprom(void)
{
    unsigned char i, j;

    eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_ZONE, 0 );
    eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_SUBZONE, 0 );
    
    for ( i=0; i<10; i++ ) {
        eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_CH0_SUBZONE + i, i );
    }
    
    eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_MSB, CHANNEL_DEFAULT_DIRECTION_MSB );
    eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_LSB, CHANNEL_DEFAULT_DIRECTION_LSB );

    for ( i=0; i<10; i++ ) {
        eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + i,
                        OUTPUT_CTRL_ONEVENT |
                        OUTPUT_CTRL_OFFEVENT |
                        OUTPUT_CTRL_ENABLED );
    }
    
    for ( i=0; i<10; i++ ) {
        eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_CH0_INPUT_CTRL + i,
                        INPUT_CTRL_EVENT_ON |
                        INPUT_CTRL_EVENT_OFF |
                        INPUT_CTRL_ENABLE );
    }
    
    eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_MODULE_CTRL, 0 );
    
    eeprom_write( VSCP_EEPROM_END + REG0_BEIJING_STREAM_TIMING, 0 );
    
    for ( i=0; i<20; i++ ) {
        eeprom_write( VSCP_EEPROM_END + 
                        REG0_COUNT +  // needed for page 1
                        REG1_BEIJING_CH0_TIMING_PULSE_MSB + i, 0 );
    }

    for ( i=0; i<20; i++ ) {
        eeprom_write( VSCP_EEPROM_END + 
                        REG0_COUNT +  // needed for page 1
                        REG1_BEIJING_CH0_TIMING_PROTECT_MSB + i, 0 );
    }

    // * * * Decision Matrix * * *
    // All elements disabled.
    for ( i = 0; i < DESCION_MATRIX_ROWS; i++ ) {
        for ( j = 0; j < 8; j++ ) {
            eeprom_write( VSCP_EEPROM_END + 
                            REG0_COUNT + REG1_COUNT +
                            REG_DESCION_MATRIX + i * 8 + j, 0 );
        }
    }

}

///////////////////////////////////////////////////////////////////////////////
// doApplicationOneSecondWork
//

void doApplicationOneSecondWork(void)
{

    uint8_t i;
    uint16_t iodirections;  // Directions fro I/O channels
    uint8_t ctrlreg;        // Control register
    BOOL bOn = FALSE;
    BOOL bInput;            // State of input

    // Get direction for channels
    iodirections = ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_MSB ) << 8 ) +
                    eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_IO_DIRECTION_LSB );
    
    for ( i = 0; i < 10; i++ ) {
        
        if (  iodirections & ( 1 << i ) ) {
            
            // * * * Input * * *
            
            // Get input control register for this channel
            ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_INPUT_CTRL + i );
            
            // If not enabled check next
            if ( !( ctrlreg & INPUT_CTRL_ENABLE ) ) continue;
            
            switch ( i ) {
                    
                case 0:
                    bInput = CHANNEL0;
                    break;
                        
                case 1:
                    bInput = CHANNEL1;
                    break;
                        
                case 2:
                    bInput = CHANNEL2;
                    break;
                        
                case 3:
                    bInput = CHANNEL3;
                    break;

                case 4:
                    bInput = CHANNEL4;
                    break;

                case 5:
                    bInput = CHANNEL5;
                    break;

                case 6:
                    bInput = CHANNEL6;
                    break;

                case 7:
                    bInput = CHANNEL7;
                    break;

                case 8:
                    bInput = CHANNEL8;
                    break;

                case 9:
                    bInput = CHANNEL9;
                    break;                        
                }
            
            // Should change ON event be sent?
            if (  ctrlreg & INPUT_CTRL_EVENT_ON ) {
                                
                if ( bInput ) {
                    
                    // Yes if last state was zero
                    if ( !( current_iostate & ( 1 << i ) ) ) {
                        if ( ctrlreg &  INPUT_CTRL_EVENT_SELECT ) {
                            SendInformationEvent( i, 
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNON );
                        }   
                        else {
                            SendInformationEvent( i, 
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_ON );
                        }
                    }
                }
            }
            
            // Should change OFF event be sent?
            if (  ctrlreg & INPUT_CTRL_EVENT_OFF ) {
                
                if ( !bInput ) {
                    
                    // Yes if last state was one
                    if ( current_iostate & ( 1 << i ) ) {
                        if ( ctrlreg &  INPUT_CTRL_EVENT_SELECT ) {
                            SendInformationEvent( i, 
                                                    VSCP_CLASS1_CONTROL,
                                                    VSCP_TYPE_CONTROL_TURNOFF );
                        }
                        else {
                            SendInformationEvent( i, 
                                                    VSCP_CLASS1_INFORMATION,
                                                    VSCP_TYPE_INFORMATION_OFF );
                        }
                    }
                }
            }
            
            // Should alarm high event be sent?
            if ( ctrlreg & INPUT_CTRL_ALARM_HIGH ) {
                
                if ( bInput ) {
                    
                    // Yes if last state was zero
                    if ( !( current_iostate & ( 1 << i ) ) || 
                            ( ctrlreg & INPUT_CTRL_ALARM_CONTINIOUS ) ) {
                        SendInformationEvent( i, 
                                                VSCP_CLASS1_ALARM,
                                                VSCP_TYPE_ALARM_ALARM );
                        vscp_alarmstatus |= ALARM_INPUT_HIGH;
                    }
                    
                }
                
            }
            
            // Should alarm low event be sent?
            if ( ctrlreg & INPUT_CTRL_ALARM_LOW ) {
                
                if (!bInput ) {
                    
                    // Yes if last state was one
                    if ( current_iostate & ( 1 << i ) ||
                            ( ctrlreg & INPUT_CTRL_ALARM_CONTINIOUS ) ) {
                        SendInformationEvent( i, 
                                                VSCP_CLASS1_ALARM,
                                                VSCP_TYPE_ALARM_ALARM );
                        vscp_alarmstatus |= ALARM_INPUT_LOW;
                    }
                    
                }
                
            }
            
        }
        else {
            
            // * * * Output * * *
            
            // Get output control register for this channel
            ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + i );

            // If not enabled check next
            if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) continue;

            // * * * Protection timer * * *

            if ( channel_protection_timer[ i ] ) {

                channel_protection_timer[ i ]--;

                // Check if its time to act on protection time
                if (!channel_protection_timer[ i ] &&
                        ( eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + i ) &
                        OUTPUT_CTRL_PROTECTION ) ) {

                    // Yes - its time to protect this channel
                    doActionOff(0, (1 << i));

                    // Should alarm be sent?
                    if (ctrlreg & OUTPUT_CTRL_ALARM) {
                        SendInformationEvent( i, 
                                                VSCP_CLASS1_ALARM,
                                                VSCP_TYPE_ALARM_ALARM);
                    }
                    
                    vscp_alarmstatus |= ALARM_STATE_PROTECTION;

                }
            } 
            else {
                // Reload protection timer
                channel_protection_timer[ i ] =
                        eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH0_TIMING_PROTECT_MSB + i) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH0_TIMING_PROTECT_LSB + i);
            }

            // * * * Pulsed channels * * *
            if ( channel_pulse_flags & (1 << i) ) {

                if ( channel_pulse_timer[ i ] ) {

                    channel_pulse_timer[ i ]--;

                    // If zero its time for state change
                    if ( !channel_pulse_timer[ i ] ) {

                        switch (i) {

                            case 0:
                                if ( CHANNEL0 ) {
                                    CHANNEL0 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL0 = 1;
                                    bOn = TRUE;
                                }
                                break;

                            case 1:
                                if ( CHANNEL1 ) {
                                    CHANNEL1 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL1 = 1;
                                    bOn = TRUE;
                                }
                                break;

                            case 2:
                                if ( CHANNEL2 ) {
                                    CHANNEL2 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL2 = 1;
                                    bOn = TRUE;
                                }
                                break;

                            case 3:
                                if ( CHANNEL3 ) {
                                    CHANNEL3 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL3 = 1;
                                    bOn = TRUE;
                                }
                                break;

                            case 4:
                                if ( CHANNEL4 ) {
                                    CHANNEL4 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL4 = 1;
                                    bOn = TRUE;
                                }
                                break;

                            case 5:
                                if ( CHANNEL5 ) {
                                    CHANNEL5 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL5 = 1;
                                    bOn = TRUE;
                                }
                                break;

                            case 6:
                                if ( CHANNEL6 ) {
                                    CHANNEL6 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL6 = 1;
                                    bOn = TRUE;
                                }
                                break;
                               
                            case 7:
                                if ( CHANNEL7 ) {
                                    CHANNEL7 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL7 = 1;
                                    bOn = TRUE;
                                }
                                break;
                                
                            case 8:
                                if ( CHANNEL8 ) {
                                    CHANNEL8 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL8 = 1;
                                    bOn = TRUE;
                                }
                                break;
                                
                            case 9:
                                if ( CHANNEL9 ) {
                                    CHANNEL9 = 0;
                                    bOn = FALSE;
                                } 
                                else {
                                    CHANNEL9 = 1;
                                    bOn = TRUE;
                                }
                                break;    
                        }

                        // Reload pulse timer
                        channel_pulse_timer[ i ] =
                                eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH0_TIMING_PULSE_MSB + 2 * i ) * 256 +
                                                eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH0_TIMING_PULSE_MSB + 2 * i );

                        if ( bOn ) {

                            // Reload protection timer
                            if ( eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + i) & 
                                                OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ i ] =
                                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                        REG1_BEIJING_CH0_TIMING_PROTECT_MSB + 2 * i) * 256 +
                                                        eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                        REG1_BEIJING_CH0_TIMING_PROTECT_LSB + 2 * i);
                            }

                            if ( ctrlreg & OUTPUT_CTRL_ONEVENT ) {
                                SendInformationEvent( i, 
                                                        VSCP_CLASS1_INFORMATION,
                                                        VSCP_TYPE_INFORMATION_ON);
                            }


                        } 
                        else {

                            if ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) {
                                SendInformationEvent( i, VSCP_CLASS1_INFORMATION,
                                                        VSCP_TYPE_INFORMATION_OFF);
                            }

                        }

                    } // State change

                } // Something to count down
                else {
                    // Reload the pulse timer
                    channel_pulse_timer[ i ] =
                            eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PULSE_MSB + 2 * i) * 256 +
                                        eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PULSE_LSB + 2 * i);
                }

            } // Pulse bit

        }

    } // for all channels
    
    
    // Check if stream event should be sent
    if ( eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_STREAM_TIMING ) ) {
    
        uint8_t data[ 3 ]; 
        
        data[ 0 ] = 0;  // Data coding (coding=0(bits), unit=0, sensor=0)
        data[ 1 ] = ( CHANNEL9 << 1 ) + 
                        CHANNEL8;
        data[ 2 ] = ( CHANNEL7 << 7 ) + 
                        ( CHANNEL6 << 6 ) +
                        ( CHANNEL5 << 5 ) +
                        ( CHANNEL4 << 4 ) +
                        ( CHANNEL3 << 3 ) +
                        ( CHANNEL2 << 2 ) +
                        ( CHANNEL1 << 1 ) +
                        CHANNEL0;

        // Send event
        sendVSCPFrame( VSCP_CLASS1_DATA,
                        VSCP_TYPE_DATA_IO,
                        vscp_nickname,
                        VSCP_PRIORITY_MEDIUM,
                        3,
                        data );
    }
    
    // Save the current I/O state
    current_iostate = ( CHANNEL9 << 9 ) +
                        ( CHANNEL8 << 8 ) +
                        ( CHANNEL7 << 7 ) +
                        ( CHANNEL6 << 6 ) +
                        ( CHANNEL5 << 5 ) +
                        ( CHANNEL4 << 4 ) +
                        ( CHANNEL3 << 3 ) +
                        ( CHANNEL2 << 2 ) +
                        ( CHANNEL1 << 1 ) +
                        CHANNEL0;
}

///////////////////////////////////////////////////////////////////////////////
// Get Major version number for this hardware module
//

unsigned char getMajorVersion()
{
    return FIRMWARE_MAJOR_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
// Get Minor version number for this hardware module
//

unsigned char getMinorVersion()
{
    return FIRMWARE_MINOR_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
// Get Subminor version number for this hardware module
//

unsigned char getSubMinorVersion()
{
    return FIRMWARE_SUB_MINOR_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
// Get GUID from EEPROM
//

#ifdef ENABLE_WRITE_2PROTECTED_LOCATIONS

void vscp_setGUID(uint8_t idx, uint8_t data ) {
    if ( idx>15 ) return;
    eeprom_write(VSCP_EEPROM_REG_GUID + idx, data);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Get Manufacturer id and subid from EEPROM
//

#ifdef ENABLE_WRITE_2PROTECTED_LOCATIONS

void vscp_setManufacturerId( uint8_t idx, uint8_t data ) {
    if ( idx>7 ) return;
    eeprom_write(VSCP_EEPROM_REG_MANUFACTUR_ID0 + idx, data);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Get the bootloader algorithm code
//

unsigned char getBootLoaderAlgorithm(void)
{
    return VSCP_BOOTLOADER_PIC1;
}

///////////////////////////////////////////////////////////////////////////////
// Get the buffer size
//

unsigned char getBufferSize(void)
{
    return 8; // Standard CAN frame
}

///////////////////////////////////////////////////////////////////////////////
//  vscp_readNicknamePermanent
//

uint8_t vscp_readNicknamePermanent(void)
{
    return eeprom_read( VSCP_EEPROM_NICKNAME );
}

///////////////////////////////////////////////////////////////////////////////
//  vscp_writeNicknamePermanent
//

void vscp_writeNicknamePermanent(uint8_t nickname)
{
    eeprom_write( VSCP_EEPROM_NICKNAME, nickname );
}

///////////////////////////////////////////////////////////////////////////////
// vscp_getZone
//

uint8_t vscp_getZone(void)
{
    return eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_ZONE );
}

///////////////////////////////////////////////////////////////////////////////
// vscp_getSubzone
//

uint8_t vscp_getSubzone(void)
{
    return eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_SUBZONE );
}

///////////////////////////////////////////////////////////////////////////////
// doWork
//
// The actual work is done here.
//

void doWork(void)
{
    if ( VSCP_STATE_ACTIVE == vscp_node_state ) {
		;
    }
}

///////////////////////////////////////////////////////////////////////////////
// vscp_readAppReg
//

uint8_t vscp_readAppReg(uint8_t reg)
{    
    uint8_t rv;

    rv = 0x00; // default read
    
    // * * *  Page 0  * * *
    if ( 0 == vscp_page_select ) {

        // Zone
        if ( reg == REG0_BEIJING_ZONE ) {
            rv = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_ZONE );
        }            // SubZone
        else if ( reg == REG0_BEIJING_SUBZONE ) {
            rv = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_SUBZONE );
        }
        else if ( ( reg >= REG0_BEIJING_CH0_SUBZONE ) && 
                    ( reg <= REG0_BEIJING_CH9_SUBZONE ) ) {
            rv = eeprom_read( VSCP_EEPROM_END + 
                                REG0_BEIJING_CH0_SUBZONE + 
                                ( reg - REG0_BEIJING_CH0_SUBZONE ) );
        }
        else if ( reg == REG0_BEIJING_IO_DIRECTION_MSB ) {
            rv = ( TRISAbits.TRISA0 << 1 ) +
                    TRISAbits.TRISA1;
        }
        else if ( reg == REG0_BEIJING_IO_DIRECTION_LSB ) {
            rv = ( TRISAbits.TRISA2 << 7 ) +
                    ( TRISCbits.TRISC7 << 6 ) +
                    ( TRISCbits.TRISC6 << 5 ) +
                    ( TRISCbits.TRISC5 << 4 ) +
                    ( TRISCbits.TRISC4 << 3 ) +
                    ( TRISCbits.TRISC3 << 2 ) +
                    ( TRISBbits.TRISB1 << 1 ) +
                    TRISBbits.TRISB0;
        }
        else if ( ( reg >= REG0_BEIJING_CH0_STATUS ) &&
                ( reg <= REG0_BEIJING_CH9_STATUS ) ) {

            switch (reg) {

                    // Channel 0 Status
                case REG0_BEIJING_CH0_STATUS:
                    rv = CHANNEL0;
                    break;

                    // Channel 1 Status
                case REG0_BEIJING_CH1_STATUS:
                    rv = CHANNEL1;
                    break;

                    // Channel 2 Status
                case REG0_BEIJING_CH2_STATUS:
                    rv = CHANNEL2;
                    break;

                    // Channel 3 Status
                case REG0_BEIJING_CH3_STATUS:
                    rv = CHANNEL3;
                    break;

                    // Channel 4 Status
                case REG0_BEIJING_CH4_STATUS:
                    rv = CHANNEL4;
                    break;

                    // Channel 5 Status
                case REG0_BEIJING_CH5_STATUS:
                    rv = CHANNEL5;
                    break;

                    // Channel 6 Status
                case REG0_BEIJING_CH6_STATUS:
                    rv = CHANNEL6;
                    break;

                    // Channel 7 Status
                case REG0_BEIJING_CH7_STATUS:
                    rv = CHANNEL7;
                    break;

                    // Channel 8 Status
                case REG0_BEIJING_CH8_STATUS:
                    rv = CHANNEL8;
                    break;

                    // Channel 9 Status
                case REG0_BEIJING_CH9_STATUS:
                    rv = CHANNEL9;
                    break;
            }
        }
        // Read all other registers
        else if ( ( reg >= REG0_BEIJING_CH0_OUTPUT_CTRL ) &&
                ( reg < REG0_COUNT ) ) {
            rv = eeprom_read( VSCP_EEPROM_END + reg );
        }
        
    } // page 0
    
    // * * *  Page 1  * * *
    else if ( 1 == vscp_page_select ) {
        if ( reg < REG1_COUNT ) {
            rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );
        }
    }
    // * * *  Page 2  * * *
    else if ( 2 == vscp_page_select ) {
        if ( reg < ( REG_DESCION_MATRIX + 8*DESCION_MATRIX_ROWS ) ) {
            rv = eeprom_read( VSCP_EEPROM_END + 
                                REG_DESCION_MATRIX + 
                                REG0_COUNT + 
                                REG1_COUNT + 
                                reg );
        }
    }
    
    return rv;

}

///////////////////////////////////////////////////////////////////////////////
// vscp_writeAppReg
//

uint8_t vscp_writeAppReg( uint8_t reg, uint8_t val )
{
    uint8_t rv;
    BOOL bInfoEvent = FALSE;
    BOOL bOn = FALSE;

    rv = ~val; // error return
    
    // * * *  Page 0  * * *
    if ( 0 == vscp_page_select ) {

        // Zone
        if ( reg == REG0_BEIJING_ZONE ) {
            eeprom_write(VSCP_EEPROM_END + REG0_BEIJING_ZONE, val);
            rv = eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_ZONE);
        }
        else if ( reg == REG0_BEIJING_SUBZONE ) {
            // SubZone
            eeprom_write(VSCP_EEPROM_END + REG0_BEIJING_SUBZONE, val);
            rv = eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_SUBZONE);
        }
        // Channel sub zones
        else if ( ( reg >= REG0_BEIJING_CH0_SUBZONE ) && 
                       ( reg <= REG0_BEIJING_CH9_SUBZONE ) ) {
            eeprom_write( VSCP_EEPROM_END + 
                                REG0_BEIJING_CH0_SUBZONE + 
                                ( reg - REG0_BEIJING_CH0_SUBZONE ), val );
            rv = eeprom_read( VSCP_EEPROM_END + 
                                REG0_BEIJING_CH0_SUBZONE + 
                                ( reg - REG0_BEIJING_CH0_SUBZONE ) );
        }
        else if ( reg == REG0_BEIJING_IO_DIRECTION_MSB ) {
            
            eeprom_write( VSCP_EEPROM_END + 
                                    REG0_BEIJING_IO_DIRECTION_MSB,
                                    ( val & 0x03 ) );
            
            TRISAbits.TRISA0 = ( val & 0x02 ) ? 1 : 0;
            TRISAbits.TRISA1 = ( val & 0x01 ) ? 1 : 0;
            
            rv = ( TRISAbits.TRISA0 << 1 ) +
                    TRISAbits.TRISA1;
        }
        else if ( reg == REG0_BEIJING_IO_DIRECTION_LSB ) {
            
            eeprom_write( VSCP_EEPROM_END + 
                                    REG0_BEIJING_IO_DIRECTION_LSB,
                                    val);
            
            TRISAbits.TRISA2 = ( val & 0x80 ) ? 1 : 0;
            TRISCbits.TRISC7 = ( val & 0x40 ) ? 1 : 0;
            TRISCbits.TRISC6 = ( val & 0x20 ) ? 1 : 0;
            TRISCbits.TRISC5 = ( val & 0x10 ) ? 1 : 0;
            TRISCbits.TRISC4 = ( val & 0x08 ) ? 1 : 0;
            TRISCbits.TRISC3 = ( val & 0x04 ) ? 1 : 0;
            TRISBbits.TRISB1 = ( val & 0x02 ) ? 1 : 0;
            TRISBbits.TRISB0 = ( val & 0x01 ) ? 1 : 0;
            
            rv = ( TRISAbits.TRISA2 << 7 ) +
                    ( TRISCbits.TRISC7 << 6 ) +
                    ( TRISCbits.TRISC6 << 5 ) +
                    ( TRISCbits.TRISC5 << 4 ) +
                    ( TRISCbits.TRISC4 << 3 ) +
                    ( TRISCbits.TRISC3 << 2 ) +
                    ( TRISBbits.TRISB1 << 1 ) +
                    TRISBbits.TRISB0;
            
        }
        // Channel Status registers?
        else if ( (reg >= REG0_BEIJING_CH0_STATUS) && (reg <= REG0_BEIJING_CH9_STATUS) ) {

            // If enabled
            if ( eeprom_read( VSCP_EEPROM_END +
                                REG0_BEIJING_CH0_OUTPUT_CTRL + 
                                ( reg - REG0_BEIJING_CH0_STATUS ) ) &
                                OUTPUT_CTRL_ENABLED ) {

                switch ( reg ) {

                    // Channel 0 State
                    case REG0_BEIJING_CH0_STATUS:
                    
                        bInfoEvent = TRUE;

                        if ( 1 == val ) {
                            
                            CHANNEL0 = 1;
                            rv = CHANNEL0;
                            bOn = TRUE;

                            // If needed - Update protection timer
                            if ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL ) & OUTPUT_CTRL_PROTECTION ) {
                                channel_protection_timer[ 0 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH0_TIMING_PROTECT_MSB ) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH0_TIMING_PROTECT_LSB );
                            }

                        }
                        else if (0 == val) {
                            CHANNEL0 = 0;
                            rv = CHANNEL0;
                            bOn = FALSE;
                        }
                        break;

                    // Channel 1 State
                    case REG0_BEIJING_CH1_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {

                            CHANNEL1 = 1;
                            rv = CHANNEL1;
                            bOn = TRUE;

                            if (eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH1_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 1 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH1_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH1_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL1 = 0;
                            rv = CHANNEL1;
                            bOn = FALSE;
                        }
                        break;

                    // Channel 2 State
                    case REG0_BEIJING_CH2_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {

                            CHANNEL2 = 1;
                            rv = CHANNEL2;
                            bOn = TRUE;

                            if ( eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH2_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 2 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH2_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH2_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL2 = 0;
                            rv = CHANNEL2;
                            bOn = FALSE;

                        }
                        break;

                    // Channel 3 State
                    case REG0_BEIJING_CH3_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {

                            CHANNEL3 = 1;
                            rv = CHANNEL3;
                            bOn = TRUE;

                            if (eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH3_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 3 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH3_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH3_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL3 = 0;
                            rv = CHANNEL3;
                            bOn = FALSE;

                        }
                        break;

                    // Channel 4 State
                    case REG0_BEIJING_CH4_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {

                            CHANNEL4 = 1;
                            rv = CHANNEL4;
                            bOn = TRUE;

                            if (eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH4_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 4 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH4_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH4_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL4 = 0;
                            rv = CHANNEL4;
                            bOn = FALSE;

                        }
                        break;

                    // Channel 5 State
                    case REG0_BEIJING_CH5_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {

                            CHANNEL5 = 1;
                            rv = CHANNEL5;
                            bOn = TRUE;

                            if (eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH5_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 5 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH5_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH5_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL5 = 0;
                            rv = CHANNEL5;
                            bOn = FALSE;
                        }
                        break;

                    // Channel 6 State
                    case REG0_BEIJING_CH6_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {
                        
                            CHANNEL6 = 1;
                            rv = CHANNEL6;
                            bOn = TRUE;

                            if (eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH6_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 6 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH6_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH6_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL6 = 0;
                            rv = CHANNEL6;
                            bOn = FALSE;
                        }
                        break;

                    // Channel 7 State
                    case REG0_BEIJING_CH7_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {
                        
                            CHANNEL7 = 1;
                            rv = CHANNEL7;
                            bOn = TRUE;

                            if (eeprom_read(VSCP_EEPROM_END + REG0_BEIJING_CH7_OUTPUT_CTRL) & OUTPUT_CTRL_PROTECTION) {
                                channel_protection_timer[ 7 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH7_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH7_TIMING_PROTECT_LSB);
                            }

                        }
                        else if (0 == val) {
                            CHANNEL7 = 0;
                            rv = CHANNEL7;
                            bOn = FALSE;
                        }
                        break;
                    
                    // Channel 8 State
                    case REG0_BEIJING_CH8_STATUS:

                        bInfoEvent = TRUE;

                        if (1 == val) {
                        
                            CHANNEL8 = 1;
                            rv = CHANNEL8;
                            bOn = TRUE;

                            if ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH8_OUTPUT_CTRL ) & OUTPUT_CTRL_PROTECTION ) {
                                channel_protection_timer[ 8 ] =
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH8_TIMING_PROTECT_MSB) * 256 +
                                    eeprom_read(VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH8_TIMING_PROTECT_LSB);
                            }

                        }
                        else if ( 0 == val ) {
                            CHANNEL8 = 0;
                            rv = CHANNEL8;
                            bOn = FALSE;
                        }
                        break;
    
                    // Channel 9 State
                    case REG0_BEIJING_CH9_STATUS:

                        bInfoEvent = TRUE;

                        if ( 1 == val ) {
                        
                            CHANNEL9 = 1;
                            rv = CHANNEL9;
                            bOn = TRUE;

                            if ( eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH9_OUTPUT_CTRL ) & OUTPUT_CTRL_PROTECTION ) {
                                channel_protection_timer[ 9 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH9_TIMING_PROTECT_MSB ) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                REG1_BEIJING_CH9_TIMING_PROTECT_LSB );
                            }

                        }
                        else if ( 0 == val ) {
                            CHANNEL9 = 0;
                            rv = CHANNEL9;
                            bOn = FALSE;
                        }
                        break;

                }
            }
        }

        // Output control registers
        else if ( (reg >= REG0_BEIJING_CH0_OUTPUT_CTRL ) &&
                    (reg <= REG0_BEIJING_CH9_OUTPUT_CTRL) ) {

            switch( reg ) {

                case REG0_BEIJING_CH0_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {

                        CHANNEL0 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0000000001; // Enable pulse output
                        channel_pulse_timer[ 0 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH0_TIMING_PULSE_MSB ) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH0_TIMING_PULSE_MSB );
                    }
                    else {
                        channel_pulse_flags &= 0b1111111110; // Disable pulse output
                        CHANNEL0 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 0 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PROTECT_LSB );
                    }
                    break;

                case REG0_BEIJING_CH1_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_ALARM ) {

                        CHANNEL1 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0000000010; // Enable pulse output
                        channel_pulse_timer[ 1 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH0_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH0_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b1111111101; // Disable pulse output
                        CHANNEL1 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 1 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PROTECT_LSB );
                    }
                    break;

                case REG0_BEIJING_CH2_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {

                        CHANNEL2 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0000000100; // Enable pulse output
                        channel_pulse_timer[ 2 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH2_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH2_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b1111111011; // Disable pulse output
                        CHANNEL2 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 2 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH2_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH2_TIMING_PROTECT_LSB );
                    }
                    break;

                case REG0_BEIJING_CH3_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {

                        CHANNEL3 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0000001000; // Enable pulse output
                        channel_pulse_timer[ 3 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH3_TIMING_PULSE_MSB ) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH3_TIMING_PULSE_LSB );
                    }
                    else {
                        channel_pulse_flags &= 0b1111110111; // Disable pulse output
                        CHANNEL3 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 3 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH3_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH3_TIMING_PROTECT_LSB );
                    }
                    break;

                case REG0_BEIJING_CH4_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PROTECTION ) {

                        CHANNEL4 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0000010000; // Enable pulse output
                        channel_pulse_timer[ 4 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH4_TIMING_PULSE_MSB ) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH4_TIMING_PULSE_LSB );
                    }
                    else {
                        channel_pulse_flags &= 0b1111101111; // Disable pulse output
                        CHANNEL4 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 4 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH4_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH4_TIMING_PROTECT_LSB );
                    }
                    break;

                case REG0_BEIJING_CH5_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {

                        CHANNEL5 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0000100000; // Enable pulse output
                        channel_pulse_timer[ 5 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH5_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH5_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b1111011111; // Disable pulse output
                        CHANNEL5 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 5 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH5_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH5_TIMING_PROTECT_LSB );
                    }
                    break;

                case REG0_BEIJING_CH6_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {
                    
                        CHANNEL6 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0001000000; // Enable pulse output
                        channel_pulse_timer[ 6 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH6_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH6_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b1110111111; // Disable pulse output
                        CHANNEL6 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 6 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH6_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH6_TIMING_PROTECT_LSB );
                    }
                    break;
                
                case REG0_BEIJING_CH7_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {
                    
                        CHANNEL7 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0010000000; // Enable pulse output
                        channel_pulse_timer[ 7 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH7_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH7_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b1101111111; // Disable pulse output
                        CHANNEL7 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 7 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH7_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH7_TIMING_PROTECT_LSB );
                    }
                    break;    

                case REG0_BEIJING_CH8_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {
                    
                        CHANNEL8 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b0100000000; // Enable pulse output
                        channel_pulse_timer[ 8 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH8_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH8_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b1011111111; // Disable pulse output
                        CHANNEL8 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 8 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH8_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH8_TIMING_PROTECT_LSB );
                    }
                    break;
                
                case REG0_BEIJING_CH9_OUTPUT_CTRL:

                    eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
                    rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

                    if ( val & OUTPUT_CTRL_PULSE ) {
                    
                        CHANNEL9 = 0; // Start out at a known state
                        bOn = FALSE;

                        channel_pulse_flags |= 0b1000000000; // Enable pulse output
                        channel_pulse_timer[ 9 ] =
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH9_TIMING_PULSE_MSB) * 256 +
                                    eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                                    REG1_BEIJING_CH9_TIMING_PULSE_LSB);
                    }
                    else {
                        channel_pulse_flags &= 0b0111111111; // Disable pulse output
                        CHANNEL9 = 0; // End with off
                        bOn = FALSE;
                    }

                    if ( val & OUTPUT_CTRL_PROTECTION ) {
                        channel_protection_timer[ 9 ] =
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH9_TIMING_PROTECT_MSB ) * 256 +
                            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH9_TIMING_PROTECT_LSB );
                    }
                    break;        
                
            }
            
        }
        else if ( ( reg >= REG0_BEIJING_CH0_INPUT_CTRL ) && 
                    ( reg <= REG0_BEIJING_STREAM_TIMING ) ) {
            eeprom_write( VSCP_EEPROM_END + reg, val);
            rv = eeprom_read( VSCP_EEPROM_END + reg );
        }
        
    } // page 0
    
    // * * *  Page 1  * * *
    else if ( 1 == vscp_page_select ) {
    
        // Pulse time registers
        if ( (reg >= REG1_BEIJING_CH0_TIMING_PULSE_MSB ) &&
                    (reg <= REG1_BEIJING_CH9_TIMING_PULSE_LSB ) ) {

            // Write to EEPROM
            eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
            rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );

            // We let the default take care of the MSB's
            // and give some special treatment to the LSB's
            switch ( reg ) {

                case REG1_BEIJING_CH0_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 0 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH0_TIMING_PULSE_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH0_TIMING_PULSE_LSB );
                    break;

                case REG1_BEIJING_CH1_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 1 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH1_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH1_TIMING_PULSE_LSB);
                    break;

                case REG1_BEIJING_CH2_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 2 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH2_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH2_TIMING_PULSE_LSB);
                    break;

                case REG1_BEIJING_CH3_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 3 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH3_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH3_TIMING_PULSE_LSB);
                    break;

                case REG1_BEIJING_CH4_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 4 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH4_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH4_TIMING_PULSE_LSB);
                    break;

                case REG1_BEIJING_CH5_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 5 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH5_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH5_TIMING_PULSE_LSB);
                    break;

                case REG1_BEIJING_CH6_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 6 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH6_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH6_TIMING_PULSE_LSB);
                    break;

                case REG1_BEIJING_CH7_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 7 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH7_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH7_TIMING_PULSE_LSB);
                    break;
                
                case REG1_BEIJING_CH8_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 8 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH8_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH8_TIMING_PULSE_LSB);
                    break;    
                
                case REG1_BEIJING_CH9_TIMING_PULSE_LSB:
                    channel_pulse_timer[ 9 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH9_TIMING_PULSE_MSB) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                    REG1_BEIJING_CH9_TIMING_PULSE_LSB);
                    break;    

                default:
                    break;
            }

        }
        // Relay protection time registers
        else if ( ( reg >= REG1_BEIJING_CH0_TIMING_PROTECT_MSB ) &&
                ( reg <= REG1_BEIJING_CH9_TIMING_PROTECT_LSB ) ) {

            // Write to EEPROM
            eeprom_write( VSCP_EEPROM_END + REG0_COUNT + reg, val );
            rv = eeprom_read( VSCP_EEPROM_END + REG0_COUNT + reg );
        
            switch ( reg ) {

                case REG1_BEIJING_CH0_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 0 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH0_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH1_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 1 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH1_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH1_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH2_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 2 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH2_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH2_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH3_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 3 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH3_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH3_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH4_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 4 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH4_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH4_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH5_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 5 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH5_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH5_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH6_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 6 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH6_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH6_TIMING_PROTECT_LSB );
                    break;

                case REG1_BEIJING_CH7_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 7 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH7_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH7_TIMING_PROTECT_LSB );
                    break;
                
                case REG1_BEIJING_CH8_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 8 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH8_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH8_TIMING_PROTECT_LSB );
                    break; 
                
                case REG1_BEIJING_CH9_TIMING_PROTECT_LSB:
                    channel_protection_timer[ 9 ] =
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH9_TIMING_PROTECT_MSB ) * 256 +
                        eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                                        REG1_BEIJING_CH9_TIMING_PROTECT_LSB );
                    break;    

                default:
                    break;
            }

        }
        
    } // page 1
    
    // * * *  Page 2  * * *
    else if ( 2 == vscp_page_select ) {
    
        if ( reg < (REG_DESCION_MATRIX + DESCION_MATRIX_ROWS * 8) ) {
            eeprom_write( VSCP_EEPROM_END + 
                            REG_DESCION_MATRIX +
                            REG0_COUNT + 
                            REG1_COUNT + 
                            reg, val );
            calculateSetFilterMask();  // Calculate new hardware filter
            rv = eeprom_read( VSCP_EEPROM_END + 
                            REG_DESCION_MATRIX + 
                            REG0_COUNT + 
                            REG1_COUNT + 
                            reg );
        }
        
    } // page 2
        
    // --------------------------------------------------------------------------

    // Send information Event
    // if enabled in the registers
    if ( bInfoEvent ) {

        unsigned char val;
        val = eeprom_read( VSCP_EEPROM_END + 
                            REG0_BEIJING_CH0_STATUS + 
                            ( reg - REG0_BEIJING_CH0_STATUS ) );

        if ( bOn ) {

            if ( val & OUTPUT_CTRL_ONEVENT ) {
                SendInformationEvent( reg - REG0_BEIJING_CH0_STATUS,
                                        VSCP_CLASS1_INFORMATION,
                                        VSCP_TYPE_INFORMATION_ON );
            }

        }
        else {

            if ( val & OUTPUT_CTRL_OFFEVENT ) {
                SendInformationEvent( reg - REG0_BEIJING_CH0_STATUS,
                                        VSCP_CLASS1_INFORMATION,
                                        VSCP_TYPE_INFORMATION_OFF );
            }
            
        }
    }

    return rv;

}

///////////////////////////////////////////////////////////////////////////////
// Send Decision Matrix Information
//

void sendDMatrixInfo(void)
{
    vscp_omsg.priority = VSCP_PRIORITY_MEDIUM;
    vscp_omsg.flags = VSCP_VALID_MSG + 2;
    vscp_omsg.vscp_class = VSCP_CLASS1_PROTOCOL;
    vscp_omsg.vscp_type = VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE;

    vscp_omsg.data[ 0 ] = DESCION_MATRIX_ROWS;
    vscp_omsg.data[ 1 ] = REG_DESCION_MATRIX;

    vscp_sendEvent(); // Send data
}


///////////////////////////////////////////////////////////////////////////////
// SendInformationEvent
//

void SendInformationEvent( unsigned char idx,
                            unsigned char eventClass,
                            unsigned char eventTypeId )
{
    uint8_t data[3];

    data[ 0 ] = idx; // Register
    data[ 1 ] = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_ZONE );
    data[ 2 ] = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_SUBZONE + idx );
    sendVSCPFrame( eventClass,
                    eventTypeId,
                    vscp_nickname,
                    VSCP_PRIORITY_MEDIUM,
                    3,
                    data );
}

///////////////////////////////////////////////////////////////////////////////
// Do decision Matrix handling
// 
// The routine expects vscp_imsg to contain a valid incoming event
//

void doDM(void)
{
    unsigned char i;
    unsigned char dmflags;
    unsigned short class_filter;
    unsigned short class_mask;
    unsigned char type_filter;
    unsigned char type_mask;

    // Don't deal with the protocol functionality
    if ( VSCP_CLASS1_PROTOCOL == vscp_imsg.vscp_class ) return;

    for (i = 0; i<DESCION_MATRIX_ROWS; i++) {

        // Get DM flags for this row
        dmflags = eeprom_read( VSCP_EEPROM_END + 
                                    REG0_COUNT + 
                                    REG1_COUNT + 
                                    REG_DESCION_MATRIX + 
                                    VSCP_DM_POS_FLAGS + 
                                    (8 * i) );

        // Is the DM row enabled?
        if ( dmflags & VSCP_DM_FLAG_ENABLED ) {

            // Should the originating id be checked and if so is it the same?
            if ( ( dmflags & VSCP_DM_FLAG_CHECK_OADDR ) &&
                    ( vscp_imsg.oaddr != eeprom_read( VSCP_EEPROM_END + REG0_COUNT + REG1_COUNT + REG_DESCION_MATRIX + (8 * i) ) ) ) {
                continue;
            }

            // Check if zone should match and if so if it match
            if ( dmflags & VSCP_DM_FLAG_CHECK_ZONE ) {
                if ( 255 != vscp_imsg.data[ 1 ] ) {
                    if ( vscp_imsg.data[ 1 ] != eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_ZONE ) ) {
                        continue;
                    }
                }
            }

            // Check if sub zone should match and if so if it match
            if ( dmflags & VSCP_DM_FLAG_CHECK_SUBZONE ) {
                if ( 255 != vscp_imsg.data[ 2 ] ) {
                    if ( vscp_imsg.data[ 2 ] != eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_SUBZONE ) ) {
                        continue;
                    }
                }
            }
            
            class_filter = ( ( dmflags & VSCP_DM_FLAG_CLASS_FILTER) << 8 ) +
                    eeprom_read( VSCP_EEPROM_END +
                                    REG0_COUNT + REG1_COUNT + 
                                    REG_DESCION_MATRIX +
                                    (8 * i) +
                                    VSCP_DM_POS_CLASSFILTER);
            
            class_mask = ( ( dmflags & VSCP_DM_FLAG_CLASS_MASK ) << 7 ) +
                    eeprom_read( VSCP_EEPROM_END +
                                    REG0_COUNT + REG1_COUNT + 
                                    REG_DESCION_MATRIX +
                                    (8 * i) +
                                    VSCP_DM_POS_CLASSMASK);
            
            type_filter = eeprom_read( VSCP_EEPROM_END +
                                        REG0_COUNT + REG1_COUNT + 
                                        REG_DESCION_MATRIX +
                                        (8 * i) +
                                        VSCP_DM_POS_TYPEFILTER);
            
            type_mask = eeprom_read( VSCP_EEPROM_END +
                                        REG0_COUNT + REG1_COUNT + 
                                        REG_DESCION_MATRIX +
                                        (8 * i) +
                                        VSCP_DM_POS_TYPEMASK);
                           
            if ( !( ( class_filter ^ vscp_imsg.vscp_class ) & class_mask ) &&
                    !( ( type_filter ^ vscp_imsg.vscp_type ) & type_mask ) ) {

                // OK Trigger this action
                switch ( eeprom_read( VSCP_EEPROM_END + 
                                        REG0_COUNT + REG1_COUNT + 
                                        REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTION ) ) {

                    case BEIJING_ACTION_NOOP:
                        break;
                        
                    case BEIJING_ACTION_SET: // Enable Channels in arg. bit arry
                        doActionOn( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                            REG0_COUNT + REG1_COUNT + 
                                                            REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;

                    case BEIJING_ACTION_CLR: // Disable Channels in arg. bitarry
                        doActionOff( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                            REG0_COUNT + REG1_COUNT + 
                                                            REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;

                    case BEIJING_ACTION_PULSEON: // Pulse Channels in arg. bitarry, zone, subzone
                        doActionPulseOn( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                                REG0_COUNT + REG1_COUNT + 
                                                                REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;
                        
                    case BEIJING_ACTION_PULSEOFF: // Pulse Channels in arg. bitarry, zone, subzone
                        doActionPulseOff( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                                REG0_COUNT + REG1_COUNT + 
                                                                REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;    

                    case BEIJING_ACTION_STATUS: // Send status for all Channels
                        doActionStatus( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                                REG0_COUNT + REG1_COUNT + 
                                                                REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;
                        
                    case BEIJING_ACTION_STATUSALL: // Send status for all Channels
                        doActionStatusAll( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                                REG0_COUNT + REG1_COUNT + 
                                                                REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;    

                    case BEIJING_ACTION_TOGGLE: // Toggle Channels
                        doActionToggle( dmflags, eeprom_read( VSCP_EEPROM_END + 
                                                                REG0_COUNT + REG1_COUNT + 
                                                                REG_DESCION_MATRIX + (8 * i) + VSCP_DM_POS_ACTIONPARAM ) );
                        break;

                } // case
                
            } // Filter/mask
            
        } // Row enabled
        
    } // for each row
    
}

///////////////////////////////////////////////////////////////////////////////
// doActionOn
// 

void doActionOn( unsigned char dmflags, unsigned char arg )
{
    unsigned char ctrlreg;
    BOOL bEvent = FALSE;
            
    // Check for a valid argument
    if ( arg > 9 ) return;

    ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + arg );

    // Do nothing if disabled
    if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) return;

    switch ( arg ) {

        case 0:
            if ( 0 == CHANNEL0 ) bEvent = TRUE;
            CHANNEL0 = 1; 
            break;

        case 1:
            if ( 0 == CHANNEL1 ) bEvent = TRUE;
            CHANNEL1 = 1;
            break;

        case 2:
            if ( 0 == CHANNEL2 ) bEvent = TRUE;
            CHANNEL2 = 1;
            break;

        case 3:
            if ( 0 == CHANNEL3 ) bEvent = TRUE;
            CHANNEL3 = 1;
            break;

        case 4:
            if ( 0 == CHANNEL4 ) bEvent = TRUE;
            CHANNEL4 = 1;
            break;

        case 5:
            if ( 0 == CHANNEL5 ) bEvent = TRUE;
            CHANNEL5 = 1;
            break;

        case 6:
            if ( 0 == CHANNEL6 ) bEvent = TRUE;
            CHANNEL6 = 1;
            break;
                
        case 7:
            if ( 0 == CHANNEL7 ) bEvent = TRUE;
            CHANNEL7 = 1;
            break;
                
        case 8:
            if ( 0 == CHANNEL8 ) bEvent = TRUE;
            CHANNEL8 = 1;
            break;    
                
        case 9:
            if ( 0 == CHANNEL9 ) bEvent = TRUE;
            CHANNEL7 = 1;
            break;    

    }
    
    if ( ctrlreg & OUTPUT_CTRL_PROTECTION ) {
        channel_protection_timer[ arg ] =
                eeprom_read( VSCP_EEPROM_END + 
                                REG0_COUNT +
                                REG1_BEIJING_CH0_TIMING_PROTECT_MSB + 
                                2 * arg ) * 256 +
                eeprom_read( VSCP_EEPROM_END + 
                                REG0_COUNT +
                                REG1_BEIJING_CH0_TIMING_PROTECT_LSB  + 
                                2 * arg );
    }

    // Should off event be sent?
    if ( bEvent && ( ctrlreg & OUTPUT_CTRL_ONEVENT ) ) {
        SendInformationEvent( arg, VSCP_CLASS1_INFORMATION, VSCP_TYPE_INFORMATION_ON );
    }

}

///////////////////////////////////////////////////////////////////////////////
// doActionOff
// 

void doActionOff( unsigned char dmflags, unsigned char arg )
{
    unsigned char ctrlreg;
    BOOL bEvent = FALSE;
    
    // Check for a valid argument
    if ( arg > 9 ) return;

    ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + arg );

    // Do nothing if disabled
    if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) return;

    switch ( arg ) {

        case 0:
            if ( 1 == CHANNEL0 ) bEvent = TRUE;
            CHANNEL0 = 0; 
            break;

        case 1:
            if ( 1 == CHANNEL1 ) bEvent = TRUE;            
            CHANNEL1 = 0;
            break;

        case 2:
            if ( 1 == CHANNEL2 ) bEvent = TRUE;
            CHANNEL2 = 0;
            break;

        case 3:
            if ( 1 == CHANNEL3 ) bEvent = TRUE;
            CHANNEL3 = 0;
            break;

        case 4:
            if ( 1 == CHANNEL4 ) bEvent = TRUE;
            CHANNEL4 = 0;
            break;

        case 5:
            if ( 1 == CHANNEL5 ) bEvent = TRUE;
            CHANNEL5 = 0;
            break;

        case 6:
            if ( 1 == CHANNEL6 ) bEvent = TRUE;
            CHANNEL6 = 0;
            break;
                
        case 7:
            if ( 1 == CHANNEL7 ) bEvent = TRUE;
            CHANNEL7 = 0;
            break;
                
        case 8:
            if ( 1 == CHANNEL8 ) bEvent = TRUE;
            CHANNEL8 = 0;
            break;    
                
        case 9:
            if ( 1 == CHANNEL9 ) bEvent = TRUE;
            CHANNEL7 = 0;
            break;    

    }
    
    // Should off event be sent?
    if ( bEvent && ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) ) {
        SendInformationEvent( arg, VSCP_CLASS1_INFORMATION, VSCP_TYPE_INFORMATION_OFF );
    }
}

///////////////////////////////////////////////////////////////////////////////
// doActionPulseOn
// 

void doActionPulseOn(unsigned char dmflags, unsigned char arg)
{
    unsigned char ctrlreg;
    BOOL bEvent = FALSE;
    
    // Check for a valid argument
    if ( arg > 9 ) return;

    ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + arg );

    // Do nothing if disabled
    if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) return;

    switch ( arg ) {

        case 0:
            if ( 1 == CHANNEL0 ) bEvent = TRUE;
            CHANNEL0 = 0; // Start out at a known state
            break;

        case 1:
            if ( 1 == CHANNEL1 ) bEvent = TRUE;
            CHANNEL1 = 0;
            break;

        case 2:
            if ( 1 == CHANNEL2 ) bEvent = TRUE;
            CHANNEL2 = 0;
            break;

        case 3:
            if ( 1 == CHANNEL3 ) bEvent = TRUE;
            CHANNEL3 = 0;
            break;

        case 4:
            if ( 1 == CHANNEL4 ) bEvent = TRUE;
            CHANNEL4 = 0;
            break;

        case 5:
            if ( 1 == CHANNEL5 ) bEvent = TRUE;
            CHANNEL5 = 0;
            break;

        case 6:
            if ( 1 == CHANNEL6 ) bEvent = TRUE;
            CHANNEL6 = 0;
            break;
                
        case 7:
            if ( 1 == CHANNEL7 ) bEvent = TRUE;
            CHANNEL7 = 0;
            break;
                
        case 8:
            if ( 1 == CHANNEL8 ) bEvent = TRUE;
            CHANNEL8 = 0;
            break;    
                
        case 9:
            if ( 1 == CHANNEL9 ) bEvent = TRUE;
            CHANNEL7 = 0;
            break;    

    }
    
    // Should off event be sent?
    if ( bEvent && ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) ) {
        SendInformationEvent( arg, VSCP_CLASS1_INFORMATION, VSCP_TYPE_INFORMATION_OFF );
    }
    
    channel_pulse_flags |= ( 1 << arg ); // Enable pulse output
    channel_pulse_timer[ 0 ] =
            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                            REG1_BEIJING_CH0_TIMING_PULSE_MSB + arg ) * 256 +
            eeprom_read( VSCP_EEPROM_END + REG0_COUNT +
                            REG1_BEIJING_CH0_TIMING_PULSE_LSB + arg );
}

///////////////////////////////////////////////////////////////////////////////
// doActionPulseOff
// 

void doActionPulseOff(unsigned char dmflags, unsigned char arg)
{
    unsigned char ctrlreg;
    BOOL bEvent = FALSE;
    
    // Check for a valid argument
    if ( arg > 9 ) return;

    ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + arg );

    // Do nothing if disabled
    if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) return;

    switch ( arg ) {

        case 0:
            if ( 1 == CHANNEL0 ) bEvent = TRUE;
            CHANNEL0 = 0; // Start out at a known state
            break;

        case 1:
            if ( 1 == CHANNEL1 ) bEvent = TRUE;
            CHANNEL1 = 0;
            break;

        case 2:
            if ( 1 == CHANNEL2 ) bEvent = TRUE;
            CHANNEL2 = 0;
            break;

        case 3:
            if ( 1 == CHANNEL3 ) bEvent = TRUE;
            CHANNEL3 = 0;
            break;

        case 4:
            if ( 1 == CHANNEL4 ) bEvent = TRUE;
            CHANNEL4 = 0;
            break;

        case 5:
            if ( 1 == CHANNEL5 ) bEvent = TRUE;
            CHANNEL5 = 0;
            break;

        case 6:
            if ( 1 == CHANNEL6 ) bEvent = TRUE;
            CHANNEL6 = 0;
            break;
                
        case 7:
            if ( 1 == CHANNEL7 ) bEvent = TRUE;
            CHANNEL7 = 0;
            break;
                
        case 8:
            if ( 1 == CHANNEL8 ) bEvent = TRUE;
            CHANNEL8 = 0;
            break;    
                
        case 9:
            if ( 1 == CHANNEL9 ) bEvent = TRUE;
            CHANNEL7 = 0;
            break;    

    }
    
    // Should off event be sent?
    if ( bEvent && ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) ) {
        SendInformationEvent( arg, VSCP_CLASS1_INFORMATION, VSCP_TYPE_INFORMATION_OFF );
    }
    
    channel_pulse_flags &= ~( 1 << arg ); // Disable pulse output
}


///////////////////////////////////////////////////////////////////////////////
// doActionToggle
// 

void doActionToggle( unsigned char dmflags, unsigned char arg )
{
    unsigned char ctrlreg;
    BOOL bOn = FALSE;
    
    // Check for a valid argument
    if ( arg > 9 ) return;

    ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + arg );

    // Do nothing if disabled
    if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) return;

    switch ( arg ) {

        case 0:
            if ( CHANNEL0 ) {
                CHANNEL0 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL0 = 1;
                bOn = TRUE;
            }
            break;

        case 1:
            if ( CHANNEL1 ) {
                CHANNEL1 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL1 = 1;
                bOn = TRUE;
            }
            break;

        case 2:
            if ( CHANNEL2 ) {
                CHANNEL2 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL2 = 1;
                bOn = TRUE;
            }
                break;

        case 3:
            if ( CHANNEL3 ) {
                CHANNEL3 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL3 = 1;
                bOn = TRUE;
            }
            break;

        case 4:
            if ( CHANNEL4 ) {
                CHANNEL4 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL4 = 1;
                bOn = TRUE;
            }
            break;

        case 5:
            if ( CHANNEL5 ) {
                CHANNEL5 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL5 = 1;
                bOn = TRUE;
            }
            break;

        case 6:
            if ( CHANNEL6 ) {
                CHANNEL6 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL6 = 1;
                bOn = TRUE;
            }
            break;
                
        case 7:
            if ( CHANNEL7 ) {
                CHANNEL7 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL7 = 1;
                bOn = TRUE;
            }
            break; 
                
        case 8:
            if ( CHANNEL8 ) {
                CHANNEL8 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL8 = 1;
                bOn = TRUE;
            }
            break;    

        case 9:
            if ( CHANNEL9 ) {
                CHANNEL9 = 0;
                bOn = FALSE;
            } 
            else {
                CHANNEL9 = 1;
                bOn = TRUE;
            }
            break;    
    }

    if ( bOn ) {

        // Should on event be sent?
        if ( ctrlreg & OUTPUT_CTRL_ONEVENT ) {
            SendInformationEvent( arg, 
                                    VSCP_CLASS1_INFORMATION, 
                                    VSCP_TYPE_INFORMATION_ON );
        }

    }
    else {

        // Should off event be sent?
        if ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) {
            SendInformationEvent( arg, 
                                    VSCP_CLASS1_INFORMATION, 
                                    VSCP_TYPE_INFORMATION_OFF );
        }

    }

}


///////////////////////////////////////////////////////////////////////////////
// doActionStatus
// 

void doActionStatus(unsigned char dmflags, unsigned char arg)
{
    unsigned char ctrlreg;
    BOOL bOn = FALSE;
    
    // Check for a valid argument
    if ( arg > 9 ) return;

    ctrlreg = eeprom_read( VSCP_EEPROM_END + REG0_BEIJING_CH0_OUTPUT_CTRL + arg );

    // Do nothing if disabled
    if ( !( ctrlreg & OUTPUT_CTRL_ENABLED ) ) return;

    switch ( arg ) {

        case 0:
            bOn = CHANNEL0;
            break;

        case 1:
            bOn = CHANNEL1;
            break;

        case 2:
            bOn = CHANNEL2;
            break;

        case 3:
            bOn = CHANNEL3;
            break;

        case 4:
            bOn = CHANNEL4;
            break;

        case 5:
            bOn = CHANNEL5;
            break;

        case 6:
            bOn = CHANNEL6;
            break;
                
        case 7:
            bOn = CHANNEL7;
            break;    
                
        case 8:
            bOn = CHANNEL8;
            break;    

        case 9:
            bOn = CHANNEL9;
            break;    
    
    }

    if ( bOn ) {

        // Should off event be sent?
        if ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) {
            SendInformationEvent( arg, 
                                    VSCP_CLASS1_INFORMATION, 
                                    VSCP_TYPE_INFORMATION_OFF );
        }

    }
    else {
        // Should off event be sent?
        if ( ctrlreg & OUTPUT_CTRL_OFFEVENT ) {
            SendInformationEvent( arg, 
                                    VSCP_CLASS1_INFORMATION, 
                                    VSCP_TYPE_INFORMATION_OFF );
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// doActionStatusAll
// 

void doActionStatusAll(unsigned char dmflags, unsigned char arg)
{
    uint8_t i;
    
    for ( i=0; i<9; i++ ) {
        doActionStatus( dmflags, i );
    }
  
}


///////////////////////////////////////////////////////////////////////////////
//                        VSCP Required Methods
//////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// Get Major version number for this hardware module
//

unsigned char vscp_getMajorVersion()
{
    return FIRMWARE_MAJOR_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
// Get Minor version number for this hardware module
//

unsigned char vscp_getMinorVersion()
{
    return FIRMWARE_MINOR_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
// Get Subminor version number for this hardware module
//

unsigned char vscp_getSubMinorVersion()
{
    return FIRMWARE_SUB_MINOR_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
// getVSCP_GUID
//
// Get GUID from EEPROM
//

uint8_t vscp_getGUID(uint8_t idx)
{
    return eeprom_read( VSCP_EEPROM_REG_GUID + idx );
}


///////////////////////////////////////////////////////////////////////////////
// getDeviceURL
//
// Get device URL from EEPROM
//

uint8_t vscp_getMDF_URL(uint8_t idx)
{
    return vscp_deviceURL[ idx ];
}

///////////////////////////////////////////////////////////////////////////////
// Get Manufacturer id and subid from EEPROM
//

uint8_t vscp_getUserID(uint8_t idx)
{
    return eeprom_read( VSCP_EEPROM_REG_USERID + idx );
}

///////////////////////////////////////////////////////////////////////////////
//  setVSCPUserID
//

void vscp_setUserID(uint8_t idx, uint8_t data)
{
    eeprom_write( idx + VSCP_EEPROM_REG_USERID, data );
}

///////////////////////////////////////////////////////////////////////////////
// getVSCPManufacturerId
// 
// Get Manufacturer id and subid from EEPROM
//

uint8_t vscp_getManufacturerId(uint8_t idx)
{
    return eeprom_read( VSCP_EEPROM_REG_MANUFACTUR_ID0 + idx );
}

///////////////////////////////////////////////////////////////////////////////
// Get the bootloader algorithm code
//

uint8_t vscp_getBootLoaderAlgorithm(void)
{
    return VSCP_BOOTLOADER_PIC1;
}

///////////////////////////////////////////////////////////////////////////////
// Get the buffer size
//

uint8_t vscp_getBufferSize(void)
{
    return 8; // Standard CAN frame
}


///////////////////////////////////////////////////////////////////////////////
//  getNickname
//

uint8_t vscp_getNickname(void)
{
    return eeprom_read(VSCP_EEPROM_NICKNAME);
}

///////////////////////////////////////////////////////////////////////////////
//  setNickname
//

void vscp_setNickname(uint8_t nickname)
{
    eeprom_write(VSCP_EEPROM_NICKNAME, nickname);
}

///////////////////////////////////////////////////////////////////////////////
//  getSegmentCRC
//

uint8_t vscp_getSegmentCRC(void)
{
    return eeprom_read( VSCP_EEPROM_SEGMENT_CRC );
}

///////////////////////////////////////////////////////////////////////////////
//  setSegmentCRC
//

void vscp_setSegmentCRC(uint8_t crc)
{
    eeprom_write( VSCP_EEPROM_SEGMENT_CRC, crc );
}

///////////////////////////////////////////////////////////////////////////////
//  setVSCPControlByte
//

void vscp_setControlByte(uint8_t ctrl)
{
    eeprom_write(VSCP_EEPROM_CONTROL, ctrl);
}


///////////////////////////////////////////////////////////////////////////////
//  getVSCPControlByte
//

uint8_t vscp_getControlByte(void)
{
    return eeprom_read(VSCP_EEPROM_CONTROL);
}

///////////////////////////////////////////////////////////////////////////////
//  vscp_getEmbeddedMdfInfo
//

void vscp_getEmbeddedMdfInfo(void)
{
    // No embedded DM so we respond with info about that

    vscp_omsg.priority = VSCP_PRIORITY_NORMAL;
    vscp_omsg.flags = VSCP_VALID_MSG + 3;
    vscp_omsg.vscp_class = VSCP_CLASS1_PROTOCOL;
    vscp_omsg.vscp_type = VSCP_TYPE_PROTOCOL_RW_RESPONSE;

    vscp_omsg.data[ 0 ] = 0;
    vscp_omsg.data[ 1 ] = 0;
    vscp_omsg.data[ 2 ] = 0;

    // send the event
    vscp_sendEvent();
}

/*
///////////////////////////////////////////////////////////////////////////////
// vscp_getZone
//

uint8_t vscp_getZone( void )
{
        return eeprom_read( VSCP_EEPROM_END + EEPROM_ZONE );
}

///////////////////////////////////////////////////////////////////////////////
// vscp_getSubzone
//

uint8_t vscp_getSubzone( void )
{
        return eeprom_read( VSCP_EEPROM_END + EEPROM_SUBZONE );
}
 */

///////////////////////////////////////////////////////////////////////////////
// vscp_goBootloaderMode
//

void vscp_goBootloaderMode( uint8_t algorithm )
{
    if ( VSCP_BOOTLOADER_PIC1 != algorithm  ) return;

    // OK, We should enter boot loader mode
    // 	First, activate bootloader mode
    eeprom_write(VSCP_EEPROM_BOOTLOADER_FLAG, VSCP_BOOT_FLAG);

    // Reset processor
    Reset();
}

///////////////////////////////////////////////////////////////////////////////
//  vscp_getMatrixInfo
//

void vscp_getMatrixInfo(char *pData)
{
    uint8_t i;

    vscp_omsg.data[ 0 ] = 7; // Matrix is seven rows
    vscp_omsg.data[ 1 ] = 72; // Matrix start offset

    // The rest set to zero no paging
    for ( i = 2; i < 8; i++ ) {
        vscp_omsg.data[ i ] = 0;
    }

}

///////////////////////////////////////////////////////////////////////////////
// vscp_getFamilyCode
//

uint32_t vscp_getFamilyCode() {
    return 0L;
}


///////////////////////////////////////////////////////////////////////////////
// vscp_getFamilyType
//

uint32_t vscp_getFamilyType() {
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// vscp_restoreDefaults
//

void vscp_restoreDefaults() {
    init_app_eeprom();
    init_app_ram();
}


///////////////////////////////////////////////////////////////////////////////
// vscp_getRegisterPagesUsed
//

uint8_t vscp_getRegisterPagesUsed( void )
{
    return 3; // One pae used
}

///////////////////////////////////////////////////////////////////////////////
// sendVSCPFrame
//

int8_t sendVSCPFrame( uint16_t vscpclass,
                        uint8_t vscptype,
                        uint8_t nodeid,
                        uint8_t priority,
                        uint8_t size,
                        uint8_t *pData )
{
    uint32_t id = ( (uint32_t)priority << 26 ) |
                    ( (uint32_t)vscpclass << 16 ) |
                    ( (uint32_t)vscptype << 8 ) |
                    nodeid; // node address (our address)

    if ( !sendCANFrame( id, size, pData ) ) {
        return FALSE;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// getVSCPFrame
//

int8_t getVSCPFrame(uint16_t *pvscpclass,
                        uint8_t *pvscptype,
                        uint8_t *pNodeId,
                        uint8_t *pPriority,
                        uint8_t *pSize,
                        uint8_t *pData)
{
    uint32_t id;

    if ( !getCANFrame(&id, pSize, pData) ) {
        return FALSE;
    }

    *pNodeId = id & 0x0ff;
    *pvscptype = (id >> 8) & 0xff;
    *pvscpclass = (id >> 16) & 0x1ff;
    *pPriority = (uint16_t) (0x07 & (id >> 26));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// sendCANFrame
//

int8_t sendCANFrame(uint32_t id, uint8_t dlc, uint8_t *pdata)
{
    uint8_t rv = FALSE;
    sendTimer = 0;

    while ( sendTimer < 1 ) {
        if ( ECANSendMessage( id, pdata, dlc, ECAN_TX_XTD_FRAME ) ) {
            rv = TRUE;
            break;
        }
    }

    vscp_omsg.flags = 0;

    return rv;
}

///////////////////////////////////////////////////////////////////////////////
// getCANFrame
//

int8_t getCANFrame(uint32_t *pid, uint8_t *pdlc, uint8_t *pdata)
{
    ECAN_RX_MSG_FLAGS flags;

    // Don't read in new event if there already is a event
    // in the input buffer
    if (vscp_imsg.flags & VSCP_VALID_MSG) return FALSE;

    if ( ECANReceiveMessage( pid, pdata, pdlc, &flags) ) {

        // RTR not interesting
        if (flags & ECAN_RX_RTR_FRAME) return FALSE;

        // Must be extended frame
        if (!(flags & ECAN_RX_XTD_FRAME)) return FALSE;

        return TRUE;

    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// calculateSetFilterMask
//
// Calculate and set required filter and mask
// for the current decision matrix
//

void calculateSetFilterMask( void )
{
    uint8_t i,j;
    uint8_t lastOID;
    uint32_t rowmask;
    uint32_t rowfilter;

    // Reset filter masks
    uint32_t mask = 0xffffffff; // Just id 0x00000000 will come true
    uint32_t filter = 0;

    // Go through all DM rows
    for ( i=0; i < DESCION_MATRIX_ROWS; i++ ) {

        // No need to check not active DM rows
        if ( eeprom_read( VSCP_EEPROM_END + 8*i + 1 ) & 0x80 ) {

            // build the mask
            // ==============
            // We receive
            //  - all priorities
            //  - hard coded and not hard coded
            //  - from all nodes

            rowmask =
                    // Bit 9 of class mask
                    ( (uint32_t)( eeprom_read( VSCP_EEPROM_END + 8*i + 1 ) & 2 ) << 23 ) |
                    // Rest of class mask
                    ( (uint32_t)eeprom_read( VSCP_EEPROM_END + 8*i + 2 ) << 16 ) |
                    // Type mask
                    ( (uint32_t)eeprom_read( VSCP_EEPROM_END + 8*i + 4 ) << 8 ) |
					// OID  - handle later
					0xff;
                    /*( ( eeprom_read( VSCP_EEPROM_END + 8*i + 1 ) & 0x20 ) << 20 )*/;   // Hardcoded bit

            // build the filter
            // ================

            rowfilter =
                    // Bit 9 of class filter
                    ( (uint32_t)( eeprom_read( VSCP_EEPROM_END + 8*i + 1 ) & 1 ) << 24 ) |
                    // Rest of class filter
                    ( (uint32_t)eeprom_read( VSCP_EEPROM_END + 8*i + 3 ) << 16 ) |
                    // Type filter
                    ( (uint32_t)eeprom_read( VSCP_EEPROM_END + 8*i + 5 ) << 8 ) |
                    // OID Mask cleard if not same OID for all or one or more
                    // rows don't have OID check flag set.
                    eeprom_read( VSCP_EEPROM_END + 8*i );

            if ( 0 == i ) filter = rowfilter;   // Hack for first iteration loop

            // Form the mask - if one mask have a don't care (0)
            // the final mask should also have a don't care on that position
            mask &= rowmask;

            // Check the calculated filter and the current
            // filter to see if the bits are the same. If they
            // are not then clear the mask at that position
            for ( j=0; j<32; j++ ) {
                // If two bits are different we clear the mask bit
                if ( ( ( filter >> j ) & 1 ) != ( ( rowfilter >> j ) & 1 ) ) {
                    mask &= ~(1<<j);
                }
            }

            // Form the filter
            // if two bits are not the same they will be zero
            // All zeros will be zero
            // All ones will be one
            filter &= rowfilter;

            // Not check OID?
            if ( !eeprom_read( VSCP_EEPROM_END + 8*i + 1 ) & 0x40 ) {
                // No should not be checked for this position
                // This mean that we can't filter on a specific OID
                // so mask must be a don't care
                mask &= ~0xff;
            }

            if (i) {
                // If the current OID is different than the previous
                // we accept all
                for (j = 0; j < 8; j++) {
                    if ((lastOID >> i & 1)
                            != (eeprom_read(VSCP_EEPROM_END + 8 * i) >> i & 1)) {
                        mask &= (1 << i);
                    }
                }

                lastOID = eeprom_read(VSCP_EEPROM_END + 8 * i);

            } 
            else {
                // First round we just store the OID
                lastOID = eeprom_read(VSCP_EEPROM_END + 8 * i);
            }

        }
    }
    
    // Must be in config. mode to change settings.
    ECANSetOperationMode( ECAN_OP_MODE_CONFIG );

    //Set mask 1
    ECANSetRXM1Value( mask, ECAN_MSG_XTD );

    // Set filter 1
    ECANSetRXF1Value( filter, ECAN_MSG_XTD );

    // Return to Normal mode to communicate.
    ECANSetOperationMode( ECAN_OP_MODE_NORMAL );

}

