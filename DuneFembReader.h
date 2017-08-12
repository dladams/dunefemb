// DuneFembReader.h
//
// David Adams
// August 2017
//
// Class that reads a waveform from a DUNE FEMB gain test file
// into AdcChannelData. These files hold waveforms for multiple
// subruns and multiple channels.

#ifndef DuneFembReader_H
#define DuneFembReader_H

#include "dune/DuneInterface/AdcTypes.h"

class AdcChannelData;
class TFile;
class TTree;

class DuneFembReader {

public:

  // Bad subrun or channel.
  static UShort_t badVal() { return UShort_t(-1); }

  // Bad entry.
  static Long64_t badEntry() { return -1; }

public:

  // Ctor from a file.
  DuneFembReader(std::string fname);

  // Read data for one entry (waveform) in the tree.
  // If pacd is not null, the channel and waveform are copied to it.
  int read(Long64_t ient, AdcChannelData* pacd);

  // Read data for one subrun and channel.
  // If pacd is not null, the channel and waveform are copied to it.
  int read(UShort_t subrun, UShort_t, AdcChannelData* pacd);

  // Return the index data for the current entry.
  UShort_t subrun() const { return m_subrun; }
  UShort_t channel() { return m_chan; }
  
  // Return the file.
  TFile* file() const { return m_pfile; }

  // Return the tree.
  TTree* tree() const { return m_ptree; }

private:

  TFile* m_pfile;
  TTree* m_ptree;
  Long64_t m_entry;
  UShort_t m_subrun;
  UShort_t m_chan;
  std::vector<unsigned short>* m_pwf;

};

#endif
