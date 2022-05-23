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
void toogle_relays(uint8_t);
void load_presets(void);
void startup_led_show(void);
void engage_bypass(void);

// global variables
uint8_t switch_mask = 0x1F; // only 5 sitches and not all of them can be pressed at the sametime for an action to take place
volatile uint16_t milliseconds = 0; // uC millisecond counter used in delay_ms_(), and is reset throughout the application
volatile uint8_t mode = 0; // 0b00000001 = LoopMode, 0b00000010 = PresetMode
volatile uint8_t prev_mode = 0; // previous mode
volatile uint8_t bank = 0b00000001; // start in BankA, 0b00000010 = BankB, 0b00000100 = BankC, 0b00001000 = BankD
volatile uint8_t loop_settings = 0b00000000; // loop that wants to be set regardless of which mode you are in
volatile uint8_t bypass = 0;
// typedef int bool; // define boolean type
// #define True 1
// #define False 0

// switch related globals:
volatile uint8_t sw_press = 0b00000000; // which switch was pressed --> can read this from the port pin interupt bus
volatile uint8_t prev_sw = 0b00000000;
volatile uint16_t sw_hold_timer = 0; // to detect switch holds
// volatile uint16_t sw_inactivity_timer = 0;

// indexed variables
volatile uint8_t mode_relay_settings[BOOL_BANK_SIZE] = { 0, 0 }; // global boolean relay values per mode
// volatile uint8_t bypass_mode[BOOL_BANK_SIZE] = { 0, 0 };  // start out not in bypass for each mode
volatile uint8_t prev_relay_state[BOOL_BANK_SIZE] = { 0, 0 }; // previous relay state for each mode
volatile uint8_t presets[PRESET_BANK_SIZE][PRESET_LOOP_SIZE]; // presets


//############
// Main app //
//############

int main(void) {
  /* code */
  // initialize things
  init_pins();
  init_interrupts();
  // read EEPROM, if nothing is valid (i.e) blank, then start in a default mode with every loop on in loop mode
  uint8_t *pEE = (uint8_t *)EEPROM_START;
  if ( eeprom_read_byte(pEE++) == 0xAA ) { // 0xAA is the "valid flag"
    load_presets(); //TODO: how to save a multi-index array to eeprom ? what will the memory location flag be?
} else {
 // nothing is saved in EEPROM so startup in a default mode:
  mode = 0b00000001; // loop mode
  sw_press = 1; // to execute toogle_relays()
}

// cool light chase fade on startup with preset LEDs
// if you don't want this to happen everytime the pedal powers on then commment it out.
startup_led_show();


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
        // set relays and handle sw_press
        // TODo: only want a loop selection to tigger an action
        if (sw_press & ) {
          mode_relay_settings[RELAYLOOP] = loop_settings;
          toggle_relays(mode_relay_settings[RELAYLOOP]);
      }

        // handle switch hold and save preset
        if (sw_hold) save_preset();

        // save current relay state
        // turn off all relays
        // turn on all relays to previous relay state
        if (bypass) engage_bypass();
      }
    break;

    // TODO: set the bank for each mode

    // preset mode
    case 0b00000010:
      // turn on preset mode led and loop led off
      set0(BMMLED_PORT, LOOPMODELED);
      set1(BMMLED_PORT, PRESETMODELED);
      while (mode == 0b00000010) {
        if (sw_press) toggle_relays();
      }
      preset_mode();
    break;

    // bypass mode
  //   case 0b00000100:
  //   // need to remember the last relay state and mode when coming out of bypass mode
  //   // check to see if bypass mode has been pressed again
  //   // toggle off any relays and LED's
  //   // maybe blink a mode LED ??
  //   // only want to toggle once. use a whiile loop here?
  //   // while in bypass mode -- allow channels to be turned on
  //   // use prev_sw variable to set the relays when you press bypass again!
  //   while (bypass) {
  //     // if the relay ports are already off then do nothing
  //     if (RELAY_PORT) {
  //       // want to turn off all relays that are on on.
  //       prev_relay_state[BLOOP] = BLOOP; // remember the current loops that are on before turning them off
  //       mode_relay_settings[BLOOP] = 0;
  //       toggle_relays();
  //     }
  //   }
  //   mode = prev_mode; // return to previous mode
  //   break;
  // }



}

  return 0;
}

