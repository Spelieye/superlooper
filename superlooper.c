/***************************************************
 *
 * "THE DON'T-CARE LICENCE"
 * I don't care, do whatever you want with this file. Go Ham.
 *
 * DLG
 *
****************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
// #include <util/delay.h>
#include <avr/eeprom.h>

#include "looper_header.h"

// delcare functions
void init_pins(void);
void init_interrupts(void);
void delay_ms_(uint16_t); // interrupt based delay function
// void toogle_relays(void);
void set_relays(void);


// global variables
uint8_t switch_mask = 0b00111111;
volatile uint16_t milliseconds = 0; // uC millisecond counter used in delay_ms_(), and is reset throughout the application
volatile uint8_t mode = 0; // 0b00000001 = LoopMode, 0b00000010 = PresetMode
volatile uint8_t prev_mode = 0; // previous mode
volatile uint8_t bank = 0b00000001; // start in BankA, 0b00000010 = BankB, 0b00000100 = BankC, 0b00001000 = BankD

typedef int bool; // define boolean type
#define True 1
#define False 0

volatile bool bypass_mode = False; // start out not in bypass mode

// switch related globals:
// olatile uint8_t switch_press = 0b00000000; // which switch was pressed --> can read this from the port pin interupt bus 
volatile uint8_t relay_state = 0b00000000; // desired state state
volatile uint16_t sw_hold_timer = 0; // to detect switch holds


volatile uint8_t bools[BOOL_BANK_SIZE] = { 0, 0 }; // global boolean values per bank


//############
// Main app //
//############

int main(void) {
  /* code */
  // initialize things
  init_pins();
  init_interrupts();
  // read EEPROM, if nothing is valid (i.e) blank, then start in a default mode with every loop on in loop mode
//   uint8_t *pEE = (uint8_t *)EEPROM_START;
//   if ( eeprom_read_byte(pEE++) == 0xAA ) { // 0xAA is the "valid flag"
//
// } else {
 // nothing is saved in EEPROM so startup in a default mode
  mode = 0b00000001; // loop mode
  bools[BRELAY] = 0xFF; // set all the bits to 1
  sw_press = 1; // to execute toogle_relays()
// }

  // cool light chase fade on startup with preset LEDs




// main app loop
while(1) {
  // choose which mode pedal is in
  switch(mode) {
    // loop mode
    case 0b00000001:
    // turn on loop mode led and preset led off
    set1(BMMLED_PORT, LOOPMODELED);
    set0(BMMLED_PORT, PRESETMODELED);
    // stay in loop mode
      while (mode == 0b00000001) {
        // handle switch press and set relay
        if (sw_press != 0) {
          set_relays();
        }
        // handle switch hold and save preset
        if (sw_hold != 0 ) {
          save_preset();
        }
      }
    break;

    // preset mode
    case 0b00000010:
      preset_mode();
    break;

    // bypass mode
    case 0b00000100:
    // need to remember the last relay state and mode when coming out of bypass mode
    // check to see if bypass mode has been pressed again
    // toggle off any relays and LED's
    // maybe blink a mode LED ??
    // only want to toggle once. use a whiile loop here?
    while (bypass) {
      // if the relay ports are already off then do nothing
      if (RELAY_PORT != 0) {
        bools[BRELAY] = 0;
        toggle_relays();
      }
      // blink_led(); // use pin and port
      set1(BMMLED_PORT, PRESETMODELED);
      delay_ms_(LED_BLINK_DELAY);
      set0(BMMLED_PORT, PRESETMODELED);
      /// blink LED to indicate bypass mode
    }
    mode = prev_mode; // return to previious mode
    break;
  }



}

  return 0;
}

// use pin interupts

// user timmer interupts for counting

//##############################
// Interrupt: TIMER1_COMPA_vect //
//##############################
ISR(TIMER1_COMPA_vect, ISR_NOBLOCK) {
  milliseconds++; // need to handle this incase a switch is not pressed for a long time to prevent overflow
  if(sw_hold_timer) sw_hold_timer--;
  if(sw_inactivity_timer) sw_inactivity_timer--;

}


