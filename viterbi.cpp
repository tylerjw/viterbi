#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include <sstream>

#include "viterbi.h"

using namespace std;

int count_bits_4(uint8_t n);
int find_min(int * list, int len);
void print_8(uint8_t byte);
void print_4(uint8_t byte);
void print_array(uint8_t * bytes, int len);
void trellis_1_2_encode(uint8_t * inBlock, uint8_t * outBlock);
void viterbi_1_2_decode_copy(uint8_t * encoded, uint8_t * decoded);
void viterbi_1_2_decode_mat(uint8_t * encoded, uint8_t * decoded);

std::string ViterbiPath::toString() {
  stringstream ss;
  ss << "{";
  for (int i = 0; i < path.size()-1; i++) {
    ss << path[i] << ", ";
  }
  ss << path.back() << "} - (" << dist << ")";

  return ss.str();
}

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
    4 // 1111
  };

  return lookup[n];
}

/* return the index of the first lowest value in a list */
int find_min(int * list, int len)
{
  int min = list[0];  
  int index = 0;  
  int unique = 1; 
  int i;

  for (i = 1; i < len; i++) {
    if (list[i] < min) {
      min = list[i];
      index = i;
      unique = 1;
    } else if (list[i] == min) {
      unique = 0;
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
    cout << i << ": ";
    print_4(bytes[i]);
    cout << "(" << (int)bytes[i] << ")   " << i << ": ";
    print_4(bytes[i+1]);
    cout << "(" << (int)bytes[i+1] << ")" << endl;
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
void viterbi_1_2_decode_copy(uint8_t * encoded, uint8_t * decoded)
{
  const uint8_t table[4][4] = {
    {0x0, 0xF, 0xC, 0x3},
    {0x4, 0xB, 0x8, 0x7},
    {0xD, 0x2, 0x1, 0xE},
    {0x9, 0x6, 0x5, 0xA}
  };
  ViterbiPath paths[4];
  int path_distance[4];
  for (int path = 0; path < 4; path++) path_distance[path] = 0;
  
  for (int i = 0; i < 192; i += 4)
  {
    uint8_t codeword = encoded[i/4] & 0xF; // pairs of dibits
    int survivors[4] = {0};
    array<vector<int>, 4> nextState; // for storing the next state(s) of each current end points
    fill(nextState.begin(), nextState.end(), vector<int>(0));
    int dist[4]; // distance from one point to the next 4 points

    if (i == 0) {
      for (int next = 0; next < 4; next++) {
        path_distance[next] = count_bits_4(codeword ^ table[0][next]);
        survivors[next]++;
        nextState[next].push_back(next);
      }
    } else {
      for (int next = 0; next < 4; next++) {
        for (int path = 0; path < 4; path++) {
          int prev = paths[path].getEnd();
          // total hamming distance to the next next
          dist[path] = count_bits_4(codeword ^ table[prev][next]) + paths[path].getDist(); 
        }
        int path = find_min(dist, 4); // index of the next with the shortest distance to new next
        path_distance[next] = dist[path]; // store the distance for this path
        survivors[path]++;
        nextState[path].push_back(next);
      }
    }

    for (int path = 0; path < 4; path++) {
      int pathObjIndex = 0;

      // first do the forked ones
      for (int next = 1; next < nextState[path].size(); next++) {
        int destState = nextState[path][next];
        int destDistance = path_distance[destState];

        // fork!
        // find a path to copy into
        bool foundDest = false;
        int dest = 0;
        for (int end = 0; end < 4; end++) {
          if (!survivors[end]) {
            dest = end;
            foundDest = true;
            break;
          }
        }

        if (!foundDest) {
          cerr << "ERROR: did not find path to copy into" << endl;
          break;
        }

        paths[dest] = paths[path]; // copy the object

        survivors[dest]++; // we have copied into this one
        // insert the new data
        paths[dest].insert(destState, destDistance);
      }

      // now insert the original
      if (nextState[path].size() > 0) {
        int output = nextState[path][0];
        paths[path].insert(output, path_distance[output]);
      }
    }
  }

  // pick the path with the lowest distance
  int lowest_distance = paths[0].getDist();
  int lowest_path = 0;
  for (int i = 1; i < 4; i++) {
    if (paths[i].getDist() < lowest_distance) {
      lowest_distance = paths[i].getDist();
      lowest_path = i;
    }
  }

  vector<int> path = paths[lowest_path].getPath();
  for (int i = 0; i < path.size(); i++) {
    decoded[i] = path[i];
  }
}

/* Decodes 1/2 rate encoded bits using viterbi.
 * encoded - 196 bits - 48 pairs of dibits
 * decoded - 96 bits - 48 dibits
 */
void viterbi_1_2_decode_mat(uint8_t * encoded, uint8_t * decoded)
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

  int path_distance[4];
  for (int path = 0; path < 4; path++) path_distance[path] = 0;

  int next_path_distance[4];
  
  for (int i = 0; i < 192; i += 4)
  {
    uint8_t codeword = encoded[i/4] & 0xF;    
    int dist[4]; // distance from one point to the next 4 points

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
    for (int i = 0; i < 4; i++) {
      path_distance[i] = next_path_distance[i];
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

  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i] = state;
    state = trace[i][state];
  }
}

/* Decodes 3/4 rate encoded bits using viterbi.
 * encoded - 196 bits - 48 pairs of dibits
 * decoded - 144 bits - 48 tribits
 */
void viterbi_3_4_decode_mat(uint8_t * encoded, uint8_t * decoded)
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
  // this matrix is for building the paths.  Each point will be populated 
  // with the previous state at that point
  int trace[48][8]; // trace of previous points

  int path_distance[8];
  for (int path = 0; path < 8; path++) path_distance[path] = 0;

  int next_path_distance[8];
  
  for (int i = 0; i < 192; i += 4)
  {
    uint8_t codeword = encoded[i/4] & 0xF;
    int dist[8]; // distance from one point to the next 4 points

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
    for (int i = 0; i < 8; i++) {
      path_distance[i] = next_path_distance[i];
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

  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i] = state;
    state = trace[i][state];
  }
}

int main() {
  uint8_t inBlock[48];
  uint8_t outBlock[48];
  uint8_t decoded[48];
  bool testPass;

  cout << "1/2 rate encoding and decoding ********************  ";

  for(int i = 0; i < 48; i++) {
    inBlock[i] = i % 4;
    decoded[i] = 0;
  }
  inBlock[12] = 0;

  trellis_1_2_encode(inBlock, outBlock);
  viterbi_1_2_decode_mat(outBlock, decoded);

  testPass = true;
  for(int i = 0; i < 48; i++) {
    if(inBlock[i] != decoded[i]) {
      testPass = false;
      break;
    }
  }

  if(!testPass) {
    cout << "FAIL!" << endl;
    cout << "inBlock --" << endl;
    print_array(inBlock, 48);
    cout << "encoded -- " << endl;
    print_array(outBlock, 48);
    cout << "decoded -- " << endl;
    print_array(decoded, 48);
  } else {
    cout << "Pass" << endl;
  }

  cout << "3/4 rate encoding and decoding ********************  ";

  for (int i = 0; i < 48; i++) {
    inBlock[i] = i % 8; // tribits
    decoded[i] = 0;
  }

  trellis_3_4_encode(inBlock, outBlock);
  viterbi_3_4_decode_mat(outBlock, decoded);

  testPass = true;
  for(int i = 0; i < 48; i++) {
    if(inBlock[i] != decoded[i]) {
      testPass = false;
      break;
    }
  }

  if(!testPass) {
    cout << "FAIL!" << endl;
    cout << "inBlock --" << endl;
    print_array(inBlock, 48);
    cout << "encoded -- " << endl;
    print_array(outBlock, 48);
    cout << "decoded -- " << endl;
    print_array(decoded, 48);
  } else {
    cout << "Pass" << endl;
  }

  return 0;
}
