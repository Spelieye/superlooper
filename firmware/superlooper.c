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
void startup_led_show_2(void);
void engage_bypass(uint8_t);
void blink_both_leds(void);
void blink_preset_led(void);
void set_bank_leds(void);
void switch_logic(void);

// global variables
uint8_t switch_mask = SWITCH_PCINTBITS; // 0x1F - 0b11111 only 5 switches
volatile uint16_t milliseconds = 0; // uC millisecond counter used in delay_ms_(), and is reset throughout the application
volatile uint8_t mode = 1; // 1 = LoopMode, 2 = PresetMode
volatile uint8_t bank = 0; // 0 = BankA, 1 = BankB, 2 = BankC, 3 = BankD
volatile uint8_t bypass = 0;

// switch related globals:
volatile uint8_t sw_press = 0; // which switch was pressed
volatile uint8_t sw_hold = 0; // which switches were held
volatile uint16_t sw_hold_timer = SW_HOLD_TMR; // to detect switch holds

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
  // read EEPROM, if nothing is valid (i.e) no presets have been saved previously, then start in a default mode with every loop off in loop mode
  uint8_t *pEE = (uint8_t *)EEPROM_START;
  // set to variable and read back what value is returned when nothing has been written to the register
  if ( eeprom_read_byte(pEE++) == 0xAA ) { // 0xAA is the "valid flag"
    load_presets(); 
  } 

 // startup in a saved mode, if no presets are saved then default start up in loop mode so you can save loops to a preset

  // cool random led startup with preset LEDs
  // comment this out if you don't want this to happen everytime the pedal powers on. 
  //startup_led_show();
  // sequence through the leds 
  startup_led_show_2();
  set_bank_leds();

  // main app loop
  while(1) {
    // choose which mode the pedal is in
    switch (mode) {
      // LOOP mode
      case 1:
        // turn on loop mode led and preset led off
        set1(BMMLED_PORT, LOOPLED);
        set0(BMMLED_PORT, PRESETLED);
        // clear any preset leds
        PRESETLED_PORT = 0; 
        // Turn on any loops that were left on 
        RELAY_PORT = mode_relay_settings[LOOP];
        // stay in loop mode
        while (mode == 1) {
          // set relays and handle sw_press
          if (sw_press) {
            sw_hold_timer = 0;
            while (sw_hold_timer < SW_HOLD_TMR) { 
              // switch(es) were released
              if ( (SWITCH_IPA & switch_mask) == switch_mask ) {
                switch_logic();
                if (sw_press) toggle_loop_relays();
                break;
              }
            }
            if (sw_press) {
              // if you have made it this far then the switches have been held
              sw_hold = sw_press; // know which switches were held
              sw_press = 0; 
            }
          }  
          // engage bypass -- clear all loops
          if (bypass) engage_bypass(LOOP);
          // handle switch hold and save preset
          if (sw_hold) save_preset();
        }
        break;

      // PRESET mode
      case 2:
        // turn on preset mode led and loop led off
        set0(BMMLED_PORT, LOOPLED);
        set1(BMMLED_PORT, PRESETLED);
        // get previous preset loop setting
        get_preset();

        while (mode == 2) {

          if (sw_press) {
            // preset loop selection will happen when the switch is pressed instead of when released
            // results in better timing of effect switching when playing
            switch_logic();
            // check again b/c there are certain switch selections that don't change loops  
            if (sw_press) {
              // Save the relay state only when a new loop has been selected
              // if the preset hasn't already been selected then save the last new preset selected 
              eeprom_update_byte((uint8_t *)PRESET_REGISTER, mode_relay_settings[PRESET]);
              get_preset();
            }
          }
          // clear all loops 
          if (bypass) engage_bypass(PRESET); 
          //if any switch is held while in loop mode, nothing happens
        }
        break;
    }
  }

return 0;
}


//##############################
// Interrupt: TIMER0_OVF_vect //
//##############################
ISR(TIMER0_OVF_vect, ISR_NOBLOCK) {
  // timer interupt will occur about every 2 ms
  milliseconds = milliseconds + 2; // no need to handle this since unassigned integers can never overflow in C
  if (sw_hold_timer < SW_HOLD_TMR) sw_hold_timer = sw_hold_timer + 2; 
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
      if ( (SWITCH_IPA & switch_mask) == switch_mask ) { // if a pin is still not GND after 25ms this was an error
        return;
      }
      // PINA = 0b11111
      // switch 3, 4, 5 are pressed --> PINA: 0b00011
      // switch_mask: 0b11111
      // sw_press = 0b11
      sw_press = (SWITCH_IPA & switch_mask); 
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
    // save current loop settings 
    eeprom_update_byte((uint8_t *)LOOP_REGISTER, mode_relay_settings[LOOP]);

   return;
  }


