#include <iostream>
#include <cstdint>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <random>
#include <fstream>

using namespace std;

int count_bits_4(uint8_t n);
float distance(uint8_t hard, float * soft_bits);
int find_min(float * list, int len);
int find_min(int * list, int len);
void print_8(uint8_t byte);
void print_4(uint8_t byte);
void print_array(uint8_t * bytes, int len);
void print_array(float * list, int len);
void trellis_1_2_encode(uint8_t * inBlock, uint8_t * outBlock);
int viterbi_1_2_decode(uint8_t * encoded, uint8_t * decoded);
int viterbi_1_2_decode(float * encoded, uint8_t * hard_encoded, uint8_t * decoded);
int viterbi_3_4_decode(uint8_t * encoded, uint8_t * decoded);
int viterbi_3_4_decode(float * encoded, uint8_t * hard_encoded, uint8_t * decoded);

// routines for testing
void bits_to_symbols(uint8_t * dibitpairs, int len, float * symbols);
void symbols_to_bits(float * symbols, int len, uint8_t * dibitpairs, float * softbits);
void noisy_channel(float * symbols, int len, float stddev);
void zero(uint8_t * list, int len);
int test_1_2(uint8_t * expected, uint8_t * result, int len);
int test_3_4(uint8_t * expected, uint8_t * result, int len);

// do not use for values > 16
int count_bits_4(uint8_t n)
{
  const uint8_t lookup[16] = {
    0, // 0000
    1, // 0001
    1, // 0010
    2, // 0011
    1, // 0100
    2, // 0101
    2, // 0110
    3, // 0111
    1, // 1000
    2, // 1001
    2, // 1010
    3, // 1011
    2, // 1100
    3, // 1101
    3, // 1110
    4  // 1111
  };

  return lookup[n];
}

// soft bits is of size 4
float distance(uint8_t hard, float * soft_bits) {
  float distance = 0;
  uint8_t mask = 0x8;
  for(int i = 0; i < 4; mask >>= 1, i++) {
    float fhard = (hard&mask) ? 127.0 : -127.0;
    distance += fabs(fhard - soft_bits[i]);
  }
  return distance;
}

int find_min(float * list, int len)
{
  float min = list[0];  
  int index = 0;  
  int i;

  for (i = 1; i < len; i++) {
    if (list[i] < min) {
      min = list[i];
      index = i;
    }
  }

  return index;
}

/* return the index of the first lowest value in a list */
int find_min(int * list, int len)
{
  int min = list[0];  
  int index = 0;  
  int i;

  for (i = 1; i < len; i++) {
    if (list[i] < min) {
      min = list[i];
      index = i;
    }
  }

  return index;
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

void print_4(uint8_t byte)
{
  for (uint8_t mask = 0x8; mask > 0; mask >>= 1) {
    char c = (mask&byte) ? '1' : '0';
    cout << c;
  }
}

void print_array(uint8_t * bytes, int len)
{
  for(int i = 0; i < len; i+=2) {
    cout << ((i<10)?" ":"") << i << ": ";
    print_4(bytes[i]);
    cout << "(" << (int)bytes[i] << ")   " << ((i+1<10)?" ":"") << i+1 << ": ";
    print_4(bytes[i+1]);
    cout << "(" << (int)bytes[i+1] << ")" << endl;
  }
}

void print_array(float * list, int len)
{
  for(int i = 0; i < len; i++) {
    cout << ((i<10)?" ":"") << i << ": " << list[i] << endl;
  }
}

/* Encodes a packet of bits using the 1/2 rate encoder
 * inBlock - 96 bits - 48 dibits
 * outBlock - 196 bits - 48 pairs of dibits
 */
void trellis_1_2_encode(uint8_t * inBlock, uint8_t * outBlock)
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
    int dibit = inBlock[i] & 0x3; // dibits
    int codeword = table[state][dibit];
    state = dibit; // next state = previous input

    outBlock[i] = codeword;
  }
}

/* Encodes a packet of bits using the 3/4 rate encoder
 * inBlock - 144 bits - 48 tribits
 * outBlock - 196 bits - 48 pairs of dibits
 */
