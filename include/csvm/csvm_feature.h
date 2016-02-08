#ifndef CSVM_FEATURE_H
#define CSVM_FEATURE_H

#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;
namespace csvm{
  
  class Feature{
    
    
  public:
     bool debugOut, normalOut;
      vector< double> content;
      string label;
      int labelId;
      int size;
      Feature(int size,double initValue);
      Feature(Feature* f);
      Feature(vector<double>& vect);
      void setLabelId(int id);
      unsigned int getLabelId();
      double getDistanceSq(Feature& f);
      double getManhDist(Feature* f);
  };
  
  
  
}

#endif