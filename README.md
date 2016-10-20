# viterbi

Generic viterbi implementation based on P25 Air interface

1/2 rate trellis table

{{0x0, 0xF, 0xC, 0x3},
 {0x4, 0xB, 0x8, 0x7},
 {0xD, 0x2, 0x1, 0xE},
 {0x9, 0x6, 0x5, 0xA}}

 Trellis encoder is implemented as a finite state machine.  It appends a 0x00 dibit at the end of the stream to flush out the final state.

 Trellis encoder recieves m dibits as input, and outputs 2m dibits.  The encoding processs is a 4-state finite state machine with an initial state of 0.  

 The FSM used in this particular implementation has the special property of having the current input as the next state.

 For each dibit input, there is a corresponding output constellation point which is represented as a dibit pair (4-bit word).

On page 34 of BAAA-A of the P25 spec there is this table that corrisponds dibit pairs to these values.  This table was used to traslate the above table gotten from page 33 of the same document.

0 0b0010
1 0b1010
2 0b0111
3 0b1111
4 0b1110
5 0b0110
6 0b1011
7 0b0011
8 0b1101
9 0b0101
10 0b1000
11 0b0000
12 0b0001
13 0b1001
14 0b0100
15 0b1100
