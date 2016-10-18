// File:  main.h

/* ******************************************************************************
 * VSCP (Very Simple Control Protocol)
 * http://www.vscp.org
 *
 * Copyright (C) 1995-2005 Ake Hedman, eurosource
 * Copyright (C) 1995-2013 Ake Hedman, Grodans Paradis AB
 *                          <akhe@grodansparadis.com>
 *
 *  This work is licensed under the Creative Common 
 *  Attribution-NonCommercial-ShareAlike 3.0 Unported license. The full
 *  license is available in the top folder of this project (LICENSE) or here
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 *  It is also available in a human readable form here 
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/
 * 
 *	This file is part of VSCP - Very Simple Control Protocol 	
 *	http://www.vscp.org
 *
 * ******************************************************************************
 */

#ifndef BEIJING_H
#define BEIJING_H

//Defines
#define	TRUE			1
#define	FALSE			0

// IO0 - RB0 INT0/AN10 (weak pull up)
// IO1 - RB1 INT1/AN8 (weak pull up)
// IO2 - RC3 SCK/SCL (I2C/SPI clock))
// IO3 - RC4 SDI/SDA (I2C data/SPI data in MISO)
// IO4 - RC5 SDO (SPI data out MOSI)
// IO5 - RC6 TX/Sync. clock
// IO6 - RC7 RX/Sync. data
// IO7 - RA2 AD2
// IO8 - RA1 AN1
// IO9 - RA0 AN0

//
// 8 MHz with PLL => 8 MHz
// 1:4 prescaler => 1 MHz (1 uS cycle )
// 1 ms == 1000 uS
// 65535 - 1000 = 64535 = 0xfc17
//
// Timer2 use 250 and prescaler 1:4
//
//#define TIMER0_RELOAD_VALUE               0xfc17

//
// 10 MHz with PLL => 40 MHz
// 1:4 prescaler => 1.25 MHz ( 0.800 uS cycle )
// 1 ms == 1000 uS
// 65535 - 1250 = 64285 = 0xfb1d
//
// Timer2 use 156 and prescaler 1:8
//
#define TIMER0_RELOAD_VALUE                 0xfb1d

//
// Timer 2 is used as a 1 ms clock
// 156 is loaded eight time to give ~1250 cycles
// Timer2 use 156 and prescaler 1:4, Postscaler 1:16
// 100 ns * 56 * 4 * 16 ~ 1 ms
//
#define TIMER2_RELOAD_VALUE                 156

#define STATUS_LED                          PORTCbits.RC1
#define INIT_BUTTON                         PORTCbits.RC0

// Defines for channels LATBbits.LATB0
#define CHANNEL0                            PORTBbits.RB0
#define CHANNEL1                            PORTBbits.RB1
#define CHANNEL2                            PORTCbits.RC3
#define CHANNEL3                            PORTCbits.RC4
#define CHANNEL4                            PORTCbits.RC5
#define CHANNEL5                            PORTCbits.RC6
#define CHANNEL6                            PORTCbits.RC7
#define CHANNEL7                            PORTAbits.RA2
#define CHANNEL8                            PORTAbits.RA1
#define CHANNEL9                            PORTAbits.RA0

#define LATCH_CHANNEL0                      LATBbits.LATB0
#define LATCH_CHANNEL1                      LATBbits.LATB1
#define LATCH_CHANNEL2                      LATCbits.LATC3
#define LATCH_CHANNEL3                      LATCbits.LATC4
#define LATCH_CHANNEL4                      LATCbits.LATC5
#define LATCH_CHANNEL5                      LATCbits.LATC6
#define LATCH_CHANNEL6                      LATCbits.LATC7
#define LATCH_CHANNEL7                      LATAbits.LATA2
#define LATCH_CHANNEL8                      LATAbits.LATA1
#define LATCH_CHANNEL9                      LATAbits.LATA0

#define CHANNEL_DIRECTION_OUTPUT            0
#define CHANNEL_DIRECTION_INPUT             1

// * * * Channel states * * *
#define CHANNEL_STATE_INACTIVE              0
#define CHANNEL_STATE_ACTIVE                2
#define CHANNEL_STATE_PULSE                 3

#define CHANNEL_CONTROLBIT_STATE            0x01 	// Current channel state

// Bits for output control register
#define OUTPUT_CTRL_PULSE                   0x01    // Enable pulse output
#define OUTPUT_CTRL_ALARM                   0x02    // Send alarm on protection time activation
#define OUTPUT_CTRL_PROTECTION              0x04 	// Enable protection timer
#define OUTPUT_CTRL_ONEVENT                 0x08 	// Send ON event            
#define OUTPUT_CTRL_OFFEVENT                0x10 	// Semd OFF event
#define OUTPUT_CTRL_ENABLED                 0x80 	// Relay enabled

