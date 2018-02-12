// FembTestPulseTree.cxx

#include "FembTestPulseTree.h"

#include "TFile.h"
#include "TTree.h"
#include "TDirectory.h"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

//**********************************************************************

FembTestPulseTree::FembTestPulseTree(string fname, string sopt) {
  m_needWrite = false;
  m_ptree = nullptr;
  m_pfile = TFile::Open(fname.c_str(), sopt.c_str());
  if ( m_pfile != nullptr && ! m_pfile->IsOpen() ) {
    delete m_pfile;
    m_pfile = nullptr;
  }
  if ( file() != nullptr ) {
    TDirectory* pdirSave = gDirectory;
    file()->cd();
    m_ptree = new TTree("FembTestPulse", "FEMB test pulses");
    m_ptree->Branch("femb", &(m_data.femb), "femb/i");
    m_ptree->Branch("gain", &(m_data.gain), "gain/i");
    m_ptree->Branch("shap", &(m_data.shap), "shap/i");
    m_ptree->Branch("extp", &(m_data.extp), "extp/O");
    m_ptree->Branch("chan", &(m_data.chan), "chan/i");
    m_ptree->Branch("ped0", &(m_data.ped0));
    m_ptree->Branch("qexp", &(m_data.qexp));
    m_ptree->Branch("sevt", &(m_data.sevt));
    m_ptree->Branch("pede", &(m_data.pede));
    m_ptree->Branch("cmea", &(m_data.cmea), "cmea/F");
    m_ptree->Branch("crms", &(m_data.crms), "crms/F");
    m_ptree->Branch("cdev", &(m_data.cdev));
    m_ptree->Branch("stk1", &(m_data.stk1), "stk1/F");
    m_ptree->Branch("stk2", &(m_data.stk2), "stk2/F");
    m_ptree->Branch("nsat", &(m_data.nsat));
    m_ptree->Branch("qcal", &(m_data.qcal));
    pdirSave->cd();
  }
  clear();
}

//**********************************************************************

FembTestPulseTree::~FembTestPulseTree() {
  const string myname = "FembTestPulseTree::dtor: ";
  if ( file() != nullptr ) {
    TDirectory* pdirSave = gDirectory;
    if ( pdirSave == file() ) pdirSave = nullptr;
    // Root may close the file before this object is destroyed.
    if ( file()->IsOpen() ) {
      write();
      file()->cd();
      file()->Write();
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

void FembTestPulseTree::clear() {
  m_data.femb = 999;
  m_data.gain = 999;
  m_data.shap = 999;
  m_data.extp = false;
  m_data.chan = 999;
  m_data.qexp = 0.0;
  m_data.sevt = 0;
  m_data.nsat = -1;
  m_data.cmea = 0.0;
  m_data.crms = 0.0;
  m_data.stk1 = -1.0;
  m_data.stk2 = -1.0;
  m_data.qcal.clear();
}

//**********************************************************************

void FembTestPulseTree::fill(bool doClear) {
  if ( tree() != nullptr ) tree()->Fill();
  if ( doClear ) clear();
  m_needWrite = true;
}

//**********************************************************************

void FembTestPulseTree::write() {
  TDirectory* pdirSave = gDirectory;
  file()->Write();
  if ( pdirSave != nullptr ) pdirSave->cd();
  m_needWrite = false;
}

//**********************************************************************

void FembTestPulseTree::fill(const FembTestPulseData& data) {
  m_data = data;
  fill(false);
}

//**********************************************************************