void trellis_3_4_encode(uint8_t * inBlock, uint8_t * outBlock)
{
  const uint8_t table[8][8] = {
    { 0, 8, 4, 12, 2, 10, 6, 14 },
    { 4, 12, 2, 10, 6, 14, 0, 8 },
    { 1, 9, 5, 13, 3, 11, 7, 15 },
    { 5, 13, 3, 11, 7, 15, 1, 9 },
    { 3, 11, 7, 15, 1, 9, 5, 13 },
    { 7, 15, 1, 9, 5, 13, 3, 11 },
    { 2, 10, 6, 14, 0, 8, 4, 12 },
    { 6, 14, 0, 8, 4, 12, 2, 10 }
  };
  int state = 0;

  for(int i = 0; i < 25; i++) {
    outBlock[i] = 0;
  }

  for(int i = 0; i < 48; i++) {
    int dibit = inBlock[i] & 0x7; // tribits
    int codeword = table[state][dibit];
    state = dibit; // next state = previous input

    outBlock[i] = codeword; // paris of dibits
  }
}

/* Decodes 1/2 rate encoded bits using viterbi.
 * encoded - 196 bits - 48 pairs of dibits
 * decoded - 96 bits - 48 dibits
 */
int viterbi_1_2_decode(uint8_t * encoded, uint8_t * decoded)
{
  const uint8_t table[4][4] = {
    {0x0, 0xF, 0xC, 0x3},
    {0x4, 0xB, 0x8, 0x7},
    {0xD, 0x2, 0x1, 0xE},
    {0x9, 0x6, 0x5, 0xA}
  };
  // this matrix is for building the paths.  Each point will be populated 
  // with the previous state at that point
  int trace[48][4]; // trace of previous points

  uint8_t hard_decoded[49];
  hard_decoded[0] = 0;

  int path_distance[4];
  for (int path = 0; path < 4; path++) path_distance[path] = 0;

  int next_path_distance[4];
  
  for (int i = 0; i < 192; i += 4)
  {
    uint8_t codeword = encoded[i/4] & 0xF;    
    int dist[4]; // distance from one point to the next 4 points

    uint8_t hard_codeword = encoded[i/4] & 0xF;
    int k = i/4;
    int min_dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][0]);
    if(min_dist) {
      for(int l = 1; l < 4; l++) {
        int dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][l]);
        if (dist < min_dist) {
          min_dist = dist;
          hard_decoded[k+1] = l;
        }
        if(!min_dist)
          break;
      }
    } else {
      hard_decoded[k+1] = 0;
    }

    if (i == 0) {
      for (int next = 0; next < 4; next++) {
        next_path_distance[next] = count_bits_4(codeword ^ table[0][next]);

        trace[0][next] = 0; // all points come from 0 at the start 
      }
    } else {
      for (int next = 0; next < 4; next++) {
        for (int prev = 0; prev < 4; prev++) {
          // total hamming distance to the next next
          dist[prev] = count_bits_4(codeword ^ table[prev][next]) + path_distance[prev]; 
        }
        int prev = find_min(dist, 4); // index of the next with the shortest distance to new next
        next_path_distance[next] = dist[prev]; // store the distance for this prev
        trace[i/4][next] = prev; // trace the previous position of this next point
      }
    }
    for (int j = 0; j < 4; j++) {
      path_distance[j] = next_path_distance[j];
    }
  }    

  // pick the path with the lowest distance
  int lowest_distance = path_distance[0];
  int best_path = 0;
  for (int i = 1; i < 4; i++) {
    if (path_distance[i] < lowest_distance) {
      lowest_distance = path_distance[i];
      best_path = i;
    }
  }

  int biterrors = 0;
  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i] = state;
    state = trace[i][state];

    if((hard_decoded[i+1]&0x1) != (decoded[i]&0x1)) {
      biterrors++;
    }
    if((hard_decoded[i+1]&0x2) != (decoded[i]&0x2)) {
      biterrors++;
    }
  }

  return biterrors;
}

/* Decodes 3/4 rate encoded bits using viterbi.
 * encoded - 196 bits - 48 pairs of dibits
 * decoded - 144 bits - 48 tribits
 */
