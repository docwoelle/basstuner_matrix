
#include <SPI.h>

void rotate(uint8_t w[8], uint8_t (&d)[8]) {
  for (uint8_t i = 0; i < 8; i++) d[i] = bitRead(w[7], i) * 128 + bitRead(w[6], i) * 64 + bitRead(w[5], i) * 32 + bitRead(w[4], i) * 16 + bitRead(w[3], i) * 8 + bitRead(w[2], i) * 4 + bitRead(w[1], i) * 2 + bitRead(w[0], i);
}

void spiTransmit()
{
  /*
     Originalzustand
    128 64  32  16  8   4   2   1  <-- (Byte in Zeile)
    o   o   o   o   o   o   o   o   1  (Zeile)
    o   o   o   o   o   o   o   o   2
    o   o   o   o   o   o   o   o   3
    o   o   o   o   o   o   o   o   4
    o   o   o   o   o   o   o   o   5
    o   o   o   o   o   o   o   o   6
    o   o   o   o   o   o   o   o   7
    o   o   o   o   o   o   o   o   8
  */

  uint8_t w[8];
  // d hält die gedrehten 8 bytes
  uint8_t d[8];
  for (uint8_t n = 0; n < 8; n++) {
    // Startposition Lower Bytes
    uint8_t i = n * 8;
    uint8_t j = (7 - n) * 8;
    // w füllen mit 8 Bytes aus lower byte, beginnend rechts unten
    for (uint8_t o = 0; o < 8; o++) {
      w[o] = lb[i + o];
    }
    rotate (w, d);
    for (uint8_t o = 0; o < 8; o++) {
      lbout[j + o] = d[o];
    }

    // w füllen mit 8 Bytes aus upper byte, beginnend rechts oben
    for (uint8_t o = 0; o < 8; o++) {
      w[7 - o] = ub[i + o];
    }
    rotate (w, d);
    for (uint8_t o = 0; o < 8; o++) {
      ubout[i + (7 - o)] = d[o];
    }
  }

  for (uint8_t z = 1; z < 9; z++) {             // Zeile (1-7)
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalLow(CS_PIN);
    for (uint8_t s = 0; s < 8; s++) {          // Display (0-8)
      SPI.transfer(z);   // Zeile
      SPI.transfer(ubout[s * 8 + z - 1]); // Byte in Zeile (upper bytes)
    }
    for (uint8_t s = 0; s < 8; s++) {          // Display (0-8)
      SPI.transfer(z);   // Zeile
      SPI.transfer(lbout[s * 8 + z - 1]); // Byte in Zeile
    }
    digitalHigh(CS_PIN);
    SPI.endTransaction();
  }
}

void spiTr(uint8_t opCode, uint8_t data)
{
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalLow(CS_PIN);
  SPI.transfer(opCode);
  SPI.transfer(data);
  digitalHigh(CS_PIN);
  SPI.endTransaction();
}


void initialize(uint8_t intensity = 15)
{
  spiTr(OP_SHUTDOWN, 1);  // wake up
  spiTr(OP_SCANLIMIT, 7); // all on
  spiTr(OP_INTENSITY, intensity); // mid intensity
  spiTr(OP_DISPLAYTEST, 0); // no test
  spiTr(OP_DECODEMODE, 0);  // no decode
  clear();
}

void clear(void)
{
  for (uint8_t s = 0; s < 16; s++) {
    for (uint8_t i = 0; i < 8; i++)
      spiTr(OP_DIGIT0 + i, 0);
  }
}