// use pin interupts

// user timmer interupts for counting

//##############################
// Interrupt: TIMER1_COMPA_vect //
//##############################
ISR(TIMER1_COMPA_vect, ISR_NOBLOCK) {
  // timer interupt will occur about every ms
  milliseconds++; // no need to handle this since unassigned integers can never overflow in C
  if(sw_hold_timer) sw_hold_timer--;

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

      sw_press = (SWITCH_IPA & switch_mask); // remember which switch was pressed b/c it's been longer than 50ms

      sw_hold_timer = SW_HOLD_TMR;
      while (sw_hold_timer > 0) {
        // switch was released in the mean time
        if ( (SWITCH_IPA & switch_mask) == switch_mask ) {
          prev_sw |= sw_press; // know which switches have been pressed
          // check to see if the mode or bank needs to change
          if ( sw_press == 0b00010001)  { // mode selection has been pressed  - sw 6 & 4 & 2
              // if (mode & 0b00000001) { // in LoopMode
              //   mode = mode << 1; // change to PresetMode
              // } else { // in PresetMode
              //   mode = mode >> 1; // change to LoopMode
              // }
              // if in loop mode then change to preset mode else change to loop mode
              mode == 0b00000001 ? mode = mode << 1 : mode = mode >> 1;
            } else if ( sw_press == 0b00011110) { // just bypass switch is pressed
              // if you press bypass again you want to come out of bypass mode
              bypass ? 0 : 1;
              // if (bypass) {
              //   bypass = False;
              // } else {
              //   prev_mode = mode; // remember the last mode for coming out of bypass
              //   mode = 0b00000100; // bypass mode
              //   bypass = True;
              // }

            } else if ( sw_press == 0b00000011) { // bank selection has been pressed - sw 8 & 6 & 4
              // change the bank
              if (bank == 0b00001000)  { // bank D
                bank = bank >> 3; // shift 3 bits to bank A
              } else {
                bank = bank << 1; // shift 1 bit to the left to the next bank
              }
            }
            // set boolean flags for which relays need to change
            // only issue is that only one relay can be changed by the switches at a time
            // TODO: correct these to only work with 5 switches
            // these
            else if ( sw_press == 0b00011100 ) bt(relay1,loop_settings);
            else if ( sw_press == 0b00011101 ) bt(relay2,loop_settings);
            else if ( sw_press == 0b00011001 ) bt(relay3,loop_settings);
            else if ( sw_press == 0b00011011 ) bt(relay4,loop_settings);
            else if ( sw_press == 0b00010011 ) bt(relay5,loop_settings);
            else if ( sw_press == 0b00010111 ) bt(relay6,loop_settings);
            else if ( sw_press == 0b00000111 ) bt(relay7,loop_settings);
            else if ( sw_press == 0b00001111 ) bt(relay8,loop_settings);

          }
        }
        sw_hold = sw_press; // know which switch was held after 3 secs
      }

  return;
}

//###################
// Misc. functions //
//###################
void toogle_relays(uint8_t relay_settings) {
    // if the relay is off then turn it on
    // mute output
    set1(BMMLED_PORT, MUTE);
    // set relay ports by toggling the correspnding port bit
    // Turning off relays will also turn off the corresponding loop led since the relay coil and led are in series
    RELAY_PORT ^= relay_settings;
    // delay
    delay_ms_(MUTE_DELAY);
    // unmute output
    set0(BMMLED_PORT, MUTE);
    // handle relays and switches
    // mode_relay_settings[BLOOP] = 0;
    sw_press = 0;

   return;
  }

void preset_mode() {
  // while condition to stay in this mode
  // set the corresponding saved loops in each bank

}

void save_preset() {
  //save preset to desired loop channel in the desired bank
  // blink that loop led until all the footswitches have been release
  //TODO: how to save a multi-index array to eeprom ? what will the memory location flag be?

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