//##############################
// Interrupt: PCINT0_vect //
//##############################
ISR(PCINT0_vect, ISR_NOBLOCK) {

// check to see if any button was pressed
  if( (SWITCH_IPA & switch_mask) < switch_mask ) {

    // see if it wasn't pressed at all
      delay_ms_(SW_DEBOUNCE_TIME);
      if ( (SWITCH_IPA & switch_mask) == switch_mask ) { // if a pin is still not GND after 50ms this was an error
        return;
      }

      prev_sw |= SWITCH_IPA; // remember which switch was pressed b/c it's been longer than 50ms

      sw_hold_timer = SW_HOLD_TMR;
      while(sw_hold_timer > 0) {
        // switch was released in the mean time
        if ( (SWITCH_IPA & switch_mask) == switch_mask ) {
          sw_press |= prev_sw; // know which switch was pressed at this time
          // check to see if the mode or bank needs to change
          if ( sw_press == 0b00101111 )  { // only switch5 (mode switch) is pressed
              if (mode & 0b00000001) { // in LoopMode
                mode = mode << 1; // change to PresetMode
              } else { // in PresetMode
                mode = mode >> 1; // change to LoopMode
              }
            } else if ( sw_press == 0b00011111  ) { // just bypass switch is pressed
              mode = 0b00000100;
              // if you press bypass again you want to come out of bypass mode
              if (bypass) {
                bypass = False;
              } else {
                prev_mode = mode; // remember the last mode for coming out of bypass
                bypass = True;
              }

            } else if ( sw_press == 0b00001111 ) { // both mode and bypass are pressed
              // change the bank
              if (bank == 0b00001000)  { // bank D
                bank = bank >> 3; // shift 3 bits to bank A
              } else {
                bank = bank << 1; // shift 1 bit to the left to the next bank
              }
            }
            // set boolean flags for which relays need to change
            // only issue is that only one relay can be changed by the switches at a time
            else if ( sw_press == 0b00100111 ) bt(relay1,BRELAY);
            else if ( sw_press == 0b00110111 ) bt(relay2,BRELAY);
            else if ( sw_press == 0b00110011 ) bt(relay3,BRELAY);
            else if ( sw_press == 0b00111011 ) bt(relay4,BRELAY);
            else if ( sw_press == 0b00111001 ) bt(relay5,BRELAY);
            else if ( sw_press == 0b00111101 ) bt(relay6,BRELAY);
            else if ( sw_press == 0b00111100 ) bt(relay7,BRELAY);
            else if ( sw_press == 0b00111110 ) bt(relay8,BRELAY);

          }
        }
        sw_hold |= SWITCH_IPA; // know which switch was held after 3 secs
      }

  return;
}

//###################
// Misc. functions //
//###################
void toogle_relays() {
    // if the relay is off the turn it on,

    // mute output
    set1(BMMLED_PORT, MUTE);
    // set relay ports by toggling the correspnding port bit
    RELAY_PORT ^= bools[BRELAY];
    // delay
    delay_ms_(RELAY_DELAY);
    // unmute output
    set0(BMMLED_PORT, MUTE);
    // handle relays and switches
    bools[BRELAY] = 0;
    sw_press = 0;

   return;
  }



//###################
// Init functions //
//###################
void init_pins() {

  // Switches
  SWITCH_DDR &= ~_BV(SWITCH1) | ~_BV(SWITCH2) | ~_BV(SWITCH3) | ~_BV(SWITCH4)
                  | ~_BV(SWITCH5) | ~_BV(SWITCH6);
  SWITCH_PORT |= _BV(SWITCH1) | _BV(SWITCH2) | _BV(SWITCH3) | _BV(SWITCH4)
                  | _BV(SWITCH5) | _BV(SWITCH6); // turn ON interal pullups

  // Relays
  RELAY_DDR |= _BV(RELAY1) | _BV(RELAY2) | _BV(RELAY3) | _BV(RELAY4) | _BV(RELAY5)
              | _BV(RELAY6) | _BV(RELAY7) | _BV(RELAY8);
  // LEDS
  PRESETLED_DDR |= _BV(PRESETLED1) | _BV(PRESETLED2) | _BV(PRESETLED3)
                  | _BV(PRESETLED4) | _BV(PRESETLED5) | _BV(PRESETLED6)
                    | _BV(PRESETLED7) | _BV(PRESETLED8);
  // Mode LEDS
  BMMLED_DDR |= _BV(BANKALED) | _BV(BANKBLED) | _BV(BANKCLED) | _BV(BANKDLED)
              | _BV(PRESETMODELED) | _BV(LOOPMODELED) | _BV(MUTE);

  return;

}

void init_interrupts() {

  // enable external interupts
  SWITCH_PCMSK = SWITCH_PCINTBITS; // set PCINTn pins for interrupts on pin change
  PCICR |= _BV(SWITCH_PCICRBIT);// enable wanted PCIR bits

  // use timer1 -- 16-bit timer --(can count to 65536 before overflow) want this to execute every millisecond
  TCCR1A |= _BV(COM1A1); // clear OC0A on compare match
  TCCR1B |= _BV(CS10) | _BV(WGM12); // no clock prescaler
  OCR1A = 1000; // compare match value to trigger interupt every 1ms --> (1/1E6) * 1000 = 0.001
  TIMSK1 |= _BV(OCIE1A); // enable comapre match interupt

  sei(); // enable interupts
  return;
}

//##########################
// Interrupt driven delay //
//##########################
// interrupt based delay function
void delay_ms_(uint16_t ms) {
  milliseconds = 0;
  while (milliseconds < ms);

  return;
}
