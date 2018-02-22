// FembTestTickModTree.cxx

#include "FembTestTickModTree.h"
#include "StickyCodeMetrics.h"
#include "TFile.h"
#include "TROOT.h"
#include "TDirectory.h"
#include "TKey.h"
#include "dune/DuneInterface/AdcChannelData.h"
#include <iostream>

using std::string;
using std::cout;
using std::endl;
using std::ostringstream;

//**********************************************************************

FembTestTickModTree::FembTestTickModTree(string fname, string sopt)
: m_pdata(nullptr), m_ptreeWrite(nullptr), m_chanWrite(99999) {
  const string myname = "FembTestTickModTree::ctor: ";
  TDirectory* pdirSave = gDirectory;
  cout << myname << "Opening " << fname << " in mode " << sopt << endl;
  m_pfile = TFile::Open(fname.c_str(), sopt.c_str());
  // Stop Root from deleting file before we close.
  gROOT->GetListOfFiles()->Remove(m_pfile);
  if ( m_pfile != nullptr && ! m_pfile->IsOpen() ) {
    delete m_pfile;
    m_pfile = nullptr;
  }
  clear();
  // Check for existing trees.
  if ( file() != nullptr ) {
    //TObjLink* plnk = file()->GetListOfKeys()->FirstLink();
    TIter ikey(file()->GetListOfKeys());
    string tnamePrefix = "TickModChan";
    Index lenPrefix = tnamePrefix.size();
    while ( true ) {
      TKey* pkey = dynamic_cast<TKey*>(ikey());
      if ( pkey == nullptr ) break;
      TTree* ptree = dynamic_cast<TTree*>(pkey->ReadObj());
      if ( ptree == nullptr ) continue;
      pkey->Print();
      string tname = ptree->GetName();
      if ( tname.substr(0, lenPrefix) == tnamePrefix ) {
        istringstream sscha(tname.substr(lenPrefix));
        Index icha;
        sscha >> icha;
        cout << myname << "Found tree for channel " << icha << endl;
      }
      //ikey.Next();
    }
    //TKey* pkey = dynamic_cast<TKey*>(ikey());
    //key->ReadObj()->Print();
  }
/*
  ostringstream sscha;
  if ( icha < 100 ) sscha << "0";
  if ( icha < 10 ) sscha << "0";
  sscha << icha;
  string tname = "TickModChan" + sscha.str();
*/  
  if ( pdirSave != nullptr ) pdirSave->cd();
}

//**********************************************************************

FembTestTickModTree::~FembTestTickModTree() {
  const string myname = "FembTestTickModTree::dtor: ";
  cout << myname << endl;
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
    m_ptreeWrite = nullptr;
    m_chanWrite = 99999;
    m_trees.clear();
    m_needWrite = false;
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

TTree* FembTestTickModTree::addTree(Index icha) {
  if ( haveTree(icha) ) return nullptr;
  if ( ! haveFile() ) return nullptr;
  if ( m_trees.size() < icha+1 ) {
    m_trees.resize(icha+1, nullptr);
  }
  m_ptreeWrite = nullptr;
  m_chanWrite = 99999;
  ostringstream sscha;
  if ( icha < 100 ) sscha << "0";
  if ( icha < 10 ) sscha << "0";
  sscha << icha;
  string tname = "TickModChan" + sscha.str();
  TDirectory* pdirSave = gDirectory;
  TObject* pobj = file()->Get(tname.c_str());
  if ( pobj != nullptr ) return nullptr;
  file()->cd();
  TTree* ptree = new TTree(tname.c_str(), "FEMB test pulses");
  ptree->Branch("data", &m_data);
  clear();
  m_trees[icha] = ptree;
  m_needWrite = true;
  if ( pdirSave != nullptr ) pdirSave->cd();
  m_ptreeWrite = ptree;
  m_chanWrite = icha;
  return ptree;
}

//**********************************************************************

TTree* FembTestTickModTree::getTree(Index icha) {
  m_ptreeWrite = nullptr;
  m_chanWrite = 99999;
  if ( ! haveFile() ) return nullptr;
  ostringstream sscha;
  TTree* ptree = tree(icha);
  TDirectory* pdirSave = gDirectory;
  if ( ptree == nullptr ) {
    if ( icha < 100 ) sscha << "0";
    if ( icha < 10 ) sscha << "0";
    sscha << icha;
    string tname = "TickModChan" + sscha.str();
    TObject* pobj = file()->Get(tname.c_str());
    if ( pobj == nullptr ) return nullptr;
    ptree = dynamic_cast<TTree*>(pobj);
    if ( ptree == nullptr ) return nullptr;
  }
  ptree->SetBranchAddress("data", &m_pdata);
  ptree->GetEntry(0);
  if ( m_trees.size() < icha+1 ) {
    m_trees.resize(icha+1, nullptr);
  }
  if ( m_trees[icha] == nullptr ) m_trees[icha] = ptree;
  if ( pdirSave != nullptr ) pdirSave->cd();
  return ptree;
}

//**********************************************************************

// For now, we only fill the current tree.

void FembTestTickModTree::fill(bool doClear) {
  if ( treeWrite() != nullptr && chanWrite() == m_data.chan ) treeWrite()->Fill();
  if ( doClear ) clear();
  m_needWrite = true;
}

//**********************************************************************

void FembTestTickModTree::fill(const FembTestTickModData& data) {
  m_data = data;
  fill(false);
}

//**********************************************************************

DataMap FembTestTickModTree::fill(AdcChannelData& acd) {
  // Save the data to restore after filling.
  FembTestTickModData dataSave = m_data;
  Index ntck = acd.samples.size();
  Index nraw = acd.raw.size();
  Index ntmd = data().ntmd;
  int itqmin = -1;
  int itqmax = -1;
  double qmin = 1.e10;
  double qmax = -1.e10;
  int chan = acd.channel;
  // Create vector of tree entries for this channel data.
  vector<FembTestTickModData> ents(ntmd, m_data);
  // Fill add data except peak positions for each tickmod.
  for ( Index itmd=0; itmd<ntmd; ++itmd ) {
    FembTestTickModData& data = ents[itmd];
    data.chan = chan;
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
  // Construct result.
  DataMap res;
  res.setInt("tickmodChannel", chan);
  res.setInt("tickmodPeriod", ntmd);
  res.setInt("tickmodMin", itqmin);
  res.setInt("tickmodMax", itqmax);
  res.setFloat("tickmodMinSignal", qmin);
  res.setFloat("tickmodMaxSignal", qmax);
  return res;
}

//**********************************************************************

void FembTestTickModTree::write() {
  const string myname = "FembTestTickModTree::writing: ";
  TDirectory* pdirSave = gDirectory;
  cout << myname << "Before purge: " << endl;
  file()->ls();
  file()->Purge();
  cout << myname << "After purge: " << endl;
  file()->ls();
  file()->Write();
  if ( pdirSave != nullptr ) pdirSave->cd();
  m_needWrite = false;
}

//**********************************************************************

const FembTestTickModData*
FembTestTickModTree::read(Index icha, Index ient, bool copy) {
  const string myname = "FembTestTickModTree::data: ";
  //getTree(icha);       // This makes things a lot slower
  //m_pdata = nullptr;
  if ( haveTree(icha) ) {
    if ( ient < size(icha) ) {
      tree(icha)->GetEntry(ient);
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
