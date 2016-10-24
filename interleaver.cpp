// interleaver test
// interleaver for p25

#include <iostream>

const int INTRLV_DEPTH_1 = 13; // first column is 13 deep
const int INTRLV_DEPTH_2 = 12; // second, third, and fourth is 12 deep
const int INTRLV_WIDTH = 16; // table is 4 wide * 4 bits in a dibit pair
const int BITS_PER_STEP = 4; // 4 bits in the 2 dybits

typedef size_t size_type;

/**
 * @brief interleave block of p25 data
 * @details Uses table from BAAA-A 7.2, each array is of length 196 (98 dibits)
 * 
 * @param hardBits input hard bits
 * @param softBits input soft bits
 * @param outHardBits output hard bits
 * @param outSoftBits output soft bits
 */
void blockInterleave(uint8_t * hardBits, float * softBits, uint8_t * outHardBits, float * outSoftBits)
{
   size_type ck = 0;
   for (size_type n=0; n < INTRLV_WIDTH; n+=4)
   {
    size_type depth = ((n==0) ? INTRLV_DEPTH_1 : INTRLV_DEPTH_2);
    for (size_type m=0; m < depth; m++)
    {
      // dibit 0
      outHardBits[ck + m*BITS_PER_STEP ]   = hardBits[n + m*INTRLV_WIDTH];
      outHardBits[ck + m*BITS_PER_STEP +1] = hardBits[n + m*INTRLV_WIDTH +1];

      // dibit 1
      outHardBits[ck + m*BITS_PER_STEP +2] = hardBits[n + m*INTRLV_WIDTH +2];
      outHardBits[ck + m*BITS_PER_STEP +3] = hardBits[n + m*INTRLV_WIDTH +3];

      // dibit 0
      outSoftBits[ck + m*BITS_PER_STEP ]   = softBits[n + m*INTRLV_WIDTH];
      outSoftBits[ck + m*BITS_PER_STEP +1] = softBits[n + m*INTRLV_WIDTH +1];

      // dibit 1
      outSoftBits[ck + m*BITS_PER_STEP +2] = softBits[n + m*INTRLV_WIDTH +2];
      outSoftBits[ck + m*BITS_PER_STEP +3] = softBits[n + m*INTRLV_WIDTH +3];
    }
    ck += depth * BITS_PER_STEP; 
   }
}

/**
 * @brief de-interleave block of p25 data
 * @details Uses table from BAAA-A 7.2, each array is of length 196 (98 dibits)
 * 
 * @param hardBits input hard bits
 * @param softBits input soft bits
 * @param outHardBits output hard bits
 * @param outSoftBits output soft bits
 */
void blockDeinterleave(uint8_t * hardBits, float * softBits, uint8_t * outHardBits, float * outSoftBits )
{
   size_type ck = 0;
   for (size_type n=0; n < INTRLV_WIDTH; n+=4)
   {
    size_type depth = ((n==0) ? INTRLV_DEPTH_1 : INTRLV_DEPTH_2);
    for (size_type m=0; m < depth; m++)
    {
      // dibit 0
      outHardBits[n + m*INTRLV_WIDTH] = hardBits[ck + m*BITS_PER_STEP ];
      outHardBits[n + m*INTRLV_WIDTH +1] = hardBits[ck + m*BITS_PER_STEP +1];

      // dibit 1
      outHardBits[n + m*INTRLV_WIDTH +2] = hardBits[ck + m*BITS_PER_STEP +2];
      outHardBits[n + m*INTRLV_WIDTH +3] = hardBits[ck + m*BITS_PER_STEP +3];

      // dibit 0
      outSoftBits[n + m*INTRLV_WIDTH] = softBits[ck + m*BITS_PER_STEP ];
      outSoftBits[n + m*INTRLV_WIDTH +1] = softBits[ck + m*BITS_PER_STEP +1];

      // dibit 1
      outSoftBits[n + m*INTRLV_WIDTH +2] = softBits[ck + m*BITS_PER_STEP +2];
      outSoftBits[n + m*INTRLV_WIDTH +3] = softBits[ck + m*BITS_PER_STEP +3];
    }
    
    ck += depth * BITS_PER_STEP; 
   }
}

int main() {
  uint8_t origHardBits[98*2];
  uint8_t interHardBits[98*2];
  uint8_t deinterHardBits[98*2];
  float origSoftBits[98*2];
  float interSoftBits[98*2];
  float deinterSoftBits[98*2];

  for(int i = 0; i < 98; i++) {
    origHardBits[i*2] = i;
    origHardBits[i*2+1] = i;

    origSoftBits[i*2] = i;
    origSoftBits[i*2+1] = i;
  }

  blockInterleave(origHardBits, origSoftBits, interHardBits, interSoftBits);
  blockDeinterleave(interHardBits, interSoftBits, deinterHardBits, deinterSoftBits);

  for(int i = 0; i < 98*2; i++) {
    std::cout << i << ": " << deinterSoftBits[i] << std::endl;
  }
}
