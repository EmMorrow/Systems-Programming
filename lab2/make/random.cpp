#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>
#include "random.h"

using namespace std;




// ***************** Implementation *******************


double Random::uniform_double () {
  return static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
}

int Random::uniform_int (int n) {
  return rand() % n;
}