int viterbi_3_4_decode(uint8_t * encoded, uint8_t * decoded)
{
  const uint8_t table[8][8] = {
    { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE },
    { 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x0, 0x8 },
    { 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF },
    { 0x5, 0xD, 0x3, 0xB, 0x7, 0xF, 0x1, 0x9 },
    { 0x3, 0xB, 0x7, 0xF, 0x1, 0x9, 0x5, 0xD },
    { 0x7, 0xF, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB },
    { 0x2, 0xA, 0x6, 0xE, 0x0, 0x8, 0x4, 0xC },
    { 0x6, 0xE, 0x0, 0x8, 0x4, 0xC, 0x2, 0xA }
  };
  // this matrix is for building the paths.  Each point will be populated 
  // with the previous state at that point
  int trace[48][8]; // trace of previous points

  uint8_t hard_decoded[49];
  hard_decoded[0] = 0;

  int path_distance[8];
  for (int path = 0; path < 8; path++) path_distance[path] = 0;

  int next_path_distance[8];
  
  for (int i = 0; i < 192; i += 4)
  {
    uint8_t codeword = encoded[i/4] & 0xF;
    int dist[8]; // distance from one point to the next 4 points

    uint8_t hard_codeword = encoded[i/4] & 0xF;
    int k = i/4;
    int min_dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][0]);
    if(min_dist) {
      for(int l = 1; l < 8; l++) {
        int dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][l]);
        if (dist < min_dist) {
          min_dist = dist;
          hard_decoded[k+1] = l;
        }
        if(!min_dist)
          break;
      }
    } else {
      hard_decoded[k+1] = 0;
    }

    if (i == 0) {
      for (int next = 0; next < 8; next++) {
        next_path_distance[next] = count_bits_4(codeword ^ table[0][next]);

        trace[0][next] = 0; // all points come from 0 at the start 
      }
    } else {
      for (int next = 0; next < 8; next++) {
        for (int prev = 0; prev < 8; prev++) {
          // total hamming distance to the next next
          dist[prev] = count_bits_4(codeword ^ table[prev][next]) + path_distance[prev]; 
        }
        int prev = find_min(dist, 8); // index of the next with the shortest distance to new next
        next_path_distance[next] = dist[prev]; // store the distance for this prev
        trace[i/4][next] = prev; // trace the previous position of this next point
      }
    }
    for (int j = 0; j < 8; j++) {
      path_distance[j] = next_path_distance[j];
    }
  }    

  // pick the path with the lowest distance
  int lowest_distance = path_distance[0];
  int best_path = 0;
  for (int i = 1; i < 8; i++) {
    if (path_distance[i] < lowest_distance) {
      lowest_distance = path_distance[i];
      best_path = i;
    }
  }

  int biterrors = 0;
  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i] = state;
    state = trace[i][state];

    if((hard_decoded[i+1]&0x1) != (decoded[i]&0x1)) {
      biterrors++;
    }
    if((hard_decoded[i+1]&0x2) != (decoded[i]&0x2)) {
      biterrors++;
    }
    if((hard_decoded[i+1]&0x4) != (decoded[i]&0x4)) {
      biterrors++;
    }
  }

  return biterrors;
}

/* Decodes 1/2 rate encoded bits using viterbi.
 * encoded - 196 bits - 196 float soft bits
 * decoded - 96 bits - 48 dibits
 */