// Bits for input control registers
#define INPUT_CTRL_EVENT_ON                 0x01    // Send On/TurnOn event when input goes to active state.
#define INPUT_CTRL_EVENT_OFF                0x02    // Send Off/TurnOff event when input goes to inactive state.
#define INPUT_CTRL_EVENT_SELECT             0x04    // 0 = Send On/off events. 1=Send turnon/turnoff events (if activated).
#define INPUT_CTRL_ALARM_LOW                0x08    // Alarm event sent if input goes low.  
#define INPUT_CTRL_ALARM_HIGH               0x10    // Alarm event sent if input goes high.
#define INPUT_CTRL_ALARM_CONTINIOUS         0x20    // Continious alarm
#define INPUT_CTRL_DEBOUNCE                 0x40    // Activate debounce
#define INPUT_CTRL_ENABLE                   0x80    // Enable Input.

// Bits for module control byte
#define MODULE_CTRL_DISABLE_REPEAT          0x01    // Disable repeat
#define MODULE_CTRL_PULLUP                  0x80    // Activate pullups
        
// I/O Direction 0 = output 1= input
#define CHANNEL_DEFAULT_DIRECTION_MSB       0x00    // Channel 8/9
#define CHANNEL_DEFAULT_DIRECTION_LSB       0x00    // Channel 0-7

// Alarm bits
#define ALARM_STATE_PROTECTION              0x01    // One of the protection timers has ellapsed
#define ALARM_INPUT_HIGH                    0x02    // Input has gone low
#define ALARM_INPUT_LOW                     0x04    // Input has gone high

#define DEBOUNCE_COUNT_DEFAULT              3       // 30 ms debounce

#define SHORT_PULSE_DEFAULT                 2       // 20 ms short pulse

// -----------------------------------------------

// * * *  Registers * * *

// * * * * Page 0 * * * *

#define REG0_BEIJING_ZONE                   0u
#define REG0_BEIJING_SUBZONE                1u

#define REG0_BEIJING_CH0_SUBZONE            2u
#define REG0_BEIJING_CH1_SUBZONE            3u
#define REG0_BEIJING_CH2_SUBZONE            4u
#define REG0_BEIJING_CH3_SUBZONE            5u
#define REG0_BEIJING_CH4_SUBZONE            6u
#define REG0_BEIJING_CH5_SUBZONE            7u
#define REG0_BEIJING_CH6_SUBZONE            8u
#define REG0_BEIJING_CH7_SUBZONE            9u
#define REG0_BEIJING_CH8_SUBZONE            10u
#define REG0_BEIJING_CH9_SUBZONE            11u

#define REG0_BEIJING_IO_DIRECTION_MSB       12u
#define REG0_BEIJING_IO_DIRECTION_LSB       13u

#define REG0_BEIJING_CH0_STATUS             14u  // Not written to EEPROM
#define REG0_BEIJING_CH1_STATUS             15u  // Not written to EEPROM
#define REG0_BEIJING_CH2_STATUS             16u  // Not written to EEPROM
#define REG0_BEIJING_CH3_STATUS             17u  // Not written to EEPROM
#define REG0_BEIJING_CH4_STATUS             18u  // Not written to EEPROM
#define REG0_BEIJING_CH5_STATUS             19u  // Not written to EEPROM
#define REG0_BEIJING_CH6_STATUS             20u  // Not written to EEPROM
#define REG0_BEIJING_CH7_STATUS             21u  // Not written to EEPROM
#define REG0_BEIJING_CH8_STATUS             22u  // Not written to EEPROM
#define REG0_BEIJING_CH9_STATUS             23u  // Not written to EEPROM

#define REG0_BEIJING_CH0_OUTPUT_CTRL        24u
#define REG0_BEIJING_CH1_OUTPUT_CTRL        25u
#define REG0_BEIJING_CH2_OUTPUT_CTRL        26u
#define REG0_BEIJING_CH3_OUTPUT_CTRL        27u
#define REG0_BEIJING_CH4_OUTPUT_CTRL        28u
#define REG0_BEIJING_CH5_OUTPUT_CTRL        29u
#define REG0_BEIJING_CH6_OUTPUT_CTRL        30u
#define REG0_BEIJING_CH7_OUTPUT_CTRL        31u
#define REG0_BEIJING_CH8_OUTPUT_CTRL        32u
#define REG0_BEIJING_CH9_OUTPUT_CTRL        33u

