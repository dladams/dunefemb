// FembDatasetAnalyzer.h

#ifndef FembDatasetAnalyzer_H
#define FembDatasetAnalyzer_H

#include "dune/DuneCommon/TPadManipulator.h"

class FembDatasetAnalyzer {

public:

  int process(bool print =false);
  int process(int ifmb, std::string sts, bool print =false);

  vector<TPadManipulator> pedMans;
  vector<TPadManipulator> devMans;

};

#endif
  
