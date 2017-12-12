// FembTestPulseTree.h

// David Adams
// December 2017
//
// Class that builds a Root tree from FEMB test pulses.
// There is one entry for each pulse.

#ifndef FembTestPulseTree_H
#define FembTestPulseTree_H

#include <string>
#include "FembTestPulseData.h"

class TFile;
class TTree;

class FembTestPulseTree {

public:

  // Ctor from file name.
  //   fname - file name
  //   sopt - TFile option to open the file
  FembTestPulseTree(std::string fname, std::string sopt ="RECREATE");

  // Dtor. Closes file.
  ~FembTestPulseTree();

  // Delete copy and assignment.
  FembTestPulseTree(const FembTestPulseTree&) =delete;
  FembTestPulseTree& operator=(const FembTestPulseTree&) =delete;
  
  // Set all fields to default values.
  void clear();

  // Write the current values to the tree.
  void fill(bool doClear =true);

  // Fill from data.
  // Data are left with these values.
  void fill(const FembTestPulseData& data);

  // Getters.
  TFile* file() { return m_pfile; }
  TTree* tree() { return m_ptree; }
  const FembTestPulseData& data() const { return m_data; }

public:

  using Index = unsigned int;

  // Tree data.
  FembTestPulseData m_data;

private:

  TFile* m_pfile;
  TTree* m_ptree;

};

#endif
