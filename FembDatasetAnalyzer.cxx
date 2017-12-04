// FembDatasetAnalyzer.cxx

#include "FembDatasetAnalyzer.h"
#include "FembTestAnalyzer.h"
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::ostringstream;

//**********************************************************************

int FembDatasetAnalyzer::process(bool print) {
  vector<int> ifmbs = {
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25
  };
  for ( int ifmb : ifmbs ) {
    string sts;
    if ( ifmb == 10 ) sts = "0822";
    if ( process(ifmb, sts, print) ) break;
    devMans.back().draw();
  }
  return 0;
}

//**********************************************************************

int FembDatasetAnalyzer::process(int ifmb, string sts, bool print) {
  const string myname = "FembDatasetAnalyzer::process: ";
  cout << "\n*************** FEMB " << ifmb << endl;

  FembTestAnalyzer fta(0,ifmb,2,2,sts,true,true);
  if ( fta.reader() == nullptr ) {
    cout << myname << "No reader found for FEMB " << ifmb << endl;
    return 1;
  }
  pedMans.emplace_back(*fta.draw("pedlim"));
  devMans.emplace_back(*fta.draw("devh"));
  if ( print ) {
    ostringstream ssfmb;
    if ( ifmb < 10 ) ssfmb << "0";
    ssfmb << ifmb;
    string sfmb = ssfmb.str();
    string fsuf = ".png";
    string fnam = "pedlim_femb" + sfmb + fsuf;
    pedMans.back().print(fnam);
    fnam = "devh_femb" + sfmb + fsuf;
    devMans.back().print(fnam);
  }
  return 0;
}

//**********************************************************************
