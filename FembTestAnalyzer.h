// FembTestAnalyzer.h

#ifndef FembTestAnalyzer_H
#define FembTestAnalyzer_H

#include <memory>
#include "DuneFembReader.h"

class FembTestAnalyzer {

public:

  using Index = DuneFembReader::Index;

  // Ctor from a FEMB sample set.
  FembTestAnalyzer(int a_femb, std::string a_tspat ="", bool a_isCold =true);

  // Ctor from a FEMB sample.
  FembTestAnalyzer(int a_femb, int gain, int shap,
                   std::string a_tspat ="", bool a_isCold =true,
                   bool extPulse =true, bool extClock =true);

  // Find a sample.
  // Returns 0 for success.
  int find(int gain, int shap, bool extPulse =true, bool extClock =true);

  // Return the reader for the current sample.
  DuneFembReader* reader() { return m_reader.get(); }

  // Return the parameters specifying the current sample.
  int femb() const { return m_femb; }
  std::string tspat() const { return m_tspat; }
  bool isCold() const { return m_isCold; }
  int gain() const { return m_gain; }
  int shap() const { return m_shap; }
  bool extPulse() const { return m_extPulse; }
  bool extClock() const { return m_extClock; }

  // Event and channel counts for the current sample.
  Index nEvent() const { return m_reader == nullptr ? 0 : m_reader->nEvent(); }
  Index nChannel(Index ievt) const { return m_reader == nullptr ? 0 : m_reader->nChannel(ievt); }

private:

  int m_femb;
  std::string m_tspat;
  bool m_isCold;
  int m_gain;
  int m_shap;
  bool m_extPulse;
  bool m_extClock;
  std::unique_ptr<DuneFembReader> m_reader;

};

#endif
