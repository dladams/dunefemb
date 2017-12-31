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
/*
    m_ptree->Branch("femb", &(m_data.femb), "femb/i");
    m_ptree->Branch("gain", &(m_data.gain), "gain/i");
    m_ptree->Branch("shap", &(m_data.shap), "shap/i");
    m_ptree->Branch("extp", &(m_data.extp), "extp/O");
    m_ptree->Branch("ntmd", &(m_data.ntmd));
    m_ptree->Branch("chan", &(m_data.chan), "chan/i");
    m_ptree->Branch("ped0", &(m_data.ped0));
    m_ptree->Branch("qexp", &(m_data.qexp));
    m_ptree->Branch("ievt", &(m_data.ievt));
    m_ptree->Branch("itmx", &(m_data.itmx));
    m_ptree->Branch("itmn", &(m_data.itmn));
    m_ptree->Branch("pede", &(m_data.pede));
    m_ptree->Branch("itmd", &(m_data.itmd));
    m_ptree->Branch("cmea", &(m_data.cmea), "cmea/F");
    m_ptree->Branch("crms", &(m_data.crms), "crms/F");
    m_ptree->Branch("sadc", &(m_data.sadc));
    m_ptree->Branch("adcm", &(m_data.adcm));
    m_ptree->Branch("adcn", &(m_data.adcn));
    m_ptree->Branch("efft", &(m_data.efft));
    m_ptree->Branch("stk1", &(m_data.stk1), "stk1/F");
    m_ptree->Branch("stk2", &(m_data.stk2), "stk2/F");
    m_ptree->Branch("nsat", &(m_data.nsat));
    m_ptree->Branch("radc", &(m_data.radc));
    m_ptree->Branch("qcal", &(m_data.qcal));
*/
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
    if ( file()->IsOpen() ) {
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
  m_data.stk1 = -2.0;
  m_data.stk2 = -1.0;
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
      if ( flag == AdcUnderflow || flag==AdcOverflow ) ++m_data.nsat;
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
    data.stk1 = sm.maxFraction();
    data.stk2 = sm.classicFraction();
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
  if ( pdirSave != nullptr ) pdirSave->cd();
  m_needWrite = false;
}

//**********************************************************************

const FembTestTickModData*
FembTestTickModTree::read(unsigned int ient, bool copy) {
  const string myname = "FembTestTickModTree::data: ";
  m_pdata = nullptr;
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
