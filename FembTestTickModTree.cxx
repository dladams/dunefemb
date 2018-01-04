// FembTestTickModTree.cxx

#include "FembTestTickModTree.h"
#include "StickyCodeMetrics.h"
#include "TFile.h"
#include "TROOT.h"
#include "TDirectory.h"
#include "dune/DuneInterface/AdcChannelData.h"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

//**********************************************************************

FembTestTickModTree::FembTestTickModTree(string fname, string sopt)
: m_pdata(nullptr) {
  m_needWrite = false;
  m_ptree = nullptr;
  TDirectory* pdirSave = gDirectory;
  m_pfile = TFile::Open(fname.c_str(), sopt.c_str());
  // Stop Root from deleting file before we close.
  gROOT->GetListOfFiles()->Remove(m_pfile);
  if ( m_pfile != nullptr && ! m_pfile->IsOpen() ) {
    delete m_pfile;
    m_pfile = nullptr;
  }
  if ( file() != nullptr ) {
    string tname = "FembTestTickMod";
    TObject* pobj = file()->Get(tname.c_str());
    if ( pobj != nullptr ) {
      m_ptree = dynamic_cast<TTree*>(pobj);
      m_ptree->SetBranchAddress("data", &m_pdata);
      m_ptree->GetEntry(0);
    } else {
      file()->cd();
      m_ptree = new TTree(tname.c_str(), "FEMB test pulses");
      m_ptree->Branch("data", &m_data);
    }
  }
  clear();
  if ( pdirSave != nullptr ) pdirSave->cd();
}

//**********************************************************************

FembTestTickModTree::~FembTestTickModTree() {
  const string myname = "FembTestTickModTree::dtor: ";
  if ( file() != nullptr ) {
    TDirectory* pdirSave = gDirectory;
    if ( pdirSave == file() ) pdirSave = nullptr;
    // Root may close the file before this object is destroyed.
    if ( file() != nullptr && file()->IsOpen() ) {
      if ( m_needWrite ) write();
      file()->Close();
      delete m_pfile;
    } else if ( m_needWrite ) {
      cout << myname << "WARNING: File closed before the complete tree was written." << endl;
    }
    m_pfile = nullptr;
    m_ptree = nullptr;
    if ( pdirSave != nullptr ) pdirSave->cd();
  }
}

//**********************************************************************

void FembTestTickModTree::clear() {
  m_data.femb = 999;
  m_data.gain = 999;
  m_data.shap = 999;
  m_data.extp = false;
  m_data.ntmd = 0;
  m_data.chan = 999;
  m_data.ped0 = 0.0;
  m_data.qexp = 0.0;
  m_data.ievt = -1;
  m_data.itmx = -1;
  m_data.itmn = -1;
  m_data.pede = 0.0;
  m_data.itmd = -1;
  m_data.cmea = 0.0;
  m_data.crms = 0.0;
  m_data.sadc = -1.0;
  m_data.adcm = -1.0;
  m_data.adcn = -1.0;
  m_data.efft = -1.0;
  m_data.sfmx = -2.0;
  m_data.sf00 = -1.0;
  m_data.sf01 = -1.0;
  m_data.sf63 = -1.0;
  m_data.nsat = -1;
  m_data.qcal.clear();
  m_data.radc.clear();
}

//**********************************************************************

void FembTestTickModTree::fill(bool doClear) {
  if ( tree() != nullptr ) tree()->Fill();
  if ( doClear ) clear();
  m_needWrite = true;
}

//**********************************************************************

void FembTestTickModTree::fill(const FembTestTickModData& data) {
  m_data = data;
  fill(false);
}

//**********************************************************************

void FembTestTickModTree::fill(AdcChannelData& acd) {
  // Save the data to restore after filling.
  FembTestTickModData dataSave = m_data;
  Index ntck = acd.samples.size();
  Index nraw = acd.raw.size();
  Index ntmd = data().ntmd;
  int itqmin = -1;
  int itqmax = -1;
  double qmin = 1.e10;
  double qmax = -1.e10;
  // Create vector of tree entries for this channel data.
  vector<FembTestTickModData> ents(ntmd, m_data);
  // Fill add data except peak positions for each tickmod.
  for ( Index itmd=0; itmd<ntmd; ++itmd ) {
    FembTestTickModData& data = ents[itmd];
    data.chan = acd.channel;
    data.pede = acd.pedestal;
    data.radc.clear();
    data.qcal.clear();
    data.itmd = itmd;
    data.nsat = 0;
    int qcnt = 0;
    double qsum = 0.0;
    double qqsum = 0.0;
    AdcCountVector adcs;
    for ( Index itck=itmd; itck<ntck; itck+=ntmd ) {
      short radc = itck<nraw ? acd.raw[itck] : -1;
      data.radc.push_back(radc);
      float qcal = acd.samples[itck];
      AdcFlag flag = acd.flags[itck];
      AdcCount adc = acd.raw[itck];
      ++qcnt;
      qsum += qcal;
      qqsum += qcal*qcal;
      data.qcal.push_back(qcal);
      if ( flag == AdcUnderflow || flag==AdcOverflow ) ++data.nsat;
      adcs.push_back(adc);
    }
    double qmea = qsum/qcnt;
    double qmea2 = qmea*qmea;
    double qqmea = qqsum/qcnt;
    double qrms = qqmea > qmea2 ? sqrt(qqmea - qmea2) : 0.0;
    data.cmea = qmea;
    data.crms = qrms;
    StickyCodeMetrics sm(adcs);
    data.sadc = sm.maxAdc();
    data.adcm = sm.meanAdc();
    data.adcn = sm.meanAdc2();
    data.sfmx = sm.maxFraction();
    data.sf00 = sm.zeroFraction();
    data.sf01 = sm.oneFraction();
    data.sf63 = sm.highFraction();
    if ( qmea < qmin ) {
      itqmin = itmd;
      qmin = qmea;
    }
    if ( qmea > qmax ) {
      itqmax = itmd;
      qmax = qmea;
    }
    int ngood = 0;
    float efflim = 2.5;
    for ( float qcal : data.qcal ) {
      if ( fabs(qcal - qmea) < efflim ) ++ngood;
    }
    data.efft = qcnt > 0 ? float(ngood)/qcnt : 0.0;
  }
  // Add the peak positions and fill tree.
  for ( FembTestTickModData& data : ents ) {
    data.itmn = itqmin;
    data.itmx = itqmax;
    fill(data);
  }
  // Restore data to original state.
  m_data = dataSave;
}

//**********************************************************************

void FembTestTickModTree::write() {
  TDirectory* pdirSave = gDirectory;
  file()->Write();
  file()->Purge();
  if ( pdirSave != nullptr ) pdirSave->cd();
  m_needWrite = false;
}

//**********************************************************************

const FembTestTickModData*
FembTestTickModTree::read(unsigned int ient, bool copy) {
  const string myname = "FembTestTickModTree::data: ";
  //m_pdata = nullptr;
  if ( haveTree() ) {
    if ( ient < size() ) {
      tree()->GetEntry(ient);
      if ( copy ) m_data = *m_pdata;
    } else {
      cout << myname << "ERROR: Tree does not have entry " << ient << endl;
    }
  } else {
    cout << myname << "ERROR: Tree not found." << endl;
  }
  return m_pdata;
}

//**********************************************************************

const FembTestTickModData*
FembTestTickModTree::read() const {
  return m_pdata;
}

//**********************************************************************
