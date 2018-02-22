// DuneFembReader.cxx

#include "DuneFembReader.h"
#include "dune/DuneInterface/AdcChannelData.h"
#include "TFile.h"
#include "TTree.h"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

namespace {
  using SIndex = DuneFembReader::Index;
  using Entry = DuneFembReader::Entry;
}

//**********************************************************************

DuneFembReader::DuneFembReader(string fname, int a_run, int a_subrun, string a_label)
: m_pfile(nullptr), m_ptree(nullptr),
  m_run(a_run), m_subrun(a_subrun), m_label(a_label),
  m_entry(badEntry()),
  m_event(badIndex()), m_chan(badIndex()), m_pwf(nullptr),
  m_nChan(0) {
  const string myname = "DuneFembReader::ctor: ";
  clearMetadata();
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
  m_ptree->SetBranchAddress("subrun", &m_event);
  m_ptree->SetBranchAddress("chan",   &m_chan);
  m_ptree->SetBranchAddress("wf",     &m_pwf);
  m_pwf = nullptr;
  if ( m_label.size() == 0 ) {
    string::size_type jpos = fname.rfind("/");
    if ( jpos != string::npos && jpos != 0 ) {
      string::size_type ipos = fname.rfind("/", jpos - 1);
      if ( ipos != string::npos && ipos != 0 ) {
        ipos = fname.rfind("/", ipos - 1);
        if ( ipos != string::npos ) {
          ++ipos;
          m_label = fname.substr(ipos, jpos-ipos);
        }
      }
    }
  }
  if ( m_label.size() == 0 ) m_label = fname;
  cout << myname << "Fetching channel counts." << endl;
  for ( SIndex ient=0; ient<m_ptree->GetEntries(); ++ient ) {
    read(ient);
    SIndex ievt = event();
    SIndex nevt = ievt + 1;
    SIndex ncha = channel() + 1;
    if ( nevt > m_nChanPerEvent.size() ) m_nChanPerEvent.resize(nevt, 0);
    if ( m_nChanPerEvent[ievt] <= ncha ) m_nChanPerEvent[ievt] = ncha;
    if ( ncha >  m_nChan ) m_nChan = ncha;
  }
  cout << myname << "Done fetching channel counts." << endl;
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
    if ( run() > 0 ) pacd->run = run();
    if ( subrun() > 0 ) pacd->subRun = subrun();
    pacd->event = event();
    pacd->channel = channel();
    pacd->raw.resize(m_pwf->size());
    std::copy(m_pwf->begin(), m_pwf->end(), pacd->raw.begin());
  }
  return 0;
}
  
//**********************************************************************

Entry DuneFembReader::
find(SIndex a_event, SIndex a_chan) {
  Entry nent = tree()->GetEntries();
  Entry ent0 = m_entry==badEntry() ? 0 : (m_entry)%nent;
  Entry ient = ent0;
  bool first = true;
  while ( first || ient != ent0 ) {
    first = false;
    if ( read(ient) ) break;
    if ( event() == a_event && channel() == a_chan ) {
      return m_entry = ient;
    }
    ient = (ient + 1)%nent;
  }
  m_entry  = badEntry();
  m_event = badIndex();
  m_chan   = badIndex();
  return m_entry;
}

//**********************************************************************

int DuneFembReader::
read(SIndex a_event, SIndex a_chan, AdcChannelData* pacd) {
  Entry ient = find(a_event, a_chan);
  return readWaveform(ient, pacd);
}

//**********************************************************************
