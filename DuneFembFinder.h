// DuneFembFinder.h

// David Adams
// August 2017
//
// Class to find DUNE FEMB test data and return a reader for a dataset
// specified by

#ifndef DuneFembFinder_H
#define DuneFembFinder_H

#include<string>
#include <vector>
#include <map>
#include <memory>

class DuneFembReader;

class DuneFembFinder {

public:

  using Name = std::string;
  using NameVector = std::vector<Name>;
  using Index = unsigned int;
  using FembMap = std::map<Index, NameVector>;
  using RdrPtr = std::unique_ptr<DuneFembReader>;

  // Ctor from topdir (where data is stored).
  explicit DuneFembFinder(std::string a_topdir ="~/data/dune/femb");

  // Find a sample specified by directory and file pattern in topdir.
  RdrPtr find(std::string dir, std::string fpat ="");

  // Find a sample specified by timestamp and run params.
  // Search is in topdir/wib*.
  RdrPtr find(std::string ts, Index gain, Index shap, bool extPulse, bool extClock);

  // Find a sample specified by FEMB ID, temperature and partial timestamp.
  // Timestamp is found using topdir/fembjson.dat.
  RdrPtr find(Index fembId, bool isCold, std::string ts,
              Index gain, Index shap, bool extPulse, bool extClock);

  // Getters.
  string topdir() const { return m_topdir; }

private:

  std::string m_topdir;
  FembMap m_warmFembMap;
  FembMap m_coldFembMap;

};

#endif