#define REG0_BEIJING_CH0_INPUT_CTRL         34u
#define REG0_BEIJING_CH1_INPUT_CTRL         35u
#define REG0_BEIJING_CH2_INPUT_CTRL         36u
#define REG0_BEIJING_CH3_INPUT_CTRL         37u
#define REG0_BEIJING_CH4_INPUT_CTRL         38u
#define REG0_BEIJING_CH5_INPUT_CTRL         39u
#define REG0_BEIJING_CH6_INPUT_CTRL         40u
#define REG0_BEIJING_CH7_INPUT_CTRL         41u
#define REG0_BEIJING_CH8_INPUT_CTRL         42u
#define REG0_BEIJING_CH9_INPUT_CTRL         43u

#define REG0_BEIJING_MODULE_CTRL            44u

#define REG0_BEIJING_STREAM_TIMING          45u

#define REG0_BEIJING_DEBOUNCE_COUNT         46u

#define REG0_BEIJING_SHORT_PULSE_TIME       47u

#define REG0_COUNT                          48u  // Needed for EEPROM writes

// * * * * Page 1 * * * *

#define REG1_BEIJING_CH0_TIMING_PULSE_MSB   0u
#define REG1_BEIJING_CH0_TIMING_PULSE_LSB   1u
#define REG1_BEIJING_CH1_TIMING_PULSE_MSB   2u
#define REG1_BEIJING_CH1_TIMING_PULSE_LSB   3u
#define REG1_BEIJING_CH2_TIMING_PULSE_MSB   4u
#define REG1_BEIJING_CH2_TIMING_PULSE_LSB   5u
#define REG1_BEIJING_CH3_TIMING_PULSE_MSB   6u
#define REG1_BEIJING_CH3_TIMING_PULSE_LSB   7u
#define REG1_BEIJING_CH4_TIMING_PULSE_MSB   8u
#define REG1_BEIJING_CH4_TIMING_PULSE_LSB   9u
#define REG1_BEIJING_CH5_TIMING_PULSE_MSB   10u
#define REG1_BEIJING_CH5_TIMING_PULSE_LSB   11u
#define REG1_BEIJING_CH6_TIMING_PULSE_MSB   12u
#define REG1_BEIJING_CH6_TIMING_PULSE_LSB   13u
#define REG1_BEIJING_CH7_TIMING_PULSE_MSB   14u
#define REG1_BEIJING_CH7_TIMING_PULSE_LSB   15u
#define REG1_BEIJING_CH8_TIMING_PULSE_MSB   16u
#define REG1_BEIJING_CH8_TIMING_PULSE_LSB   17u
#define REG1_BEIJING_CH9_TIMING_PULSE_MSB   18u
#define REG1_BEIJING_CH9_TIMING_PULSE_LSB   19u

#define REG1_BEIJING_CH0_TIMING_PROTECT_MSB 20u
#define REG1_BEIJING_CH0_TIMING_PROTECT_LSB 21u
#define REG1_BEIJING_CH1_TIMING_PROTECT_MSB 22u
#define REG1_BEIJING_CH1_TIMING_PROTECT_LSB 23u
#define REG1_BEIJING_CH2_TIMING_PROTECT_MSB 24u
#define REG1_BEIJING_CH2_TIMING_PROTECT_LSB 25u
#define REG1_BEIJING_CH3_TIMING_PROTECT_MSB 26u
#define REG1_BEIJING_CH3_TIMING_PROTECT_LSB 27u
#define REG1_BEIJING_CH4_TIMING_PROTECT_MSB 28u
#define REG1_BEIJING_CH4_TIMING_PROTECT_LSB 29u
#define REG1_BEIJING_CH5_TIMING_PROTECT_MSB 30u
#define REG1_BEIJING_CH5_TIMING_PROTECT_LSB 31u
#define REG1_BEIJING_CH6_TIMING_PROTECT_MSB 32u
#define REG1_BEIJING_CH6_TIMING_PROTECT_LSB 33u
#define REG1_BEIJING_CH7_TIMING_PROTECT_MSB 34u
#define REG1_BEIJING_CH7_TIMING_PROTECT_LSB 35u
#define REG1_BEIJING_CH8_TIMING_PROTECT_MSB 36u
#define REG1_BEIJING_CH8_TIMING_PROTECT_LSB 37u
#define REG1_BEIJING_CH9_TIMING_PROTECT_MSB 38u
#define REG1_BEIJING_CH9_TIMING_PROTECT_LSB 39u

