// FembTestAnalyzer.h

#ifndef FembTestAnalyzer_H
#define FembTestAnalyzer_H

#include <memory>
#include "dune/DuneInterface/Data/DataMap.h"
#include "dune/DuneInterface/Tool/AdcChannelDataModifier.h"
#include "dune/DuneInterface/Tool/AdcChannelViewer.h"
#include "dune/DuneCommon/TPadManipulator.h"
#include "DuneFembReader.h"

class FembTestAnalyzer {

public:

  using Index = DuneFembReader::Index;
  using ManMap = std::map<std::string, TPadManipulator>;

  // Ctor from a FEMB sample set.
  // opt = 100*doDraw + option is the processing option:
  // option:
  //   0 - no calibration
  //   1 - calibration with fembCalibrator
  FembTestAnalyzer(int opt, int a_femb, std::string a_tspat ="", bool a_isCold =true);

  // Ctor from file dir and pattern.
  FembTestAnalyzer(int opt, std::string dir, std::string fpat,
                   int a_femb, int a_gain, int a_shap,
                   bool a_isCold =true, bool a_extPulse =true, bool a_extClock =true);

  // Ctor from a FEMB sample by params.
  FembTestAnalyzer(int opt, int a_femb, int gain, int shap,
                   std::string a_tspat ="", bool a_isCold =true,
                   bool extPulse =true, bool extClock =true);

  // Find a sample.
  // If dir.size(), then the sample file is file pattern tspat in that directory.
  // Otherwise femb, gain, shap, isCold and extPulse are used.
  // Returns 0 for success.
  int find(int gain, int shap, bool extPulse =false, bool extClock =true, std::string dir="");

  // Return the reader for the current sample.
  DuneFembReader* reader() const { return m_reader.get(); }

  // Return the parameters specifying the current sample.
  int option() const { return m_opt; }
  bool doDraw() const { return m_doDraw; }
  int femb() const { return m_femb; }
  std::string tspat() const { return m_tspat; }
  bool isCold() const { return m_isCold; }
  int gainIndex() const { return reader()==nullptr ? -1 : reader()->gainIndex(); }
  int shapingIndex() const { return reader()==nullptr ? -1 : reader()->shapingIndex(); }
  bool extPulse() const { return m_extPulse; }
  bool extClock() const { return m_extClock; }

  // Setters.
  bool setDoDraw(bool val) { return m_doDraw = val; }

  // Event and channel counts for the current sample.
  Index nEvent() const { return m_reader == nullptr ? 0 : m_reader->nEvent(); }
  Index nChannel() const { return m_reader == nullptr ? 0 : m_reader->nChannel(); }
  Index nChannel(Index ievt) const { return m_reader == nullptr ? 0 : m_reader->nChannel(ievt); }

  // Some constants.
  double elecPerFc() const { return 6241.51; }
  double fembCfF() const { return 183.0; }
  double fembVmV() const { return 18.75; }
  double preampGain(int igain) const { return igain==0 ?  4.7 :
                                              igain==1 ?  7.8 :
                                              igain==2 ? 14.0 :
                                              igain==3 ? 25.0 : 0.0; }  // mV/fC
  double preampGain() const { return preampGain(gainIndex()); }
  double shapingTime(int ishap) const { return ishap==0 ? 0.5 :
                                               ishap< 4 ? ishap : 0.0; }  // us
  double shapingTime() const { return shapingTime(shapingIndex()); }
  double adcGain() const { return 3.0; }  // Nominal ADC gain [ADC/mV]

  // Charge [fC] for each event (= DAC setting).
  // For external pulses, the table from Brian is used.
  // Otherwise, a step size of Q = CV = fembCfF()*fembVmV() is assumed.
  double chargeFc(Index ievt);

  // Return a graph of signal vs. input charge.
  // Error bars are the RMS of each measurement (not the RMS of the mean).
  const DataMap& processChannelEvent(Index icha, Index ievt);

  // Process a channel.
  DataMap getChannelResponse(Index icha, std::string usePosFlag, bool useArea =true);
  const DataMap& processChannel(Index icha);

  // Process all channels.
  const DataMap& processAll();

  // Write calibration info to FCL.
  int writeCalibFcl();

  // Return a single plot.
  // Use sopt = "help" to display the supported plots
  TPadManipulator* draw(std::string sopt ="help", int icha =-1, int ievt =-1);

  // Return a plot of all 16 channels in an ADC.
  // Use sopt = "help" for supported plots
  TPadManipulator* drawAdc(std::string sopt, int iadc =-1, int ievt =-1);

  // Pass-through for TPadManipulator that draws it if m_doDraw is true.
  TPadManipulator* draw(TPadManipulator* pman) const;

public:

  int dbg = 0;
  std::vector<std::vector<DataMap>> chanevtResults;    // results[icha][ievt]
  std::vector<DataMap> chanResults;    // results[icha][ievt]
  DataMap allResult;

private:

  int m_opt;
  bool m_doDraw;
  int m_femb;
  std::string m_tspat;
  bool m_isCold;
  int m_gain;
  int m_shap;
  bool m_extPulse;
  bool m_extClock;
  std::unique_ptr<DuneFembReader> m_reader;
  std::vector<std::unique_ptr<AdcChannelDataModifier>> adcModifiers;
  std::vector<std::unique_ptr<AdcChannelViewer>> adcViewers;
  ManMap m_mans;

};

#endif
