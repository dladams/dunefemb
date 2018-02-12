// DuneFembReport.cxx

#include "DuneFembReport.h"

//**********************************************************************

DuneFembReport::DuneFembReport(Index ifmb, Index igai, Index ishp, string spat)
: m_ifmb(ifmb), m_igai(igai), m_ishp(ishp), m_spat(spat) { }

//**********************************************************************

FembTestAnalyzer* DuneFembReport::ftaRaw() {
  if ( ! m_pftaRaw ) {
    m_pftaRaw.reset(new FembTestAnalyzer(10, femb(), gain(), shap()));
  }
  return m_pftaRaw.get();
};

//**********************************************************************

FembTestAnalyzer* DuneFembReport::ftaHeightCalib() {
  if ( ! m_pftaHeightCalib ) {
    m_pftaHeightCalib.reset(new FembTestAnalyzer(11, femb(), gain(), shap()));
  }
  return m_pftaHeightCalib.get();
};

//**********************************************************************

int DuneFembReport::doRawHeightCalibration() {
  FembTestAnalyzer* pfta = ftaRaw();
  if ( pfta == nullptr ) return 1;
  DataMap res = pfta->processAll();
  if ( res.status() ) return 2;
  // Femb plots.
  vector<string> names = {"pedlim", "gainh", "gaina", "csddh", "csdch"};
  for ( string name : names ) {
    string fname = "rawf_" + name + ".png";
    TPadManipulator* pman = pfta->draw(name);
    pman->print(fname);
  }
  pfta->writeCalibFcl();
  // ADC plots for all events.
  vector<string> namesAdc = {"ped", "resph", "respresh"};
  for ( string name : namesAdc ) {
    for ( int iadc=0; iadc<8; ++iadc ) {
      ostringstream ssnum;
      ssnum << iadc;
      string sadc = ssnum.str();
      string fname = "rawa_" + name + "_adc" + sadc + ".png";
      TPadManipulator* pman = pfta->drawAdc(name, iadc);
      pman->print(fname);
    }
  }
  // ADC plots for event 0.
  int ievt = 0;
  vector<string> namesAdcEv0 = {"ped"};
  for ( string name : namesAdcEv0 ) {
    for ( int iadc=0; iadc<8; ++iadc ) {
      ostringstream ssnum;
      ssnum << iadc;
      string sadc = ssnum.str();
      string fname = "rawev0_" + name + "_adc" + sadc + ".png";
      TPadManipulator* pman = pfta->drawAdc(name, iadc, ievt);
      pman->print(fname);
    }
  }
  return 0;
}

//**********************************************************************

int DuneFembReport::checkRawHeightCalibration() {
  FembTestAnalyzer* pfta = ftaHeightCalib();
  if ( pfta == nullptr ) return 1;
  DataMap res = pfta->processAll();
  if ( res.status() ) return 2;
  vector<string> names = {"gainh", "gaina", "rmsh", "dev"};
  for ( string name : names ) {
    string fname = "cal_" + name + ".png";
    TPadManipulator* pman = pfta->draw(name);
    pman->print(fname);
  }
  return 0;
}

//**********************************************************************
