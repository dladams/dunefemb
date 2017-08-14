// DuneFembReader.cxx

#include "DuneFembReader.h"
#include "dune/DuneInterface/AdcChannelData.h"
#include "TFile.h"
#include "TTree.h"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

using Index = DuneFembReader::Index;
using Entry = DuneFembReader::Entry;

//**********************************************************************

DuneFembReader::DuneFembReader(string fname)
: m_pfile(nullptr), m_ptree(nullptr),
  m_entry(badEntry()),
  m_subrun(badIndex()), m_chan(badIndex()), m_pwf(nullptr) {
  const string myname = "DuneFembReader::ctor: ";
  m_pfile = TFile::Open(fname.c_str(), "READ");
  if ( m_pfile == nullptr || ! m_pfile->IsOpen() ) {
    cout << myname << "Unable to open file " << fname << endl;
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
  m_pwf = nullptr;
}

//**********************************************************************

DuneFembReader::~DuneFembReader() {
  m_pfile->Close();
  delete m_pfile;
}

//**********************************************************************

int DuneFembReader::read(Entry ient) {
  if ( tree() == nullptr ) return 1;
  if ( ient == badEntry() ) return 2;
  m_entry = ient;
  tree()->SetBranchStatus("wf", false);
  m_pwf = nullptr;
  tree()->GetEntry(ient);
  return 0;
}
  
//**********************************************************************

int DuneFembReader::
readWaveform(Entry ient, AdcChannelData* pacd) {
  if ( tree() == nullptr ) return 1;
  if ( ient == badEntry() ) return 2;
  m_entry = ient;
  tree()->SetBranchStatus("wf", true);
  tree()->GetEntry(ient);
  if ( pacd != nullptr ) {
    pacd->channel = channel();
    pacd->raw.resize(m_pwf->size());
    std::copy(m_pwf->begin(), m_pwf->end(), pacd->raw.begin());
  }
  return 0;
}
  
//**********************************************************************

Entry DuneFembReader::
find(Index a_subrun, Index a_chan) {
  Entry nent = tree()->GetEntries();
  Entry ent0 = m_entry==badEntry() ? 0 : (m_entry)%nent;
  Entry ient = ent0;
  bool first = true;
  while ( first || ient != ent0 ) {
    first = false;
    if ( read(ient) ) break;
    if ( subrun() == a_subrun && channel() == a_chan ) {
      return m_entry = ient;
    }
    ient = (ient + 1)%nent;
  }
  m_entry  = badEntry();
  m_subrun = badIndex();
  m_chan   = badIndex();
  return m_entry;
}

//**********************************************************************

int DuneFembReader::
read(Index a_subrun, Index a_chan, AdcChannelData* pacd) {
  Entry ient = find(a_subrun, a_chan);
  return readWaveform(ient, pacd);
}

//**********************************************************************
