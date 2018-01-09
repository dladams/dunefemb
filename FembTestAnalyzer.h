// FembTestAnalyzer.h

#ifndef FembTestAnalyzer_H
#define FembTestAnalyzer_H

#include "DuneFembReader.h"
#include "FembTestPulseTree.h"
#include "FembTestTickModTree.h"
#include "dune/DuneInterface/Data/DataMap.h"
#include "dune/DuneInterface/Tool/AdcChannelDataModifier.h"
#include "dune/DuneInterface/Tool/AdcChannelViewer.h"
#include "dune/DuneCommon/TPadManipulator.h"
#include <memory>

class FembTestAnalyzer {

public:

  using Index = DuneFembReader::Index;
  using ManMap = std::map<std::string, TPadManipulator>;

  // Processing options.
  //      OptNoCalib - Signal is ADC - pedestal
  //  OptHeightCalib - Signal is calibrated by height
  //    OptAreaCalib - Signal is calibrated by area
  enum CalibOption { OptNoCalib, OptHeightCalib, OptAreaCalib };

  // ROI finding options.
  //       OptNoRoi - no ROI finding
  //     OptRoiPeak - Use the peak ROI finder
  //  OptRoiTickMod - Use the tickmod ROI finder
  enum RoiOption { OptNoRoi, OptRoiPeak, OptRoiTickMod };

  // Ctor from a FEMB sample set.
  // opt = 100*doDraw + 10*ropt + popt where
  //   doDraw indicates to draw canvas with each succesful call to draw(...)
  //   ropt is the ROI option (see enum above)
  //   popt is the processing option (see enum above) and:
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

  // Set the tick period used in the tickmod tree.
  int setTickPeriod(Index val);

  // Return the reader for the current sample.
  DuneFembReader* reader() const { return m_reader.get(); }

  // Return the parameters specifying the current sample.
  CalibOption calibOption() const { return m_copt; }
  std::string calibOptionName() const;
  bool isNoCalib() const { return calibOption() == OptNoCalib; }
  bool isHeightCalib() const { return calibOption() == OptHeightCalib; }
  bool isAreaCalib() const { return calibOption() == OptAreaCalib; }
  bool isCalib() const { return isHeightCalib() || isAreaCalib(); }
  RoiOption roiOption() const { return m_ropt; }
  std::string roiOptionName() const;
  bool doRoi() const { return roiOption() != OptNoRoi; }
  bool doPeakRoi() const { return roiOption() == OptRoiPeak; }
  bool doTickModRoi() const { return roiOption() == OptRoiTickMod; }
  string calibName(bool capitalize =false) const;
  bool doDraw() const { return m_doDraw; }
  int femb() const { return m_femb; }
  std::string tspat() const { return m_tspat; }
  bool isCold() const { return m_isCold; }
  int gainIndex() const { return reader()==nullptr ? -1 : reader()->gainIndex(); }
  int shapingIndex() const { return reader()==nullptr ? -1 : reader()->shapingIndex(); }
  bool extPulse() const { return reader() == nullptr ? false : reader()->extPulse(); }
  bool extClock() const { return reader() == nullptr ? false : reader()->extClock(); }

  // Other getters.
  // If useAll is true, processAll is called before building tree.
  bool haveTools() const;     // This will be false if tools are not found in initialization.
  FembTestPulseTree* pulseTree(bool useAll =true);
  FembTestTickModTree* tickModTree(bool useAll =true);
  Index tickPeriod() const { return m_tickPeriod; }

  // Setters.
  bool setDoDraw(bool val) { return m_doDraw = val; }

  // Event and channel counts for the current sample.
  Index nEvent() const { return m_reader == nullptr ? 0 : m_reader->nEvent(); }
  Index nChannel() const { return m_reader == nullptr ? 0 : m_reader->nChannel(); }
  Index nChannel(Index ievt) const { return m_reader == nullptr ? 0 : m_reader->nChannel(ievt); }
  Index nChannelEventProcessed() const { return m_nChannelEventProcessed; }

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
  DataMap getChannelDeviations(Index icha);
  const DataMap& processChannel(Index icha);

  // Process all channels.
  // If period > 0, the tick period is first set to that value.
  const DataMap& processAll(int period =-1);

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

  CalibOption m_copt;
  RoiOption m_ropt;
  bool m_doDraw;
  int m_femb;
  std::string m_tspat;
  bool m_isCold;
  int m_gain;
  int m_shap;
  std::unique_ptr<DuneFembReader> m_reader;
  std::vector<std::string> adcModifierNames;
  std::vector<std::unique_ptr<AdcChannelDataModifier>> adcModifiers;
  std::vector<std::string> adcViewerNames;
  std::vector<std::unique_ptr<AdcChannelViewer>> adcViewers;
  ManMap m_mans;
  std::unique_ptr<FembTestPulseTree> m_ptreePulse;
  std::unique_ptr<FembTestTickModTree> m_ptreeTickMod;
  Index m_tickPeriod;
  Index m_nChannelEventProcessed;

  // Parameters.
  // These are deduced from the signal unit (ADC counts, ke, ...)
  std::string m_signalUnit;
  double m_dsigmin = 1000.0;      // Min signal unc in graphs used in gain fits (protect against stickcy codes)
  double m_dsigflow = 1000.0;     // Signal unc for under/overflows in graphs used in gain fits
  int m_sigDevHistBinCount = 10;  // bins in deviation histos
  double m_sigDevHistMax = 1.0;   // Limit for deviation histos

  // Fetch tools.
  // Separate so it can be called after gain and shaping have been determined.
  void getTools();

  // Make pattern substitutions on tool names.
  //  %GAIN% --> gainIndex()
  //  %SHAP% --> shapingIndex()
  void fixToolNames(std::vector<std::string>& names) const;

};

#endif
