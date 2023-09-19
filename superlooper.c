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
#include <stdlib.h>

#include "looper_header.h"

// delcare functions
void init_pins(void);
void init_interrupts(void);
void delay_ms_(uint16_t); // interrupt based delay function
void toggle_loop_relays(void);
void load_presets(void);
void get_preset(void);
void save_preset(void);
void startup_led_show(void);
void engage_bypass(uint8_t);
void blink_preset_led(void);
void set_bank_leds(void);

// global variables
uint8_t switch_mask = _BV(SWITCH1) | _BV(SWITCH2) | _BV(SWITCH3) | _BV(SWITCH4) | _BV(SWITCH5); // 0x1F - 0b00011111 only 5 sitches and not all of them can be pressed at the sametime for an action to take place
volatile uint16_t milliseconds = 0; // uC millisecond counter used in delay_ms_(), and is reset throughout the application
volatile uint8_t mode = 0b00000001; // 0b00000001 = LoopMode, 0b00000010 = PresetMode
volatile uint8_t bank = 0; // start in 0 = BankA, 1 = BankB, 2 = BankC, 3 = BankD
volatile uint8_t bypass = 0;

// switch related globals:
volatile uint8_t sw_press = 0b00000000; // which switch was pressed
volatile uint8_t prev_sw = 0b00000000;
volatile uint8_t sw_hold = 0b00000000; // which switches were held
volatile uint16_t sw_hold_timer = 0; // to detect switch holds

// indexed variables
volatile uint8_t mode_relay_settings[BOOL_BANK_SIZE] = { 0, 0 }; // global boolean relay values per mode
volatile uint8_t prev_relay_state[BOOL_BANK_SIZE] = { 0, 0 }; // previous relay state for each mode. integer values are from 0-255
volatile uint8_t bank_presets[PRESET_BANK_SIZE][PRESET_LOOP_SIZE] = {
                  {0, 0, 0, 0, 0, 0, 0, 0}, // A
                  {0, 0, 0, 0, 0, 0, 0, 0}, // B
                  {0, 0, 0, 0, 0, 0, 0, 0}, // C
                  {0, 0, 0, 0, 0, 0, 0, 0}, // D
                }; // bank presets

//############
// Main app //
//############

int main(void) {
  // initialize things
  init_pins();
  init_interrupts();
  // read EEPROM, if nothing is valid (i.e) blank, then start in a default mode with every loop off in loop mode
  uint8_t *pEE = (uint8_t *)EEPROM_START;
  if ( eeprom_read_byte(pEE++) == 0xAA ) { // 0xAA is the "valid flag"
    load_presets(); 
    mode = 0b00000010; // PresetMode

  } 

 // startup in a default mode with all relays off and in loop mode

  // cool random led startup with preset LEDs
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
          if (bypass) engage_bypass(LOOP);
          else if (sw_press) toggle_loop_relays();
          // handle switch hold and save preset
          if (sw_hold) save_preset();
        }
      break;

      // preset mode
      case 0b00000010:
        // turn on preset mode led and loop led off
        set0(BMMLED_PORT, LOOPMODELED);
        set1(BMMLED_PORT, PRESETMODELED);
        // get previous preset loop setting
        get_preset();

        // TODO: if I switch banks then the presets need to change to the previous loop selection for that bank

        while (mode == 0b00000010) {

          // start in default loop 1 ?

          if (bypass) engage_bypass(PRESET);
          else if (sw_press) get_preset();
        }
        // if any switch is held while in loop mode, nothing happens
      break;
    }
  }
  return 0;
}

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

