#include <Wire.h>
#include "noten.h"
String note = " 0", oldnote;
float cents;
uint8_t buf = 0, okt = 0, maxbuf = 0;
uint32_t oldms = 1000, alive = 1000, wd = 0, oldrf = 0;
uint16_t interval = 100;
bool triggered = false;
bool stimmen = false, noten = true, gross = false;
bool dunkel = true;
bool dunkelalt = false;
uint8_t mode = 1;
#define rate 40   // ms für Shift left

char lb[64], ub[64], lbout[64], ubout[64], bufferu[70], bufferl[70];

uint32_t lastDebounce1 = 0, DDl1 = 50, buttontime = 0, releasetime = 0, lasttrigger = 0;
bool IN1 = false, pushed = false, LONG = false, OLDLONG = false, oldpushed = false;
uint8_t BS1 = 0, LS1 = LOW, OLDSTATUS1 = 0;


void initialize(uint8_t intensity = 15);

void setup() {
  Wire.begin(8);                  // join i2c bus with address #8
  Wire.setClock(100000);
  Wire.onReceive(receiveEvent);   // register event
  Serial.begin(115200);           // start serial for output
  // MAX7219 initialisieren
  pinMode(CS_PIN, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(TASTER, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  initialize();
  Serial.println("-- INIT --");
  for (uint8_t n = 0; n < 64; n++) {
    lb[n] = 0;
    ub[n] = 0;
    bufferl[n] = 0;
    bufferu[n] = 0;
  }
  // LOFX-Logo einsetzen
  for (uint8_t n = 0; n < 71; n++) {
    bufferl[n] = pgm_read_byte(&lofx[n]);
    bufferu[n] = pgm_read_byte(&lofx[n + 70]);
    buf = 70;
  }
}

void loop() {
  gettaster(TASTER);
  if (millis() > oldms + interval) {
    oldms = millis();
    // Taster auswerten

    if (oldpushed != pushed) {          // kurz drücken: Stimmgerät / Noten
      oldpushed = pushed;
      if (!pushed) {
        if (releasetime - buttontime < 500) {
          clear();
          switch (mode) {
            case 1: {
                mode = 2;
                stimmen = true;
                noten = false;
                gross = false;
                break;
              }
            case 2: {
                mode = 3;
                stimmen = false;
                noten = false;
                gross = true;
                break;
              }
            case 3: {
                mode = 1;
                stimmen = false;
                noten = true;
                gross = false;
                break;
              }
            default: {
                break;
              }
          }

          if (stimmen) {
            lb[31] = 255;
            lb[32] = 255;
            triggered = true;
          }
        }
      }
    }

    if (OLDLONG != LONG) {              // Lange drücken: Helligkeit
      OLDLONG = LONG;
      if (LONG) {
        //Serial.println("LONG");
        dunkel = !dunkel;
      }

    }

    if (stimmen && triggered) {          // Stimmgerät
      for (uint8_t n = 0; n < 64; n++) {
        lb[n] = 0;
        ub[n] = 0;
      }
      triggered = false;
      uint8_t offset = 7;
      if (note[0] == 'A') offset = 0;
      if (note[0] == 'B') offset = 1;
      if (note[0] == 'C') offset = 2;
      if (note[0] == 'D') offset = 3;
      if (note[0] == 'E') offset = 4;
      if (note[0] == 'F') offset = 5;
      if (note[0] == 'G') offset = 6;
      if (offset < 7) {
        for (uint8_t n = 0; n < 8; n++) {
          lb[63 - n] = pgm_read_byte(&chrs[n + (8 * offset)]);
          lb[47 - n] = pgm_read_byte(&nums[n + (8 * (okt - 48) )]);
        }
        if (note.indexOf('#') > -1) {
          for (uint8_t n = 0; n < 8; n++) {
            lb[55 - n] = pgm_read_byte(&chrs[n + 56]);
            lb[47 - n] = pgm_read_byte(&nums[n + (8 * (okt - 48) )]);
          }
        }
      }
      uint8_t offset2 = 0;              // Cents-Anzeige
      offset = abs(cents) / 10;
      offset2 = abs(cents) - offset * 10;
      for (uint8_t n = 0; n < 8; n++) {
        lb[23 - n] = pgm_read_byte(&nums[n + (8 * offset)]);
        lb[15 - n] = pgm_read_byte(&nums[n + (8 * offset2)]);
        lb[7 - n] = pgm_read_byte(&nums[n + 96]);
        if (cents < 0) {
          lb[31 - n] = pgm_read_byte(&nums[n + 88]);
        }
        else {
          lb[31 - n] = pgm_read_byte(&nums[n + 80]);
        }
      }
      if (cents < -32) {
        cents = -32;
        ub[62] = 255;
      }
      if (cents > 32) {
        cents = 32;
        ub[1] = 255;
      }
      lb[31] = 255;
      lb[32] = 255;
      uint8_t cents2 = cents + 32;
      ub[63 - cents2] = 15;
    } else if (noten) {                                // laufende Noten auf Linien
      if ((oldnote == note) && (!triggered)) {
        oldnote = "";
      }
      if ((buf == 0) && (triggered)) {
        triggered = false;
        if (note != oldnote) {
          //Serial.println(note + " " + String(cents) + " " + (String)okt);
          fillwnote(note[0], okt);
          buf = 5;
        }
        oldnote = note;
      }
    } else if (gross) {                               // 3. Modus
      if (millis() - lasttrigger > 250) {
        for (uint8_t n = 0; n < 64; n++) {
          lb[n] = 0;
          ub[n] = 0;
        }
      }
      if (triggered) {
        triggered = false;
        uint8_t offset = 0;
        uint8_t temp = 0;
        if (note[0] == 'A') offset = 0;
        if (note[0] == 'B') offset = 1;
        if (note[0] == 'C') offset = 2;
        if (note[0] == 'D') offset = 3;
        if (note[0] == 'E') offset = 4;
        if (note[0] == 'F') offset = 5;
        if (note[0] == 'G') offset = 6;
        for (uint8_t n = 0; n < 8; n++) {
          temp = pgm_read_byte(&chrs[n + (8 * offset)]);
          ub[47 - n * 2] = bitRead(temp, 0) * 3 + bitRead(temp, 1) * 12 + bitRead(temp, 2) * 48 + bitRead(temp, 3) * 192;
          ub[46 - n * 2] = bitRead(temp, 0) * 3 + bitRead(temp, 1) * 12 + bitRead(temp, 2) * 48 + bitRead(temp, 3) * 192;
          lb[47 - n * 2] = bitRead(temp, 4) * 3 + bitRead(temp, 5) * 12 + bitRead(temp, 6) * 48 + bitRead(temp, 7) * 192;
          lb[46 - n * 2] = bitRead(temp, 4) * 3 + bitRead(temp, 5) * 12 + bitRead(temp, 6) * 48 + bitRead(temp, 7) * 192;
          ub[31 - n * 2] = 0;
          ub[30 - n * 2] = 0;
          lb[31 - n * 2] = 0;
          lb[30 - n * 2] = 0;
        }
        if (note.indexOf('#') > -1) {
          for (uint8_t n = 0; n < 8; n++) {
            temp = pgm_read_byte(&chrs[n + 56]);
            ub[31 - n * 2] = bitRead(temp, 0) * 3 + bitRead(temp, 1) * 12 + bitRead(temp, 2) * 48 + bitRead(temp, 3) * 192;
            ub[30 - n * 2] = bitRead(temp, 0) * 3 + bitRead(temp, 1) * 12 + bitRead(temp, 2) * 48 + bitRead(temp, 3) * 192;
            lb[31 - n * 2] = bitRead(temp, 4) * 3 + bitRead(temp, 5) * 12 + bitRead(temp, 6) * 48 + bitRead(temp, 7) * 192;
            lb[30 - n * 2] = bitRead(temp, 4) * 3 + bitRead(temp, 5) * 12 + bitRead(temp, 6) * 48 + bitRead(temp, 7) * 192;
          }
        }
      }
    }
  }

  if (millis() > alive + 1000) {
    alive = millis();
    if (alive - wd > 2000) Serial.println("I²C-Bus haengt.");
    //Serial.println(wd);
  }

  if (millis() > oldrf + rate) {           // Refresh des Displays
    if (dunkel != dunkelalt) {
      if (dunkel) initialize(3);
      if (!dunkel) initialize();
      dunkelalt = dunkel;
    }

    oldrf = millis();
    if ((stimmen) || (gross)) {            // Bei Stimmgerät nur das Display refreshen
      spiTransmit();
    } else if ((noten) && (millis() - lasttrigger < 30000)) {  // ansonsten Buffer 'rausschreiben, wenn vorhanden
      if (maxbuf < buf) maxbuf = buf;
      if (buf > 0) {
        ub[0] = bufferu[maxbuf - buf];
        lb[0] = bufferl[maxbuf - buf];
        buf = buf - 1;
        if (buf == 0) maxbuf = 0;
      }
      for (uint8_t n = 0; n < 8; n++) {    // und F-Schlüssel links einsetzen
        ub[63 - n] = pgm_read_byte(&fkey[n]);
        lb[63 - n] = pgm_read_byte(&fkey[n + 8]);
      }
      spiTransmit();
      shiftleft(36, 73);                   // und mit Notenlinien nachfüttern
    } else if ((noten) && (millis() - lasttrigger >= 30000)) {  // LOFX-Logo
      for (uint8_t n = 0; n < 63; n++) {
        lb[63] = 0;
        ub[63] = 0;
        lb[62 - n] = pgm_read_byte(&lofx[n]);
        ub[62 - n] = pgm_read_byte(&lofx[n + 70]);
      }
      spiTransmit();
    }
  }
}


void receiveEvent(int howMany) {           // Das Wichtigste - Auswertung der Daten auf dem I²C-Bus
  uint8_t i = 0;
  char buffer[10] = {'\0\0\0\0\0\0\0\0\0\0'};
  while (Wire.available()) {
    char c = Wire.read();
    buffer[i] = c;
    i++;
  }
  // Watchdog auf dem Bus
  if (buffer[0] == 'W') {
    wd = millis();
  } else {
    note = getValue(buffer, '|', 1);
    if (note.indexOf('#') > -1) {
      okt = note[2];
    } else {
      okt = note[1];
    }
    String ctstr = getValue(buffer, '|', 0);
    cents = (ctstr.toFloat() / 100) - 50;
    triggered = true;
    lasttrigger = millis();
  }

}
