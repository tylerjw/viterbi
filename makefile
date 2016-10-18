all: viterbi

viterbi: viterbi.cpp
	c++ viterbi.cpp -ggdb -o viterbi

clean:
	rm viterbi
