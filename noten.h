// Hardware definition
#define CS_PIN    10  // or SS
#define TASTER    4   // Taster auf D4

// Opcodes for the MAX7221 and MAX7219
// All OP_DIGITn are offsets from OP_DIGIT0
#define	OP_NOOP         0 ///< MAX72xx opcode for NO OP
#define OP_DIGIT0       1 ///< MAX72xx opcode for DIGIT0
#define OP_DIGIT1       2 ///< MAX72xx opcode for DIGIT1
#define OP_DIGIT2       3 ///< MAX72xx opcode for DIGIT2
#define OP_DIGIT3       4 ///< MAX72xx opcode for DIGIT3
#define OP_DIGIT4       5 ///< MAX72xx opcode for DIGIT4
#define OP_DIGIT5       6 ///< MAX72xx opcode for DIGIT5
#define OP_DIGIT6       7 ///< MAX72xx opcode for DIGIT6
#define OP_DIGIT7       8 ///< MAX72xx opcode for DIGIT7
#define OP_DECODEMODE   9 ///< MAX72xx opcode for DECODE MODE
#define OP_INTENSITY   10 ///< MAX72xx opcode for SET INTENSITY
#define OP_SCANLIMIT   11 ///< MAX72xx opcode for SCAN LIMIT
#define OP_SHUTDOWN    12 ///< MAX72xx opcode for SHUT DOWN
#define OP_DISPLAYTEST 15 ///< MAX72xx opcode for DISPLAY TEST

#define portOfPin(P)\
  (((P)>=0&&(P)<8)?&PORTD:(((P)>7&&(P)<14)?&PORTB:&PORTC))
#define ddrOfPin(P)\
  (((P)>=0&&(P)<8)?&DDRD:(((P)>7&&(P)<14)?&DDRB:&DDRC))
#define pinOfPin(P)\
  (((P)>=0&&(P)<8)?&PIND:(((P)>7&&(P)<14)?&PINB:&PINC))
#define pinIndex(P)((uint8_t)(P>13?P-14:P&7))
#define pinMask(P)((uint8_t)(1<<pinIndex(P)))

#define pinAsInput(P) *(ddrOfPin(P))&=~pinMask(P)
#define pinAsInputPullUp(P) *(ddrOfPin(P))&=~pinMask(P);digitalHigh(P)
#define pinAsOutput(P) *(ddrOfPin(P))|=pinMask(P)
#define digitalLow(P) *(portOfPin(P))&=~pinMask(P)
#define digitalHigh(P) *(portOfPin(P))|=pinMask(P)
#define isHigh(P)((*(pinOfPin(P))& pinMask(P))>0)
#define isLow(P)((*(pinOfPin(P))& pinMask(P))==0)
#define digitalState(P)((uint8_t)isHigh(P))

const char fntu[56] PROGMEM = {
  0x24, 0x24, 0x24, 0xf4, 0x24, // A
  36, 36, 36, 252, 36,      // B
  0x24, 0x24, 0x24, 0xff, 0x24, // C
  36, 164, 164, 164, 36,    // D
  0xa4, 0xe4, 0xe4, 0x64, 0x24, // E
  228, 116, 116, 52, 36,    // F
  36, 36, 36, 228, 36,      // G
  244, 60, 60, 44, 36,      // G2
  252, 46, 46, 38, 36,      // A2
  254, 39, 39, 37, 36,      // B2
  36, 36, 36, 164, 36       // F1
};

const char fntl[56] PROGMEM = {
  0x69, 0x79, 0x79, 0x5f, 0x49, // 65 - 'A'
  89, 93, 93, 79, 73, // 66 - 'B'
  0x4d, 0x4f, 0x4f, 0x4b, 0x49, // 67 - 'C'
  0xff, 0x4b, 0x49, 0x49, 0x49, // 68 - 'D'
  0xff, 0x49, 0x49, 0x49, 0x49, // 69 - 'E'
  127, 73, 73, 73, 73, // 70 - 'F'
  201, 233, 233, 127, 73, // 199 - 'G'
  95, 73, 73, 73, 73,      // G2
  95, 73, 73, 73, 73,      // A2
  75, 73, 73, 73, 73,      // B2
  201, 201, 201, 127, 73   // F1
};

// Buchstaben für Stimmgerät
const char chrs[65] PROGMEM = {
  0, 124, 126, 11, 11, 126, 124, 0,   // A
  0, 127, 127, 8, 8, 127, 127, 0,     // H
  0, 62, 127, 65, 65, 99, 34, 0,      // C
  0, 127, 127, 65, 99, 62, 28, 0,     // D
  0, 127, 127, 73, 73, 65, 65, 0,     // E
  0, 127, 127, 9, 9, 1, 1, 0,        // F
  0, 62, 127, 65, 73, 123, 58, 0,     // G
  20, 127, 127, 20, 20, 127, 127, 20 // #
};

// F-Schlüssel
const char fkey[17] PROGMEM = {
  4, 52, 28, 12, 252, 244, 4, 84,
  64, 64, 88, 78, 71, 65, 64, 64
};

// LOFX-Logo
const char lofx[141] PROGMEM = {
  0xff, 0xff, 0xff, 0xfa, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0,
  0xf0, 0x70, 0x70, 0x00, 0x03, 0x1f, 0x3f, 0x7e, 0x78, 0x78,
  0xf0, 0x00, 0x00, 0x00, 0xf0, 0x70, 0x7c, 0x3f, 0x1f, 0x0f,
  0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x03, 0x03,
  0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xc0, 0xf0, 0xf8, 0xfc,
  0x7e, 0x18, 0x01, 0x07, 0x0f, 0x1f, 0x7f, 0xfc, 0xf8, 0xf0,
  0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0xfc, 0x3e, 0x1e, 0x1f,
  0x0f, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x3e, 0xfc, 0xf8, 0xf0,
  0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xc7, 0xc7,
  0xc7, 0xc7, 0xc7, 0xc7, 0x07, 0x00, 0x01, 0x07, 0x0f, 0x1f,
  0x7f, 0xfc, 0xf8, 0xf0, 0xc0, 0x8c, 0x3e, 0x1f, 0x0f, 0x07,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char nums[105] PROGMEM = {
  0, 62, 127, 89, 77, 127, 62, 0, // 0
  0, 64, 68, 127, 127, 64, 64, 0, // 1
  0, 98, 115, 81, 73, 79, 70, 0,  // 2
  0, 34, 99, 73, 73, 127, 54, 0,  // 3
  0, 24, 24, 20, 18, 127, 127, 16,// 4
  0, 39, 103, 69, 69, 125, 57, 0, // 5
  0, 62, 127, 73, 73, 123, 50, 0, // 6
  0, 3, 3, 121, 125, 7, 3, 0,     // 7
  0, 54, 127, 73, 73, 127, 54, 0, // 8
  0, 38, 111, 73, 73, 127, 62, 0, // 9
  0, 0, 0, 0, 8, 28, 8, 0,        // +
  0, 0, 0, 0, 8, 8, 8, 0,         // -
  0, 28, 62, 34, 127, 34, 34, 0   // cent-Symbol
};
