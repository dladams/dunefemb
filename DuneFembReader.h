// DuneFembReader.h
//
// David Adams
// August 2017
//
// Class that reads a waveform from a DUNE FEMB gain test file
// into AdcChannelData. These files hold waveforms for multiple
// events (aka subruns) and multiple channels.
//
// Note that the field "subrun" in the tree is used to assing the event
// number here.

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

  // Bad event/subrun or channel.
  static Index badIndex() { return Index(-1); }

  // Bad entry.
  static Entry badEntry() { return -1; }

public:

  // Ctor from a file.
  // If either the run or subrun is non-negative, then it is used to
  // set the correponding field in any filled channel data.
  DuneFembReader(std::string fname, int run =-1, int subrun =-1, std::string a_label ="");

  // Dtor.
  ~DuneFembReader();

  // Set the label.
  void setLabel(std::string a_label) { m_label = a_label; }

  // Read the event/subrun and channel for one entry (waveform) in the tree.
  int read(Long64_t ient);

  // Read the event/subrun, channel and waveform for one entry (waveform) in the tree.
  // If pacd is not null, the channel and waveform are copied to it.
  int readWaveform(Long64_t ient, AdcChannelData* pacd);

  // Find the entry for an event/subrun and channel.
  Entry find(Index event, Index chan);

  // Read data for one event and channel.
  // If pacd is not null, the channel and waveform are copied to it.
  int read(Index event, Index channel, AdcChannelData* pacd);

  // Return the index data for the current entry.
  int run() const { return m_run; }
  int subrun() const { return m_subrun; }
  std::string label() const { return m_label; }
  Entry entry() const { return m_entry; }
  Index event() const { return m_event; }
  Index channel() const { return m_chan; }
  const Waveform* waveform() const { return m_pwf; }
  
  // Return the file.
  TFile* file() const { return m_pfile; }

  // Return the tree.
  TTree* tree() const { return m_ptree; }

  // Return the maximum event number plus one in the tree.
  Index nEvent() const { return m_nChanPerEvent.size(); }
  Index nChannel() const { return m_nChan; }

  // Return the maximum channel number plus one for a given event.
  Index nChannel(Index ievt) const { return ievt < nEvent() ? m_nChanPerEvent[ievt] : 0; }

  // Metadata.
  void clearMetadata() {
    m_gainIndex = 99;
    m_shapingIndex = 99;
    m_extPulse = 99;
    m_extClock = 99;
  }
  void setMetadata(Index a_gainIndex, Index a_shapingIndex, bool a_extPulse, bool a_extClock) {
    m_gainIndex = a_gainIndex;
    m_shapingIndex = a_shapingIndex;
    m_extPulse = a_extPulse;
    m_extClock = a_extClock;
  }
  Index gainIndex() const { return m_gainIndex; }
  Index shapingIndex() const { return m_shapingIndex; }
  bool extPulse() const { return m_extPulse == 1; }
  bool intPulse() const { return m_extPulse == 0; }
  bool extClock() const { return m_extClock == 1; }
  bool intClock() const { return m_extClock == 0; }

private:

  std::string m_fileName;
  TFile* m_pfile;
  TTree* m_ptree;
  int m_run;
  int m_subrun;
  std::string m_label;
  Entry m_entry;
  Index m_event;
  Index m_chan;
  Waveform* m_pwf;
  Index m_nChan;
  vector<Index> m_nChanPerEvent;

  // Metadata.
  Index m_gainIndex;
  Index m_shapingIndex;
  Index m_extPulse;   // 0 for internal, 1 for external
  Index m_extClock;   // 0 for internal, 1 for external

};

#endif
