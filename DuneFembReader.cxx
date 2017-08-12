// DuneFembReader.cxx

#include "DuneFembReader.h"
#include "dune/DuneInterface/AdcChannelData.h"
#include "TFile.h"
#include "TTree.h"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

UShort_t badVal = -1;

//**********************************************************************

DuneFembReader::DuneFembReader(string fname)
: m_pfile(nullptr), m_ptree(nullptr),
  m_entry(badEntry()),
  m_subrun(badVal()), m_chan(badVal()), m_pwf(nullptr) {
  const string myname = "DuneFembReader::ctor: ";
  m_pfile = TFile::Open(fname.c_str(), "READ");
  if ( m_pfile == nullptr || ! m_pfile->IsOpen() ) {
    cout << myname << "Unable to ope file " << fname << endl;
    if ( m_pfile != nullptr ) {
      delete m_pfile;
      m_pfile = nullptr;
    }
    return;
  }
  string tname = "femb_wfdata";
  m_ptree = dynamic_cast<TTree*>(m_pfile->Get(tname.c_str()));
  if ( m_ptree == nullptr ) {
    cout << myname << "Tree " << tname << " not found in file " << fname << endl;
    return;
  }
  m_ptree->SetBranchAddress("subrun", &m_subrun);
  m_ptree->SetBranchAddress("chan",   &m_chan);
  m_ptree->SetBranchAddress("wf",     &m_pwf);
}

//**********************************************************************

int DuneFembReader::
read(Long64_t ient, AdcChannelData* pacd) {
  if ( tree() == nullptr ) return m_entry = badEntry();
  Int_t nbyte = tree()->GetEntry(ient);
  if ( nbyte <= 0 ) {
    m_entry  = badEntry();
    m_subrun = badVal();
    m_chan   = badVal();
    m_pwf = nullptr;
    return 1;
  }
  m_entry = ient;
  if ( pacd != nullptr ) {
    pacd->channel = channel();
    pacd->raw.resize(m_pwf->size());
    std::copy(m_pwf->begin(), m_pwf->end(), pacd->raw.end());
  }
  return 0;
}
  
//**********************************************************************

int DuneFembReader::
read(UShort_t a_subrun, UShort_t a_chan, AdcChannelData* pacd) {
  Long64_t nent = tree()->GetEntries();
  Long64_t ent0 = m_entry==badEntry() ? 0 : (m_entry)%nent;
  Long64_t ient = ent0;
  bool first = true;
  while ( first || ient != ent0 ) {
    first = false;
    read(ient, nullptr);
    if ( subrun() == a_subrun && channel() == a_chan ) {
      if ( pacd != nullptr ) read(ient, pacd);
      return 0;
    }
    ient = (ient + 1)%nent;
  }
  m_entry  = badEntry();
  m_subrun = badVal();
  m_chan   = badVal();
  m_pwf = nullptr;
  return 1;
}

//**********************************************************************
