/***************************************************
 *
 * "THE DON'T-CARE LICENCE"
 * I don't care, do whatever you want with this file. Go Ham.
 *
 * DLG
 *
****************************************************/

// define the Atmel ATmega328 pins to be used

// Clock Frequency
#define F_CPU 1000000UL // 1MHz
// #define F_CPU 8000000 // 8Mhz

// Pin Inputs --> Use PCINT0 Pin Change Interrupt Request

// Switches
#define SWITCH_PORT    PORTA
#define SWITCH_DDR     DDRA
#define SWITCH_IPA     PINA
#define SWITCH_PCICRBIT  PCIE0  // will enable pin intterupts on PCINT7:0
#define SWITCH_PCMSK  PCMSK0  // set to value 0x3F, first 5 bits PCINT0-5
#define SWITCH_PCINTBITS 0x3F // 0b00111111

// bits
#define SWITCH1 0   // loop 8
#define SWITCH2 1   // loop 6
#define SWITCH3 2   // loop 4
#define SWITCH4 3   // loop 2
#define SWITCH5 4   // mode switch
#define SWITCH6 5   // bypass switch

// Pin Outputs

// Relays
#define RELAY_PORT PORTC
#define RELAY_DDR DDRC
#define RELAY_IPA PINC
// bits
#define RELAY1 0
#define RELAY2 1
#define RELAY3 2
#define RELAY4 3
#define RELAY5 4
#define RELAY6 5
#define RELAY7 6
#define RELAY8 7

// Preset LEDs
#define PRESETLED_PORT PORTB
#define PRESETLED_DDR DDRB
#define PRESETLED_IPA PINB
// bits
#define PRESETLED1 0
#define PRESETLED2 1
#define PRESETLED3 2
#define PRESETLED4 3
#define PRESETLED5 4
#define PRESETLED6 5
#define PRESETLED7 6
#define PRESETLED8 7

// Bank, Mode, and Mute LEDs
#define BMMLED_PORT PORTD
#define BMMLED_DDR  DDRD
#define BMMLED_IPA  PIND
// Bank bits
#define BANKALED 0
#define BANKBLED 1
#define BANKCLED 2
#define BANKDLED 3
// Mode bits
#define PRESETMODELED 4
#define LOOPMODELED 5
// Mute bit
#define MUTE 6

// Switch related timers
#define	SW_INACTIVITY_TMR		1500	// miliseconds to pronounce inactivity of switch(es) input
#define	SW_HOLD_TMR			3000		// miliseconds to pronounce switch(es) as held rather than pressed
#define SW_DEBOUNCE_TIME 25 // ms
#define RELAY_DELAY 8 // ms
#define LED_BLINK_DELAY 150 // ms


// EEPROM addresses for settings and things
#define	EEPROM_START			0		// start of our data in eeprom is here

// Code macros
#define set0(port, pin)			( (port) &= (uint8_t)~_BV(pin) )
#define set1(port, pin)			( (port) |= (uint8_t)_BV(pin) )

// Boolean macros
#define	BPRESET				   0								// bank index[0] for user presets
#define BRELAY          1						      // bank index[1] name.. custom, for user setting relay states
#define BOOL_BANK_SIZE	2									// how many banks (bytes)

// boolean bit/flags bank 1 - Presets
#define preset1   0
#define preset2   1
#define preset3   2
#define preset4   3
#define preset5   4
#define preset6   5
#define preset7   6
#define preset8   7

// boolean bit/flags bank 2 - Switch specific
#define relay1 0
#define relay2 1
#define relay3 2
#define relay4 3
#define relay5 4
#define relay6 5
#define relay7 6
#define relay8 7

#define bv(bit,bank) 			( bools[bank] & (uint8_t)_BV(bit) ) 	// boolean values GET/READ macro: if(bv(3,BSYS))...
#define	bs(bit,bank) 			( bools[bank] |= (uint8_t)_BV(bit) ) 	// boolean values SET macro: bs(5,BSYS)
#define	bc(bit,bank) 			( bools[bank] &= (uint8_t)~_BV(bit) ) 	// boolean values CLEAR macro: bc(4,SW)
#define	bt(bit,bank) 			( bools[bank] ^= (uint8_t)_BV(bit) ) 	// boolean values TOGGLE macro: bt(6,SW)
