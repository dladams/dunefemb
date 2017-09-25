// DuneFembFinder

// David Adams
// August 2017
//
// Class to find DUNE FEMB test data and return a reader for a dataset
// specified by

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

  // Ctor from (partial) timestamp and gain params.
  explicit DuneFembFinder(std::string topdir ="/home/dladams/data/dune/femb");

  // Find a samples specified by timestamp.
  RdrPtr find(std::string ts, Index gain, Index shap, bool extPulse, bool extClock);

  // Find a sample specified by FEMB ID, temperature and partial timestamp.
  RdrPtr find(Index fembId, bool isCold, std::string ts,
              Index gain, Index shap, bool extPulse, bool extClock);

private:

  std::string m_topdir;
  FembMap m_warmFembMap;
  FembMap m_coldFembMap;

};
