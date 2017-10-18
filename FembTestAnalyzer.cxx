// FembTestAnalyzer.cxx

#include "FembTestAnalyzer.h"
#include <iostream>
#include "DuneFembFinder.h"

using std::string;
using std::cout;
using std::endl;

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int a_femb, int a_gain, int a_shap, 
                 std::string a_tspat, bool a_isCold,
                 bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(a_femb, a_tspat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock);
}

//**********************************************************************

FembTestAnalyzer::FembTestAnalyzer(int a_femb, string a_tspat, bool a_isCold )
: m_femb(a_femb), m_tspat(a_tspat), m_isCold(a_isCold),
  m_gain(99), m_shap(99), m_extPulse(false), m_extClock(false) { }

//**********************************************************************

int FembTestAnalyzer::
find(int a_gain, int a_shap, bool a_extPulse, bool a_extClock) {
  const string myname = "FembTestAnalyzer::find: ";
  DuneFembFinder fdr;
  cout << myname << "Fetching reader." << endl;
  m_reader = std::move(fdr.find(femb(), isCold(), tspat(), a_gain, a_shap, a_extPulse, a_extClock));
  cout << myname << "Done fetching reader." << endl;
  if ( m_reader == nullptr ) {
    m_gain = -1;
    m_shap = -1;
    m_extPulse = false;
    m_extClock = false;
    return 1;
  }
  m_gain = a_gain;
  m_shap = a_shap;
  m_extPulse = a_extPulse;
  m_extPulse = a_extClock;
  return 0;
}

//**********************************************************************