int viterbi_1_2_decode(float * encoded, uint8_t * hard_encoded, uint8_t * decoded)
{
  const uint8_t table[4][4] = {
    {0x0, 0xF, 0xC, 0x3},
    {0x4, 0xB, 0x8, 0x7},
    {0xD, 0x2, 0x1, 0xE},
    {0x9, 0x6, 0x5, 0xA}
  };

  // this matrix is for building the paths.  Each point will be populated 
  // with the previous state at that point
  int trace[48][4]; // trace of previous points

  uint8_t hard_decoded[49];
  hard_decoded[0] = 0;

  float path_distance[4];
  for (int path = 0; path < 4; path++) path_distance[path] = 0;

  float next_path_distance[4];

  for (int i = 0; i < 192; i += 4)
  {
    float * codewordPtr = &encoded[i]; // is a float now    
    float dist[4]; // distance from one point to the next 4 points

    uint8_t hard_codeword = hard_encoded[i/4] & 0xF;
    int k = i/4;
    int min_dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][0]);
    if(min_dist) {
      for(int l = 1; l < 4; l++) {
        int dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][l]);
        if (dist < min_dist) {
          min_dist = dist;
          hard_decoded[k+1] = l;
        }
        if(!min_dist)
          break;
      }
    } else {
      hard_decoded[k+1] = 0;
    }

    if (i == 0) {
      for (int next = 0; next < 4; next++) {
        path_distance[next] = distance(table[0][next], codewordPtr);

        trace[0][next] = 0; // all points come from 0 at the start 
      }
    } else {
      for (int next = 0; next < 4; next++) {
        for (int prev = 0; prev < 4; prev++) {
          // total hamming distance to the next next
          dist[prev] = distance(table[prev][next], codewordPtr) + path_distance[prev]; 
        }
        int prev = find_min(dist, 4); // index of the next with the shortest distance to new next
        next_path_distance[next] = dist[prev]; // store the distance for this prev
        trace[i/4][next] = prev; // trace the previous position of this next point
      }

      for (int j = 0; j < 4; j++) {
        path_distance[j] = next_path_distance[j];
      }
    }
  }    

  // pick the path with the lowest distance
  int best_path = find_min(path_distance, 4);

  int biterrors = 0;
  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i] = state;
    state = trace[i][state];

    if((hard_decoded[i+1]&0x1) != (decoded[i]&0x1)) {
      biterrors++;
    }
    if((hard_decoded[i+1]&0x2) != (decoded[i]&0x2)) {
      biterrors++;
    }
  }

  return biterrors;
}

/* Decodes 3/4 rate encoded bits using viterbi.
 * encoded - 196 bits - 48 pairs of dibits
 * decoded - 144 bits - 48 tribits
 */
int viterbi_3_4_decode(float * encoded, uint8_t * hard_encoded, uint8_t * decoded)
{
  const uint8_t table[8][8] = {
    { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE },
    { 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x0, 0x8 },
    { 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF },
    { 0x5, 0xD, 0x3, 0xB, 0x7, 0xF, 0x1, 0x9 },
    { 0x3, 0xB, 0x7, 0xF, 0x1, 0x9, 0x5, 0xD },
    { 0x7, 0xF, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB },
    { 0x2, 0xA, 0x6, 0xE, 0x0, 0x8, 0x4, 0xC },
    { 0x6, 0xE, 0x0, 0x8, 0x4, 0xC, 0x2, 0xA }
  };
  // this matrix is for building the paths.  Each point will be populated 
  // with the previous state at that point
  int trace[48][8]; // trace of previous points

  uint8_t hard_decoded[49];
  hard_decoded[0] = 0;

  float path_distance[8];
  for (int path = 0; path < 8; path++) path_distance[path] = 0;

  float next_path_distance[8];
  
  for (int i = 0; i < 192; i += 4)
  {
    float * codewordPtr = &encoded[i]; // is a float now    
    float dist[8]; // distance from one point to the next 4 points

    uint8_t hard_codeword = hard_encoded[i/4] & 0xF;
    int k = i/4;
    int min_dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][0]);
    if(min_dist) {
      for(int l = 1; l < 8; l++) {
        int dist = count_bits_4(hard_codeword ^ table[hard_decoded[k]][l]);
        if (dist < min_dist) {
          min_dist = dist;
          hard_decoded[k+1] = l;
        }
        if(!min_dist)
          break;
      }
    } else {
      hard_decoded[k+1] = 0;
    }

    if (i == 0) {
      for (int next = 0; next < 8; next++) {
        next_path_distance[next] = distance(table[0][next], codewordPtr);

        trace[0][next] = 0; // all points come from 0 at the start 
      }
    } else {
      for (int next = 0; next < 8; next++) {
        for (int prev = 0; prev < 8; prev++) {
          // total hamming distance to the next next
          dist[prev] = distance(table[prev][next], codewordPtr) + path_distance[prev]; 
        }
        int prev = find_min(dist, 8); // index of the next with the shortest distance to new next
        next_path_distance[next] = dist[prev]; // store the distance for this prev
        trace[i/4][next] = prev; // trace the previous position of this next point
      }
    }
    for (int j = 0; j < 8; j++) {
      path_distance[j] = next_path_distance[j];
    }
  }    

  // pick the path with the lowest distance
  float lowest_distance = path_distance[0];
  int best_path = 0;
  for (int i = 1; i < 8; i++) {
    if (path_distance[i] < lowest_distance) {
      lowest_distance = path_distance[i];
      best_path = i;
    }
  }

  int biterrors = 0;
  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i] = state;
    state = trace[i][state];

    if((hard_decoded[i+1]&0x1) != (decoded[i]&0x1)) {
      biterrors++;
    }
    if((hard_decoded[i+1]&0x2) != (decoded[i]&0x2)) {
      biterrors++;
    }
    if((hard_decoded[i+1]&0x4) != (decoded[i]&0x4)) {
      biterrors++;
    }
  }

  return biterrors;
}

