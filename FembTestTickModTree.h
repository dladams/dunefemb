// FembTestTickModTree.h

// David Adams
// December 2017
//
// Class that builds a Root tree from FEMB test tickmods (tick%period).
// There is one entry for each pulse.

#ifndef FembTestTickModTree_H
#define FembTestTickModTree_H

#include <string>
#include "FembTestTickModData.h"
#include "TTree.h"

class AdcChannelData;
class TFile;

class FembTestTickModTree {

public:

  // Ctor from file name.
  //   fname - file name
  //   sopt - TFile option to open the file
  FembTestTickModTree(std::string fname, std::string sopt ="RECREATE");

  // Dtor. Closes file.
  ~FembTestTickModTree();

  // Delete copy and assignment.
  FembTestTickModTree(const FembTestTickModTree&) =delete;
  FembTestTickModTree& operator=(const FembTestTickModTree&) =delete;
  
  // Set all fields to default values.
  void clear();

  // Data access for writing.
  FembTestTickModData& data() { return (m_pdata == nullptr ? m_data : *m_pdata); }

  // Data access for reading.
  // First method sets entry and returns it. Second returns the current entry.
  // If copy is true, the data is copied to the write cache.
  const FembTestTickModData* read(unsigned int ient, bool copy =false);
  const FembTestTickModData* read() const;

  // Write the current values to the tree.
  void fill(bool doClear =true);

  // Fill from data.
  // Data are left with these values.
  void fill(const FembTestTickModData& data);

  // Fill from ADC channel data.
  // The period is taken from the current data and, if there is sufficient data,
  // one entry is made for each of tickmod = 0, ..., period-1.
  void fill(AdcChannelData& acd);

  // Write the tree to the file.
  void write();

  // Getters.
  TFile* file() { return m_pfile; }
  const TTree* tree() const { return m_ptree; }
  TTree* tree() { return m_ptree; }
  bool haveTree() const { return tree() != nullptr; }
  unsigned int size() const { return haveTree() ? m_ptree->GetEntries() : 0; }

public:

  using Index = unsigned int;

private:

  // Tree data.
  FembTestTickModData m_data;     // For write
  FembTestTickModData* m_pdata;   // For read.

  TFile* m_pfile;
  TTree* m_ptree;
  bool m_needWrite;

};

#endif
