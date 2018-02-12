// FembTestTickModTree.h

// David Adams
// December 2017
//
// Class that builds a Root tree from FEMB test tickmods (tick%period).
// There is one entry for each pulse.
// There is one tree for each channel.

#ifndef FembTestTickModTree_H
#define FembTestTickModTree_H

#include <string>
#include "FembTestTickModData.h"
#include "dune/DuneInterface/Data/DataMap.h"
#include "TTree.h"
#include "TFile.h"

class AdcChannelData;

class FembTestTickModTree {

public:

  using Index = unsigned int;
  using TreeVector = std::vector<TTree*>;

  // Ctor from file name.
  //   fname - file name
  //   sopt - TFile option to open the file
  FembTestTickModTree(std::string fname, std::string sopt ="UPDATE");

  // Dtor. Closes file.
  ~FembTestTickModTree();

  // Delete copy and assignment.
  FembTestTickModTree(const FembTestTickModTree&) =delete;
  FembTestTickModTree& operator=(const FembTestTickModTree&) =delete;
  
  // Set data cache fields to default values.
  void clear();

  // Add the tree for channel icha.
  // Fails (returns null) if the file is not open or tree already exists.
  TTree* addTree(Index icha);

  // Get the tree for channel icha.
  // Fails (returns null) if the file is not open or tree is not present in the file.
  TTree* getTree(Index icha);

  // Data access for writing to the tree data cache.
  FembTestTickModData& data() { return (m_pdata == nullptr ? m_data : *m_pdata); }

  // Data access for reading.
  // First method sets entry and returns it. Second returns the current entry.
  // If copy is true, the data is copied to the write cache.
  const FembTestTickModData* read(Index icha, Index ient, bool copy =false);
  const FembTestTickModData* read() const;

  // Write the current values to the tree.
  void fill(bool doClear =true);

  // Fill from data.
  // Data are left with these values.
  void fill(const FembTestTickModData& data);

  // Fill from ADC channel data.
  // The period is taken from the current data and, if there is sufficient data,
  // one entry is made for each of tickmod = 0, ..., period-1.
  // The returned result contains the following:
  //     int tickmodPeriod    - Period: tickmod = tick%tickmodPeriod
  //     int tickmodMin       - Tickmod where signal average is minimum
  //     int tickmodMax       - Tickmod where signal average is maximum
  //   float tickmodMinSignal - Signal average at the minimum
  //   float tickmodMaxSignal - Signal average at the maximum
  DataMap fill(AdcChannelData& acd);

  // Write the trees to the file.
  void write();

  // Getters.
  bool haveFile() const { return m_pfile != nullptr && m_pfile->IsOpen(); }
  TFile* file() { return m_pfile; }
  bool haveTree(Index icha) const { return icha < m_trees.size() && m_trees[icha] != nullptr; }
  const TTree* tree(Index icha) const { return haveTree(icha) ? m_trees[icha] : nullptr; }
  TTree* tree(Index icha)             { return haveTree(icha) ? m_trees[icha] : nullptr; }
  TTree* treeWrite() const { return m_ptreeWrite; }
  Index chanWrite()  const { return m_chanWrite; }
  Index size() const { return m_trees.size(); }
  Index size(Index icha) const { return haveTree(icha) ? tree(icha)->GetEntries() : 0; }

private:

  // Tree data.
  FembTestTickModData m_data;     // For write
  FembTestTickModData* m_pdata;   // For read.

  TFile* m_pfile;
  TreeVector m_trees;
  bool m_needWrite;
  TTree* m_ptreeWrite;
  Index m_chanWrite;

};

#endif