void bits_to_symbols(uint8_t * dibitpairs, int len, float * symbols) {
  for (int i = 0; i < len; i++) {
    uint8_t mask = 0x8;
    for(int j = 0; j < 2; j++) {
      uint8_t msb = dibitpairs[i]&mask; mask >>= 1;
      uint8_t lsb = dibitpairs[i]&mask; mask >>= 1;
      if (!msb) {
        if(!lsb) {
          symbols[i*2 + j] = 1.0;
        } else {
          symbols[i*2 + j] = 3.0;
        }
      } else { 
        if(!lsb) {
          symbols[i*2 + j] = -1.0;
        } else {
          symbols[i*2 + j] = -3.0;
        }
      }
      symbols[i*2 + j] *= 600.0;
    }
  }
}

void symbols_to_bits(float * symbols, int len, uint8_t * dibitpairs, float * softbits) 
{
  for (int i = 0; i < len; i++) {
    if(symbols[i] > 1200.0) {

      dibitpairs[i/2] |= (0x1 << ((i%2)?0:2));

      float symbol_error = symbols[i] - 1800;
      float error_frac = fabs(symbol_error / 600.0);
      if(error_frac > 1.0) error_frac = 1.0;
      softbits[i*2] = -127.0 + error_frac * 127.0;
      softbits[i*2 + 1] = 127.0 - error_frac * 127.0;

    } else if (symbols[i] > 0.0) {

      // dibitpairs[i/2] |= 0; // no shift necesary to or with 0

      float symbol_error = symbols[i] - 600;
      float error_frac = fabs(symbol_error / 600.0);
      if(error_frac > 1.0) error_frac = 1.0;
      softbits[i*2] = -127.0 + error_frac * 127.0;
      softbits[i*2 + 1] = -127.0 + error_frac * 127.0;

    } else if (symbols[i] > -1200.0) {

      dibitpairs[i/2] |= (0x2 << ((i%2)?0:2));

      float symbol_error = symbols[i] + 600;
      float error_frac = fabs(symbol_error / 600.0);
      if(error_frac > 1.0) error_frac = 1.0;
      softbits[i*2] = 127.0 - error_frac * 127.0;
      softbits[i*2 + 1] = -127.0 + error_frac * 127.0;

    } else {

      dibitpairs[i/2] |= (0x3 << ((i%2)?0:2));

      float symbol_error = symbols[i] + 1800;
      float error_frac = fabs(symbol_error / 600.0);
      if(error_frac > 1.0) error_frac = 1.0;
      softbits[i*2] = 127.0 - error_frac * 127.0;
      softbits[i*2 + 1] = 127.0 - error_frac * 127.0;

    }
  }
}

void noisy_channel(float * symbols, int len, float stddev) 
{
  std::default_random_engine generator;
  std::normal_distribution<float> distribution(0.0, stddev);
  for(int i = 0; i < len; i++) {
    float norm = distribution(generator);
    symbols[i] += norm*600.0; 
  }
}

void zero(uint8_t * list, int len) {
  for (int i = 0; i < len; i++) {
    list[i] = 0;
  }
}

int test_1_2(uint8_t * expected, uint8_t * result, int len) {
  int errors = 0;
  for (int i = 0; i < len; i++) {
    if ((result[i]&0x2) != (expected[i]&0x2)) {
      errors++;
    }
    if ((result[i]&0x1) != (expected[i]&0x1)) {
      errors++;
    }
  }
  return errors;
}

