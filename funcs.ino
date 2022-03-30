String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void shiftleft(uint8_t oben, uint8_t unten) {
  for (uint8_t n = 63; n > 0; n--) {
    lb[n] = lb[n - 1];
    ub[n] = ub[n - 1];
    //Serial.println((String)(n-1)+": "+(String)lb[n-1]+0x30);
  }
  ub[0] = oben;
  lb[0] = unten;
}

void fillwnote(char note, uint8_t okt) {
  uint8_t offset = 0;
  if ((note == 'A')  && (okt <= 49)) offset = 0;
  if ((note == 'A')  && (okt > 49))  offset = 8;
  if ((note == 'B')  && (okt <= 49)) offset = 1;
  if ((note == 'B')  && (okt > 49))  offset = 9;
  if (note == 'C') offset = 2;
  if (note == 'D') offset = 3;
  if (note == 'E') offset = 4;
  if ((note == 'F')  && (okt <= 49)) offset = 10;
  if ((note == 'F')  && (okt >  49)) offset = 5;
  if ((note == 'G') && (okt <= 49)) offset = 6;
  if ((note == 'G') && (okt > 49))  offset = 7;
  for (uint8_t n = 0; n < 5; n++) {
    bufferu[n] = pgm_read_byte(&fntu[n + (5 * offset)]);
    bufferl[n] = pgm_read_byte(&fntl[n + (5 * offset)]);
  }
}

void gettaster(uint8_t t) {
  // Taster lesen und debouncen
  IN1 = isLow(t);
  if (IN1 != LS1) {
    lastDebounce1 = millis();
  }
  if ((millis() - lastDebounce1) > DDl1) {
    if (BS1 != IN1) {
      BS1 = IN1;
    }
  }
  if ((OLDSTATUS1 != BS1)) {
    OLDSTATUS1 = BS1;
    if (BS1 == 1) {
      buttontime = millis();
      pushed = true;
    }
    else {
      releasetime = millis();
      pushed = false;
    }
  }
  if ((millis() - buttontime > 500) && (!LONG) && (pushed)) LONG = true;
  if ((millis() - buttontime > 500) && (LONG) && (!pushed)) LONG = false;
  LS1 = IN1;
}