#define REG1_COUNT                          40u

//      Page 2

#define DESCION_MATRIX_PAGE                 2
#define DESCION_MATRIX_ROWS                 8
#define REG_DESCION_MATRIX                  0    // Start of matrix

// * * * Persistent storage

#define VSCP_EEPROM_BOOTLOADER_FLAG         0x00	// Reserved for bootloader

#define VSCP_EEPROM_NICKNAME                0x01	// Persistant nickname id storage
#define VSCP_EEPROM_CONTROL1                0x02	// Persistant control byte
#define VSCP_EEPROM_CONTROL2                0x03	// Persistant control byte

//#define EEPROM_ZONE                       0x04	// Zone node belongs to
//#define EEPROM_SUBZONE                    0x05	// Subzone node belongs to

#define VSCP_EEPROM_REG_USERID              0x06
#define VSCP_EEPROM_REG_USERID1             0x07
#define VSCP_EEPROM_REG_USERID2             0x08
#define VSCP_EEPROM_REG_USERID3             0x09
#define VSCP_EEPROM_REG_USERID4             0x0A

// The following can be stored in flash or eeprom

#define VSCP_EEPROM_REG_MANUFACTUR_ID0      0x0B
#define VSCP_EEPROM_REG_MANUFACTUR_ID1      0x0C
#define VSCP_EEPROM_REG_MANUFACTUR_ID2      0x0D
#define VSCP_EEPROM_REG_MANUFACTUR_ID3      0x0E

#define VSCP_EEPROM_REG_MANUFACTUR_SUBID0   0x0F
#define VSCP_EEPROM_REG_MANUFACTUR_SUBID1   0x10
#define VSCP_EEPROM_REG_MANUFACTUR_SUBID2   0x11
#define VSCP_EEPROM_REG_MANUFACTUR_SUBID3   0x12

// The following can be stored in program ROM (recommended) or in EEPROM

#define VSCP_EEPROM_REG_GUID                0x13	// Start of GUID MSB
// 		0x13 - 0x22

#define VSCP_EEPROM_REG_DEVICE_URL          0x23	// Start of Device URL storage
// 		0x23 - 0x42

#define VSCP_EEPROM_END                     0x43	// marks end of VSCP EEPROM usage
                                                    //   (next free position)

// --------------------------------------------------------------------------------

#define BEIJING_ACTION_NOOP                 0
#define BEIJING_ACTION_SET                  1
#define BEIJING_ACTION_CLR                  2
#define BEIJING_ACTION_PULSEON              3
#define BEIJING_ACTION_PULSEOFF             4
#define BEIJING_ACTION_TOGGLE               5
#define BEIJING_ACTION_STATUS               6
#define BEIJING_ACTION_STATUSALL            7
#define BEIJING_ACTION_SHORTPULSE           8

// Function Prototypes

void doWork(void);
void init(void);
void init_app_ram(void);
void init_app_eeprom(void);
void read_app_register(unsigned char reg);
void write_app_register(unsigned char reg, unsigned char val);
void sendDMatrixInfo(void);
void SendInformationEvent(unsigned char idx, unsigned char eventClass, unsigned char eventTypeId);

void doDM(void);

void doActionOn(unsigned char dmflags, unsigned char arg);
void doActionOff(unsigned char dmflags, unsigned char arg);
void doActionPulseOn(unsigned char dmflags, unsigned char arg);
void doActionPulseOff(unsigned char dmflags, unsigned char arg);
void doActionStatus(unsigned char dmflags, unsigned char arg);
void doActionStatusAll(unsigned char dmflags, unsigned char arg);
void doActionToggle(unsigned char dmflags, unsigned char arg);
void doActionShortPulse( unsigned char dmflags, unsigned char arg );

void doApplicationOneSecondWork(void);

/*!
        Send Extended ID CAN frame
        @param id CAN extended ID for frame.
        @param size Number of databytes 0-8
        @param pData Pointer to databytes of frame.
        @return TRUE (!=0) on success, FALSE (==0) on failure.
 */
int8_t sendCANFrame(uint32_t id, uint8_t size, uint8_t *pData);

/*!
        Get extended ID CAN frame
        @param pid Pointer to CAN extended ID for frame.
        @param psize Pointer to number of databytes 0-8
        @param pData Pointer to databytes of frame.
        @return TRUE (!=0) on success, FALSE (==0) on failure.
 */
int8_t getCANFrame(uint32_t *pid, uint8_t *psize, uint8_t *pData);

#endif
