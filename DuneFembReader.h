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

  using Index = UShort_t;
  using Entry = Long64_t;
  using Waveform = std::vector<unsigned short>;

  // Bad subrun or channel.
  static Index badIndex() { return Index(-1); }

  // Bad entry.
  static Entry badEntry() { return -1; }

public:

  // Ctor from a file.
  DuneFembReader(std::string fname);

  // Dtor.
  ~DuneFembReader();

  // Read the subrun and channel for one entry (waveform) in the tree.
  int read(Long64_t ient);

  // Read the subrun, channel and waveform for one entry (waveform) in the tree.
  // If pacd is not null, the channel and waveform are copied to it.
  int readWaveform(Long64_t ient, AdcChannelData* pacd);

  // Find the entry for a subrun and channel.
  // Also sets the subrun and channel.
  Entry find(Index subrun, Index chan);

  // Read data for one subrun and channel.
  // If pacd is not null, the channel and waveform are copied to it.
  int read(Index subrun, Index, AdcChannelData* pacd);

  // Return the index data for the current entry.
  Entry entry() const { return m_entry; }
  Index subrun() const { return m_subrun; }
  Index channel() const { return m_chan; }
  const Waveform* waveform() const { return m_pwf; }
  
  // Return the file.
  TFile* file() const { return m_pfile; }

  // Return the tree.
  TTree* tree() const { return m_ptree; }

private:

  TFile* m_pfile;
  TTree* m_ptree;
  Entry m_entry;
  Index m_subrun;
  Index m_chan;
  Waveform* m_pwf;

};

#endif
