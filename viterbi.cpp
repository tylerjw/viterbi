#include <iostream>
#include <vector>
#include <cstdint>

using namespace std;

/* count the number of 1 bits in an int */
int count_bits(unsigned int n)
{
    int i = 0;
    for (i = 0; n != 0; i++)
        n &= n - 1;
    return i;
}

void print_8(uint8_t byte)
{
  std::cout << "b ";
  for (uint8_t mask = 0x80; mask > 0; mask >>= 1) {
    if (mask == 0x8) cout << " ";
    char c = (mask&byte) ? '1' : '0';
    cout << c;
  }
}

void print_array(uint8_t * bytes, int len)
{
  for(int i = 0; i < len; i++) {
    cout << i << ": ";
    print_8(bytes[i]);
    cout << " - " << (int)bytes[i] << endl;
  }
}

/* Encodes a packet of bits using the 1/2 rate encoder
 * inBlock - 98 bits (13 bytes)
 * outBlock - 196 bits (25 bytes)
 */
void trellis_1_2_encode(uint8_t *inBlock, uint8_t *outBlock)
{
  const uint8_t table[4][4] = {
    {0x0, 0xF, 0xC, 0x3},
    {0x4, 0xB, 0x8, 0x7},
    {0xD, 0x2, 0x1, 0xE},
    {0x9, 0x6, 0x5, 0xA}
  };
  int state = 0;

  for(int i = 0; i < 25; i++) {
    outBlock[i] = 0;
  }

  for(int i = 0; i < 48; i++) {
    int dibit = inBlock[i/4] >> (6 - (i*2) % 8) & 0x3;
    int codeword = table[state][dibit];
    state = dibit; // next state = previous input

    outBlock[i/2] |= codeword << (4 - (i*4) % 8);
  }

  cout << "inBlock --" << endl;
  print_array(inBlock, 13);
  cout << "outBlock -- " << endl;
  print_array(outBlock, 25);
}

int main() {
  uint8_t inBlock[13];
  uint8_t outBlock[25];

  for(int i = 0; i < 12; i++) {
    inBlock[i] = i;
  }

  trellis_1_2_encode(inBlock, outBlock);

  return 0;
}