int test_3_4(uint8_t * expected, uint8_t * result, int len) {
  int errors = 0;
  for (int i = 0; i < len; i++) {
    if ((result[i]&0x4) != (expected[i]&0x4)) {
      errors++;
    }
    if ((result[i]&0x2) != (expected[i]&0x2)) {
      errors++;
    }
    if ((result[i]&0x1) != (expected[i]&0x1)) {
      errors++;
    }
  }
  return errors;
}

int main() {
  // encoder
  uint8_t inBlock[48];
  uint8_t outBlock[48];

  // channel and decoder
  float symbols[96];
  float encoded[192];
  uint8_t hardbits[48];
  float softbits[192];
  uint8_t decoded_hard[48];
  uint8_t decoded_soft[48];
  int hardErrors, softErrors;
  float hardBer;
  float softBer;
  float stddev;

  for(int i = 0; i < 48; i++) {
    inBlock[i] = i % 4;
  }

  trellis_1_2_encode(inBlock, outBlock);

  cout << "1/2 rate decoding test ***********" << endl;

  fstream fs;
  fs.open("test_1_2.csv");
  fs << "stddev, reported hard errors, reported soft errors, hard errors, soft errors, hard ber, soft ber" << endl;
  for(int i = 0; i < 100; i++) {
    stddev = 0.01 * i;

    zero(hardbits, 48);
    zero(decoded_hard, 48);
    zero(decoded_soft, 48);

    bits_to_symbols(outBlock, 48, symbols);

    noisy_channel(symbols, 96, stddev);

    symbols_to_bits(symbols, 96, hardbits, softbits);

    int repHardErrors = viterbi_1_2_decode(hardbits, decoded_hard);
    int repSoftErrors = viterbi_1_2_decode(softbits, hardbits, decoded_soft);

    hardErrors = test_1_2(inBlock, decoded_hard, 48);
    softErrors = test_1_2(inBlock, decoded_soft, 48);

    hardBer = (float)hardErrors / 96.0;
    softBer = (float)softErrors / 96.0;

    // cout << "hard decode, stddev: " << stddev << ", errors: " 
    //   << hardErrors << "/96 bits, ber: " << hardBer << endl;
    // cout << "soft decode, stddev: " << stddev << ", errors: " 
    //   << softErrors << "/96 bits, ber: " << softBer << endl;

    fs << stddev << ", " << repHardErrors << ", " 
     << repSoftErrors << ", " << hardErrors << ", " 
     << softErrors << ", " << hardBer << ", "
     << softBer << endl; 

    if(hardErrors == 96 && softErrors == 96) {
      break;
    }

  }

  fs.close();

  for(int i = 0; i < 48; i++) {
    inBlock[i] = i % 8; // tribits
  }

  trellis_3_4_encode(inBlock, outBlock);

  cout << "3/4 rate decoding test ***********" << endl;

  fs.open("test_3_4.csv");
  fs << "stddev, reported hard errors, reported soft errors, hard errors, soft errors, hard ber, soft ber" << endl;
  for(int i = 0; i < 100; i++) {
    stddev = 0.01 * i;

    zero(hardbits, 48);
    zero(decoded_hard, 48);
    zero(decoded_soft, 48);

    bits_to_symbols(outBlock, 48, symbols);

    noisy_channel(symbols, 96, stddev);

    symbols_to_bits(symbols, 96, hardbits, softbits);

    int repHardErrors = viterbi_3_4_decode(hardbits, decoded_hard);
    int repSoftErrors = viterbi_3_4_decode(softbits, hardbits, decoded_soft);

    hardErrors = test_3_4(inBlock, decoded_hard, 48);
    softErrors = test_3_4(inBlock, decoded_soft, 48);

    hardBer = (float)hardErrors / 144.0;
    softBer = (float)softErrors / 144.0;

    // cout << "hard decode, stddev: " << stddev << ", errors: " 
    //   << hardErrors << "/144 bits, ber: " << hardBer << endl;
    // cout << "soft decode, stddev: " << stddev << ", errors: " 
    //   << softErrors << "/144 bits, ber: " << softBer << endl;

    fs << stddev << ", " << repHardErrors << ", " 
     << repSoftErrors << ", " << hardErrors << ", " 
     << softErrors << ", " << hardBer << ", "
     << softBer << endl;

    if(hardErrors == 144 && softErrors == 144) {
      break;
    }

  }

  fs.close();

  return 0;
}
