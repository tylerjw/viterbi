all: viterbi interleaver

viterbi: viterbi.cpp
	c++ viterbi.cpp -ggdb -o viterbi
interleaver: interleaver.cpp
	c++ interleaver.cpp -ggdb -o interleaver

clean:
	rm viterbi interleaver
