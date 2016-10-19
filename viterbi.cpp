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
    int dibit = (inBlock[i/4] >> (6 - (i*2) % 8)) & 0x3;
    int codeword = table[state][dibit];
    state = dibit; // next state = previous input

    outBlock[i/2] |= codeword << (4 - (i*4) % 8);
  }

  cout << "inBlock --" << endl;
  print_array(inBlock, 13);
  cout << "outBlock -- " << endl;
  print_array(outBlock, 25);
}


/* Decodes 1/2 rate encoded bits using viterbi.
 * encoded - 196 bits of encoded data packed (25 bytes) - input
 * decoded - 98 bits of decoded data packed (13 bytes) - output
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
    uint8_t codeword = (encoded[i/8] >> (4 - (i%8))) & 0xF; // select a 4 bit word
    // cout << i/4 + 1 << ": ";
    // print_4(codeword);
    // cout << endl;
    int survivors[4] = {0};
    array<vector<int>, 4> nextState; // for storing the next state(s) of each current end points
    fill(nextState.begin(), nextState.end(), vector<int>(0));
    int dist[4]; // distance from one point to the next 4 points

    if (i == 0) {
      for (int next = 0; next < 4; next++) {
        path_distance[next] = count_bits_4(codeword ^ table[0][next]);
        survivors[next]++;
        nextState[next].push_back(next);

        // cout << "first time " << next << " - ";
        // cout << "distance from ";
        // print_4(codeword);
        // cout << " to ";
        // print_4(table[0][next]);
        // cout << " = " << path_distance[next] << endl;

      }
    } else {
      for (int next = 0; next < 4; next++) {
        for (int path = 0; path < 4; path++) {
          int prev = paths[path].getEnd();
          // total hamming distance to the next next
          dist[path] = count_bits_4(codeword ^ table[prev][next]) + paths[path].getDist(); 

          // if (i < 8) {
          //   cout << "next: " << next << ", prev: " << prev;
          //   cout << " : dist from ";
          //   print_4(codeword);
          //   cout << " to ";
          //   print_4(table[prev][next]);
          //   cout << " = " << dist[prev] << endl;
          // }
        }
        int path = find_min(dist, 4); // index of the next with the shortest distance to new next
        path_distance[next] = dist[path]; // store the distance for this path
        survivors[path]++;
        nextState[path].push_back(next);

        // if (i < 8) { 
        //   cout << "shortest distance to " << next << " from " 
        //     << paths[path].getEnd() << " = " << path_distance[next] 
        //     << " at index: " << path << endl;
        // }
      }
    }

    // if (i < 8) {
    //   for (int i = 0; i < 4; i++) {
    //     cout << i << ": " << survivors[i] << ", ";
    //   }
    //   cout << endl;
    // }

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
        // if (i < 8) {
        //   cout << dest << " dies, to be replace by " << path << endl;
        //   cout << "paths[" << dest << "] = " << paths[dest].toString() << endl;
        // }
        paths[dest] = paths[path]; // copy the object
        // if (i < 8) {
        //   cout << "paths[" << dest << "].getDist() = " << paths[dest].toString() << endl;
        // }
        survivors[dest]++; // we have copied into this one
        // insert the new data
        paths[dest].insert(destState, destDistance);
        // if (i < 8) cout << dest << " - inserting " << destState << " at distance " << destDistance << endl;        
      }

      // now insert the original
      if (nextState[path].size() > 0) {
        int output = nextState[path][0];
        paths[path].insert(output, path_distance[output]);
      }
    }

    // if (i < 8) {
    //   for (int i = 0; i < 4; i++) {
    //     cout << paths[i].toString() << endl;
    //   }
    // }
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
    decoded[i / 4] |= path[i] << (6 - (i*2) % 8);
  }

  // cout << "lowest_distance: " << lowest_distance << endl;
  // cout << "decoded -- " << endl;
  // print_array(decoded, 13);
}

/* Decodes 1/2 rate encoded bits using viterbi.
 * encoded - 196 bits of encoded data packed (25 bytes) - input
 * decoded - 98 bits of decoded data packed (13 bytes) - output
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
    uint8_t codeword = (encoded[i/8] >> (4 - (i%8))) & 0xF; // select a 4 bit word
    // cout << i/4 + 1 << ": ";
    // print_4(codeword);
    // cout << endl;
    
    int dist[4]; // distance from one point to the next 4 points

    if (i == 0) {
      for (int next = 0; next < 4; next++) {
        next_path_distance[next] = count_bits_4(codeword ^ table[0][next]);

        trace[0][next] = 0; // all points come from 0 at the start 

        // cout << "first time " << next << " - ";
        // cout << "distance from ";
        // print_4(codeword);
        // cout << " to ";
        // print_4(table[0][next]);
        // cout << " = " << next_path_distance[next] << endl;

      }
    } else {
      for (int next = 0; next < 4; next++) {
        for (int prev = 0; prev < 4; prev++) {
          // total hamming distance to the next next
          dist[prev] = count_bits_4(codeword ^ table[prev][next]) + path_distance[prev]; 

          // if (i < 8) {
          //   cout << "next: " << next << ", prev: " << prev;
          //   cout << " : dist from ";
          //   print_4(codeword);
          //   cout << " to ";
          //   print_4(table[prev][next]);
          //   cout << " = " << dist[prev] << endl;
          // }
        }
        int prev = find_min(dist, 4); // index of the next with the shortest distance to new next
        next_path_distance[next] = dist[prev]; // store the distance for this prev
        trace[i/4][next] = prev; // trace the previous position of this next point

        // survivors[patprevh]++;
        // nextState[path].push_back(next);

        // if (i < 8) { 
        //   cout << "shortest distance to " << next << " from " 
        //     << prev << " = " << next_path_distance[next] << endl;
        // }
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

  // cout << "best_path: " << best_path << endl;
  int state = best_path;
  for (int i = 47; i >= 0; i--) {
    decoded[i / 4] |= state << (6 - (i*2) % 8);
    state = trace[i][state];
  }

  // cout << "lowest_distance: " << lowest_distance << endl;
  // cout << "decoded -- " << endl;
  // print_array(decoded, 13);
}

void viterbi_1_2_decode_copy_10000(uint8_t * encoded, uint8_t * decoded) {
  for (int i = 0; i < 10000; i++) {
    viterbi_1_2_decode_copy(encoded, decoded);
  }
}

void viterbi_1_2_decode_mat_10000(uint8_t * encoded, uint8_t * decoded) {
  for (int i = 0; i < 10000; i++) {
    viterbi_1_2_decode_mat(encoded, decoded);
  }
}

int main() {
  uint8_t inBlock[13];
  uint8_t outBlock[25];
  uint8_t decoded_copy[13];
  uint8_t decoded_mat[13];

  for(int i = 0; i < 13; i++) {
    inBlock[i] = i;
    decoded_copy[i] = 0;
    decoded_mat[i] = 0;
  }
  inBlock[12] = 0;

  trellis_1_2_encode(inBlock, outBlock);
  viterbi_1_2_decode_copy_10000(outBlock, decoded_copy);
  viterbi_1_2_decode_mat_10000(outBlock, decoded_mat);

  cout << "inBlock --" << endl;
  print_array(inBlock, 13);
  cout << "encoded -- " << endl;
  print_array(outBlock, 25);

  cout << "decoded_copy -- " << endl;
  print_array(decoded_copy, 13);
  cout << "decoded_mat -- " << endl;
  print_array(decoded_mat, 13);

  return 0;
}
