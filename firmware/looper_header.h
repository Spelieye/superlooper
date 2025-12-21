/***************************************************
 *
 * "THE DON'T-CARE LICENCE"
 * I don't care, do whatever you want with this file. Go Ham.
 *
 * DLG
 *
****************************************************/

// define the Atmel ATmega644 pins to be used

// Clock Frequency
#define F_CPU 1000000UL // 1MHz
// #define F_CPU 8000000 // 8Mhz

// Pin Inputs --> Use PCINT0 Pin Change Interrupt Request

// Switches
#define SWITCH_PORT       PORTA
#define SWITCH_DDR        DDRA
#define SWITCH_IPA        PINA
#define SWITCH_PCICRBIT   PCIE0  // will enable pin intterupts on PCINT5:0
#define SWITCH_PCMSK      PCMSK0  // set to value 0x3F, first 5 bits PCINT0-5
#define SWITCH_PCINTBITS  0x1F  // 0b11111
// bits
#define SWITCH1 0   // bypass
#define SWITCH2 1   // loop 2
#define SWITCH3 2   // loop 4
#define SWITCH4 3   // loop 6
#define SWITCH5 4   // loop 8

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
#define BANKALED  0
#define BANKBLED  1
#define BANKCLED  2
#define BANKDLED  3
// Mode bits
#define PRESETLED 4 
#define LOOPLED   5
// Mute bit
#define MUTE      6

// Switch related timers
#define	SW_HOLD_TMR			3000 // miliseconds to pronounce switch(es) as held rather than pressed
#define SW_DEBOUNCE_TIME    25 // ms
#define MUTE_DELAY          8 // ms
#define LED_BLINK_DELAY     150 // ms
#define LED_SQ_DELAY        50 // ms

// EEPROM addresses for settings and things
#define	EEPROM_START		0		// start of our data in eeprom is here
#define MODE_REGISTER		38 
#define BANK_REGISTER		39 
#define LOOP_REGISTER		40
#define PRESET_REGISTER		41

// Arrays
#define	LOOP		     0 	// bank index[0] for preset mode relay states
#define PRESET           1	 // bank index[1] for loop mode relay states
#define BOOL_BANK_SIZE	 2	 // how many bytes in the array
#define PRESET_BANK_SIZE 4  // how many banks
#define PRESET_LOOP_SIZE 8  // how many loop settings in each bank

// Code macros
#define set0(port, pin)         ( (port) &= (uint8_t)~_BV(pin) )
#define set1(port, pin)         ( (port) |= (uint8_t)_BV(pin) )
#define bv(bit,bank)            ( (bank) & (uint8_t)_BV(bit) )  // boolean values GET/READ macro: if(bv(3,BSYS))...
#define bs(bit,bank)            ( (bank) |= (uint8_t)_BV(bit) )     // boolean values SET macro: bs(5,BSYS)
#define bc(bit,bank)            ( (bank) &= (uint8_t)~_BV(bit) )    // boolean values CLEAR macro: bc(4,SW)
#define bt(bit,bank)            ( (bank) ^= (uint8_t)_BV(bit) )     // boolean values TOGGLE macro: bt(6,SW)

