#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;
// ****************** Declarations ********************

/*
 *  Class Random implements two functions for sampling 
 *  random ints and doubles.
 */

class Random {
  public:
    // returns a random floating point number 
    // uniformly distributed in the interval [0,1] 
    static double uniform_double ();

    // returns a random integer uniformly distributed 
    // in the interval [0, n-1] 
    static int uniform_int (int n);
};
  
/*
 *  Class Graph implements a simple graph. 
 *  It has one constructor that generates a random 
 *  Erdos-Renyi graph
 */