// check to see if any switch was pressed
// not all the swtiches will be pressed at the same time
  if( (SWITCH_IPA & switch_mask) != switch_mask ) {
    // see if it wasn't pressed at all
      delay_ms_(SW_DEBOUNCE_TIME);
      if ( (SWITCH_IPA & switch_mask) == switch_mask ) { // if a pin is still not GND after 50ms this was an error
        return;
      }
      // PortB is held high, so need to get the inverse bit to get
      // PINA = 0b00011111
      // switch 1 and 2 are pressed --> PINA: 0b00011100
      // switch_mask: 0b00011111
      // prev_sw = 0b00011100
      prev_sw = (SWITCH_IPA & switch_mask); // remember which switch was pressed b/c it's been longer than 50ms

      sw_hold_timer = SW_HOLD_TMR;
      while (sw_hold_timer > 0) {
        // switch was released in the mean time
        if ( (SWITCH_IPA & switch_mask) == switch_mask ) {
          sw_press = prev_sw; // know which switches have been pressed
          // check to see if the mode or bank needs to change
          switch(sw_press) { 

            case 0b00011000:  // mode selection has been pressed  - sw 1, 2, 3 
              mode == 0b00000001 ? (mode = mode << 1) : (mode = mode >> 1);
              sw_press = 0;

            case 0b00011110: // just bypass switch is pressed
              // if you press bypass again you want to come out of bypass mode
              bypass ? 0 : 1;
              sw_press = 0;

            case 0b00000011: // bank selection has been pressed - sw 3, 4, 5
              // change the bank
              if (bank == 3) bank = 0; // bank D
              else bank++; // increment banks
              sw_press = 0; // don't want to toggle any relays
              set_bank_leds();

            case 0b00011100:
              if (mode == 0b00000001) bt(RELAY1,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 0; 

           case 0b00011101:
              if (mode == 0b00000001) bt(RELAY2,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 1;

            case 0b00011001:
              if (mode == 0b00000001) bt(RELAY3,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 2;

           case 0b00011011:
              if (mode == 0b00000001) bt(RELAY4,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 3;

           case 0b00010011:
              if (mode == 0b00000001) bt(RELAY5,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 4;

            case 0b00010111:
              if (mode == 0b00000001) bt(RELAY6,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 5;

            case 0b00000111:
              if (mode == 0b00000001) bt(RELAY7,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 6;

           case 0b00001111:
              if (mode == 0b00000001) bt(RELAY8,mode_relay_settings[LOOP]);
              else mode_relay_settings[PRESET] = 7;
            }
            return;
          }
        }
        // if you have made it this far then the switches have been held
        sw_hold = sw_press; // know which switch was held after 3 secs
        sw_press = 0; 
        return;
      }
}

//###################
// Misc. functions //
//###################
void toggle_loop_relays() {
    // if the relay is off then turn it on
    // mute output
    set1(BMMLED_PORT, MUTE);
    // set relay port to desired settings
    RELAY_PORT = mode_relay_settings[LOOP];
    // delay
    delay_ms_(MUTE_DELAY);
    // unmute output
    set0(BMMLED_PORT, MUTE);
    // handle this variable
    sw_press = 0;

   return;
  }

void save_preset() {

  // save the active loops to the desired bank and preset
  // blink that loop led until all the footswitches have been release
  uint8_t *pEE = (uint8_t *)EEPROM_START; // 0
  eeprom_write_byte(pEE++, 0xAA); // say that the EEPROM is valid in register 1
  // write a byte since there are 8 bits
  // read blocks for each bank when initializing
  // write a block that is 8 bytes long for each bank
  // bank A address is 2-9
  // bank B address is 10-17
  // bank C address is 18-25
  // bank D address is 26-33

  switch (sw_hold) {

   case 0b00011100: // preset 1
    mode_relay_settings[PRESET] = 0; 
    if (bank == 0) pEE++; // bank A -- register 2
    else if (bank == 1) pEE = pEE + 9; // bank B
    else if (bank == 2) pEE = pEE + 17; // bank C
    else if (bank == 3) pEE = pEE + 25; // bank D

  case 0b00011101: // preset 2
    mode_relay_settings[PRESET] = 1;
    if (bank == 0) pEE = pEE + 2; // bank A
    else if (bank == 1) pEE = pEE + 10; // bank B
    else if (bank == 2) pEE = pEE + 18; // bank C
    else if (bank == 3) pEE = pEE + 26; // bank D

  case 0b00011001: // preset 3
    mode_relay_settings[PRESET] = 2;
    if (bank == 0) pEE = pEE + 3; // bank A
    else if (bank == 1) pEE = pEE + 11; // bank B
    else if (bank == 2) pEE = pEE + 19; // bank C
    else if (bank == 3) pEE = pEE + 27; // bank D

  case 0b00011011: // preset 4
    mode_relay_settings[PRESET] = 3;
    if (bank == 0) pEE = pEE + 4; // bank A
    else if (bank == 1) pEE = pEE + 12; // bank B
    else if (bank == 2) pEE = pEE + 20; // bank C
    else if (bank == 3) pEE = pEE + 28; // bank D

  case 0b00010011: // preset 5
    mode_relay_settings[PRESET] = 4;
    if (bank == 0) pEE = pEE + 5; // bank A
    else if (bank == 1) pEE = pEE + 13; // bank B
    else if (bank == 2) pEE = pEE + 21; // bank C
    else if (bank == 3) pEE = pEE + 29; // bank D

  case 0b00010111: // preset 6
    mode_relay_settings[PRESET] = 5;
    if (bank == 0) pEE = pEE + 6; // bank A
    else if (bank == 1) pEE = pEE + 14; // bank B
    else if (bank == 2) pEE = pEE + 22; // bank C
    else if (bank == 3) pEE = pEE + 30; // bank D

  case 0b00000111: // preset 7
    mode_relay_settings[PRESET] = 6;
    if (bank == 0) pEE = pEE + 7; // bank A
    else if (bank == 1) pEE = pEE + 15; // bank B
    else if (bank == 2) pEE = pEE + 23; // bank C
    else if (bank == 3) pEE = pEE + 31; // bank D

  case 0b00001111: // preset 8
    mode_relay_settings[PRESET] = 7;
    if (bank == 0) pEE = pEE + 8; // bank A
    else if (bank == 1) pEE = pEE + 16; // bank B
    else if (bank == 2) pEE = pEE + 24; // bank C
    else if (bank == 3) pEE = pEE + 32; // bank D -- register 33
  }
  eeprom_write_byte(pEE, mode_relay_settings[LOOP]);
  //stay in this loop until the switches have been released
  while (!(SWITCH_IPA & switch_mask) == switch_mask) {
    blink_preset_led();
  }

  sw_hold = 0; // handle variable
  return;

}

void load_presets() {
  int i, j;
  uint8_t *pEE = (uint8_t *)2; // starting register for preset 1 in bank A
  for (i=0; i<PRESET_BANK_SIZE; i++) {
    for (j=0; j<PRESET_LOOP_SIZE; j++) {
      bank_presets[i][j] = eeprom_read_byte(pEE);
      pEE = pEE + 1; // last register will be 33
    }
  }
  return;
}

void get_preset() {
  // retrieves the saved preset
  // turn off current preset led
  PRESETLED_PORT = 0;
  // turn on selected preset led
  set1(PRESETLED_PORT, mode_relay_settings[PRESET]);
  // set loop relay port to selected preset
  RELAY_PORT = bank_presets[bank][mode_relay_settings[PRESET]];
  return;
}

void engage_bypass(uint8_t index) {
  // no loop selctions have been made after the bypass
  if (mode_relay_settings[index] == 0) {
    // restore previous loop selections
    RELAY_PORT = prev_relay_state[index];
    mode_relay_settings[index] = prev_relay_state[index];
  } else {
    // save current relay settings
    prev_relay_state[index] = RELAY_PORT;
    // turn off all relays
    RELAY_PORT = 0;
    // set the current state of the relays
    mode_relay_settings[index] = 0;
  }

  bypass = 0;

  return;
}

// will only blink the preset led
void blink_preset_led() {
  set1(PRESETLED_PORT, mode_relay_settings[PRESET]);
  delay_ms_(LED_BLINK_DELAY);
  set0(PRESETLED_PORT, mode_relay_settings[PRESET]);
  delay_ms_(LED_BLINK_DELAY);

  return;
}

void set_bank_leds() {
  // turn off all bank LEDs
  bc(BANKALED, BMMLED_PORT);
  bc(BANKBLED, BMMLED_PORT);
  bc(BANKCLED, BMMLED_PORT);
  bc(BANKDLED, BMMLED_PORT);
  // turn on selected bank led
  // 1 - a
  // 2 - b
  // 3 - c
  // 4 - d
  if (bank == 0) bs(BANKALED, BMMLED_PORT);
  else if (bank == 1) bs(BANKBLED, BMMLED_PORT);
  else if (bank == 2) bs(BANKCLED, BMMLED_PORT);
  else bs(BANKDLED, BMMLED_PORT);

  return;
}

void startup_led_show() {
  // create random array of size 8 led numbers and turn them on
  // move preset leds across two times then blink end leds
  // blink preset 1 led
  int rando, i, lower = 0, upper = 7, count = 8;
  for (i=0; i<count; i++) {
    rando = (rand() % (upper-lower+1)) + lower;
    set1(PRESETLED_PORT, rando);
    delay_ms_(LED_BLINK_DELAY);
    set0(PRESETLED_PORT, rando);
    delay_ms_(LED_BLINK_DELAY);
  }
  // blink led 1 three times
  for (i=0; i<3; i++) {
    blink_preset_led();
  }
return;
}

//###################
// Init functions //
//###################
void init_pins() {

  // Switches
  SWITCH_DDR &= ~_BV(SWITCH1) | ~_BV(SWITCH2) | ~_BV(SWITCH3) | ~_BV(SWITCH4)
                  | ~_BV(SWITCH5);
  SWITCH_PORT |= _BV(SWITCH1) | _BV(SWITCH2) | _BV(SWITCH3) | _BV(SWITCH4)
                  | _BV(SWITCH5); // turn ON interal pullups

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