void switch_logic() {

  switch (sw_press) { 

    case 0b11110: // bypass switch is pressed - sw 1
      // if you press bypass again you want to come out of bypass mode
      bypass = bypass ? 0 : 1;
      // handle variable
      sw_press = 0;
      break;

    case 0b11000:  // mode selection has been pressed  - sw 1, 2, 3 
      (mode == 1) ? mode = 2 : (mode = 1);
      // save the "last" known mode setting pedal was in so it can start there on the next power up
      eeprom_update_byte((uint8_t *)MODE_REGISTER, mode);
      // handle variable
      sw_press = 0;
      break;

    case 0b11: // bank selection has been pressed - sw 3, 4, 5
      // change the bank
      if (bank == 3) bank = 0; // bank D
      else bank++; // increment banks
      // don't want to toggle any relays in loop mode
      if (mode == 1) sw_press = 0; 
      set_bank_leds();
      // save last bank setting 
      eeprom_update_byte((uint8_t *)BANK_REGISTER, bank);
      break;

    case 0b11100: // sw 1, 2
      if (mode == 1) bt(RELAY1,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 0; 
      break;

   case 0b11101: // sw 2
      if (mode == 1) bt(RELAY2,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 1;
      break;

    case 0b11001: // sw 2, 3 
      if (mode == 1) bt(RELAY3,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 2;
      break;

   case 0b11011: // sw 3
      if (mode == 1) bt(RELAY4,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 3;
      break;

   case 0b10011: // sw 3, 4
      if (mode == 1) bt(RELAY5,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 4;
      break;

    case 0b10111: // sw 4
      if (mode == 1) bt(RELAY6,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 5;
      break;

    case 0b111: // sw 4, 5
      if (mode == 1) bt(RELAY7,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 6;
      break;

   case 0b1111: // sw 5
      if (mode == 1) bt(RELAY8,mode_relay_settings[LOOP]);
      else mode_relay_settings[PRESET] = 7;
      break;
    }

  return;
}

void save_preset() {
  // save the active loops to the desired bank and preset
  // only need to write this once 
  uint8_t *pEE = (uint8_t *)EEPROM_START; // 0
  // write to eeprom - This update function reads the byte first and skips the burning if the old value is the same with the new value.
  eeprom_update_byte(pEE++, 0xAA); // say that the EEPROM is valid in register 1
  // bank A addresses are 2-9
  // bank B addresses are 10-17
  // bank C addresses are 18-25
  // bank D addresses are 26-33

  switch (sw_hold) {

    case 0b11100: // sw 1, 2 -- preset 1
      mode_relay_settings[PRESET] = 0;
      if (bank == 0) pEE++; // bank A -- register 2
      else if (bank == 1) pEE = pEE + 9; // bank B
      else if (bank == 2) pEE = pEE + 17; // bank C
      else if (bank == 3) pEE = pEE + 25; // bank D
      break;

    case 0b11101: // sw 2 -- preset 2
      mode_relay_settings[PRESET] = 1;
      if (bank == 0) pEE = pEE + 2; // bank A -- register 3
      else if (bank == 1) pEE = pEE + 10; // bank B
      else if (bank == 2) pEE = pEE + 18; // bank C
      else if (bank == 3) pEE = pEE + 26; // bank D
      break;

    case 0b11001: // sw 2, 3 -- preset 3
      mode_relay_settings[PRESET] = 2;
      if (bank == 0) pEE = pEE + 3; // bank A 
      else if (bank == 1) pEE = pEE + 11; // bank B
      else if (bank == 2) pEE = pEE + 19; // bank C
      else if (bank == 3) pEE = pEE + 27; // bank D
      break;

    case 0b11011: // sw 3 -- preset 4
      mode_relay_settings[PRESET] = 3;
      if (bank == 0) pEE = pEE + 4; // bank A
      else if (bank == 1) pEE = pEE + 12; // bank B
      else if (bank == 2) pEE = pEE + 20; // bank C
      else if (bank == 3) pEE = pEE + 28; // bank D
      break;

    case 0b10011: // sw 3, 4 -- preset 5
      mode_relay_settings[PRESET] = 4;
      if (bank == 0) pEE = pEE + 5; // bank A
      else if (bank == 1) pEE = pEE + 13; // bank B
      else if (bank == 2) pEE = pEE + 21; // bank C
      else if (bank == 3) pEE = pEE + 29; // bank D
      break;

    case 0b10111: // sw 4 -- preset 6
      mode_relay_settings[PRESET] = 5;
      if (bank == 0) pEE = pEE + 6; // bank A
      else if (bank == 1) pEE = pEE + 14; // bank B
      else if (bank == 2) pEE = pEE + 22; // bank C
      else if (bank == 3) pEE = pEE + 30; // bank D
      break;

    case 0b111: // sw 4, 5 -- preset 7
      mode_relay_settings[PRESET] = 6;
      if (bank == 0) pEE = pEE + 7; // bank A
      else if (bank == 1) pEE = pEE + 15; // bank B
      else if (bank == 2) pEE = pEE + 23; // bank C
      else if (bank == 3) pEE = pEE + 31; // bank D
      break;

    case 0b1111: // sw 5 -- preset 8
      mode_relay_settings[PRESET] = 7;
      if (bank == 0) pEE = pEE + 8; // bank A
      else if (bank == 1) pEE = pEE + 16; // bank B
      else if (bank == 2) pEE = pEE + 24; // bank C
      else if (bank == 3) pEE = pEE + 32; // bank D -- register 33
      break;
  }
  // put it into the variable for access now 
  bank_presets[bank][mode_relay_settings[PRESET]] = mode_relay_settings[LOOP];
  // save a preset with all 8 loops on, but is that ever going to happen? Like do you need 8 pedals on at once? 
  // well if you do, you can save such a preset once per bank wierdo
  if (mode_relay_settings[LOOP] == 255) {
    if (bank == 0) pEE = (uint8_t *)34; // bank A 
    else if (bank == 1) pEE = (uint8_t *)35; // bank B
    else if (bank == 2) pEE = (uint8_t *)36; // bank C
    else if (bank == 3) pEE = (uint8_t *)37; // bank D
    // save which preset you want to have all loops on
    eeprom_update_byte(pEE, mode_relay_settings[PRESET]);

  } else {
    // save normal preset
    eeprom_update_byte(pEE, mode_relay_settings[LOOP]);
    // handle if a previous value of 255 was saved and you want to save a new more sane preset
    if (bank == 0) pEE = (uint8_t *)34; // bank A
    else if (bank == 1) pEE = (uint8_t *)35; // bank B
    else if (bank == 2) pEE = (uint8_t *)36; // bank C
    else if (bank == 3) pEE = (uint8_t *)37; // bank D
    if (eeprom_read_byte(pEE) == mode_relay_settings[PRESET]) eeprom_update_byte(pEE, 255);

  }
  // blink that preset led until all the footswitches have been release
  while ( (SWITCH_IPA & switch_mask) != switch_mask) {
    blink_preset_led();
  }

  sw_hold = 0; // handle variable
  return;

}

void load_presets() {
  uint8_t *pEE = (uint8_t *)2; // data starts on register 2
  uint8_t reg_value;
  int i, j;

  for (i=0; i<PRESET_BANK_SIZE; i++) {
    for (j=0; j<PRESET_LOOP_SIZE; j++) {
      // only load if an address contains data
      reg_value = eeprom_read_byte(pEE);
      // a register will return an invalid data value 0xFF or 255 if it wasn't written to
      if (reg_value != 255 ) {
        bank_presets[i][j] = reg_value; 
      }
      pEE++; // last register will be 33
    }
  }
  // check for all loop on saved presets, again like why but here we go
  for (i=0; i<PRESET_BANK_SIZE; i++) {
    // only load if an address contains data
    reg_value = eeprom_read_byte(pEE++);
    if (reg_value != 255) {
      bank_presets[i][reg_value] = 255;
    }
   // last register will be 37
  }

  // register 38, saved mode
  reg_value = eeprom_read_byte(pEE++);
  if (reg_value != 255 ) {
    mode = reg_value; 
  }
  // register 39, saved bank 
  reg_value = eeprom_read_byte(pEE++);
  if (reg_value != 255) {
    bank = reg_value;
  }
  // register 40 - saved loops 
  reg_value = eeprom_read_byte(pEE++);
  if (reg_value != 255) {
    mode_relay_settings[LOOP] = reg_value;
  }
  // register 41 - saved preset 
  reg_value = eeprom_read_byte(pEE++);
  if (reg_value != 255) {
    mode_relay_settings[PRESET] = reg_value;
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
  set1(BMMLED_PORT, MUTE);
  RELAY_PORT = bank_presets[bank][mode_relay_settings[PRESET]];
  delay_ms_(MUTE_DELAY);
  set0(BMMLED_PORT, MUTE);

  sw_press = 0; // handle variable
  
  return;
}


// possible feature: hold bypass to save current state
void engage_bypass(uint8_t index) {

  if (RELAY_PORT == 0) {
    // restore previous loop selections
    set1(BMMLED_PORT, MUTE);
    RELAY_PORT = prev_relay_state[index];
    delay_ms_(MUTE_DELAY);
    // unmute output
    set0(BMMLED_PORT, MUTE);
    mode_relay_settings[index] = prev_relay_state[index];

  } else {
    // save current relay settings
    prev_relay_state[index] = RELAY_PORT;
    // mute output
    set1(BMMLED_PORT, MUTE);
    // turn off all relays
    RELAY_PORT = 0;
    delay_ms_(MUTE_DELAY);
    // unmute output
    set0(BMMLED_PORT, MUTE);
    if (index == LOOP) mode_relay_settings[index] = 0;
  }

  // save all relays in last known state in loop mode
  if (index == LOOP) eeprom_update_byte((uint8_t *)LOOP_REGISTER, mode_relay_settings[index]);

  bypass = 0;

  return;
}

// will blink both preset and loop led
void blink_both_leds() {
  set1(BMMLED_PORT, PRESETLED);
  set1(BMMLED_PORT, LOOPLED);
  delay_ms_(LED_BLINK_DELAY);
  set0(BMMLED_PORT, PRESETLED);
  set0(BMMLED_PORT, LOOPLED);
  delay_ms_(LED_BLINK_DELAY);

  return;
}

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
  // 0 - a
  // 1 - b
  // 2 - c
  // 3 - d
  if (bank == 0) bs(BANKALED, BMMLED_PORT);
  else if (bank == 1) bs(BANKBLED, BMMLED_PORT);
  else if (bank == 2) bs(BANKCLED, BMMLED_PORT);
  else bs(BANKDLED, BMMLED_PORT);

  return;
}

void startup_led_show() {
  // create random array of size 8 led numbers and turn them on
  // move preset leds across two times then blink end leds
  int rando, i, lower = 0, upper = 7, count = 8;
  for (i=0; i<count; i++) {
    rando = (rand() % (upper-lower+1)) + lower;
    set1(PRESETLED_PORT, rando);
    delay_ms_(LED_BLINK_DELAY);
    set0(PRESETLED_PORT, rando);
    delay_ms_(LED_BLINK_DELAY);
  }
  // blink led three times
  for (i=0; i<3; i++) {
    blink_both_leds();
  }
  return;
}

void startup_led_show_2() {
  // sequence through the leds 
  set1(BMMLED_PORT, PRESETLED);
  delay_ms_(LED_SQ_DELAY);
  set0(BMMLED_PORT, PRESETLED);
  delay_ms_(LED_SQ_DELAY);
  // go left
  int i, count = 8; 
  for (i=0; i<count; i++) {
    set1(PRESETLED_PORT, i);
    delay_ms_(LED_SQ_DELAY);
    set0(PRESETLED_PORT, i);
    delay_ms_(LED_SQ_DELAY);
  }
  // go right
  count = -1;
  for (i=6; i>count; i--) {
    set1(PRESETLED_PORT, i);
    delay_ms_(LED_SQ_DELAY);
    set0(PRESETLED_PORT, i);
    delay_ms_(LED_SQ_DELAY);
  }
   // blink led three times
  for (i=0; i<3; i++) {
    blink_both_leds();
  }
  return;
}

//###################
// Init functions //
//###################
void init_pins() {

  // Switches
  SWITCH_DDR &= ~_BV(SWITCH1) | ~_BV(SWITCH2) | ~_BV(SWITCH3) | ~_BV(SWITCH4) | ~_BV(SWITCH5);
  SWITCH_PORT |= _BV(SWITCH1) | _BV(SWITCH2) | _BV(SWITCH3) | _BV(SWITCH4) | _BV(SWITCH5); // turn ON interal pullups

  // Relays
  RELAY_DDR |= _BV(RELAY1) | _BV(RELAY2) | _BV(RELAY3) | _BV(RELAY4) | _BV(RELAY5) 
              | _BV(RELAY6) | _BV(RELAY7) | _BV(RELAY8);
  // LEDS
  PRESETLED_DDR |= _BV(PRESETLED1) | _BV(PRESETLED2) | _BV(PRESETLED3) | _BV(PRESETLED4) 
                  | _BV(PRESETLED5) | _BV(PRESETLED6) | _BV(PRESETLED7) | _BV(PRESETLED8);
  // Mode LEDS
  BMMLED_DDR |= _BV(BANKALED) | _BV(BANKBLED) | _BV(BANKCLED) | _BV(BANKDLED) | _BV(PRESETLED) | _BV(LOOPLED) | _BV(MUTE);

  return;

}

void init_interrupts() {

  // enable external interupts
  SWITCH_PCMSK = SWITCH_PCINTBITS; // set PCINTn pins for interrupts on pin change
  PCICR |= _BV(SWITCH_PCICRBIT);// enable wanted PCIR bits

  // use timer0 overflow -- 8-bit timer (can count to 255 before overflow)
  TCCR0B |= _BV(CS01); // 1/(1M/8/256) --> 2ms
  TIMSK0 |= _BV(TOIE0); // enable timer overflow interrupt

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
