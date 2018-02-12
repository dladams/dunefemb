// DuneFembReport.h

// Class to create a report for a FEMB.

#ifndef DuneFembReport_H
#define DuneFembReport_H

#include <string>
#include <memory>
#include "FembTestAnalyzer.h"

class DuneFembReport {

public:

  using Index = unsigned int;
  using FtaPtr = std::unique_ptr<FembTestAnalyzer>;

  // Ctor.
  DuneFembReport(Index ifmb, Index igai, Index ishp, std::string spat);

  // Getters.
  Index femb() const { return m_ifmb; }
  Index gain() const { return m_igai; }
  Index shap() const { return m_ishp; }

  // Analyzer that uses raw to do height calibration.
  FembTestAnalyzer* ftaRaw();

  // Analyzer that applies height calibratiion to raw.
  // It does anotehr height calibration an draws rsiduals.
  FembTestAnalyzer* ftaHeightCalib();

  // Do the raw height calibration.
  int doRawHeightCalibration();

  // Check the height calibration.
  int checkRawHeightCalibration();

private:

  FtaPtr m_pftaRaw;
  FtaPtr m_pftaHeightCalib;
  Index m_ifmb;
  Index m_igai;
  Index m_ishp;
  std::string m_spat;

};

#endif
