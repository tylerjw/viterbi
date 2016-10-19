#ifndef _VITERBI_H_
#define _VITERBI_H_

#include <vector>
#include <string>

class ViterbiPath
{
public:
  ViterbiPath() : dist(0) {}
  ~ViterbiPath() {}

  std::vector<int> getPath() { return path; }
  void insert(int dibit, int distance) { path.push_back(dibit); dist = distance; }
  int getEnd() { return (path.size()) ? path.back() : 0; }
  int getDist() { return dist; }

  std::string toString();
  
private:
  std::vector<int> path;
  int dist;

};

#endif // VITERBI_H
