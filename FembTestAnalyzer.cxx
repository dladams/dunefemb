// FembTestAnalyzer.cxx

#include "FembTestAnalyzer.h"
#include <iostream>
#include <fstream>
#include <map>
#include <iomanip>
#include "dune/DuneInterface/AdcChannelData.h"
#include "dune/ArtSupport/DuneToolManager.h"
#include "DuneFembFinder.h"
#include "TGraphErrors.h"
#include "TROOT.h"
#include "TF1.h"
#include "TPad.h"
#include "TLatex.h"
#include "TLegend.h"

using std::string;
using std::cout;
using std::endl;
using std::map;
using std::setprecision;
using std::fixed;
using std::ofstream;

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int opt, int a_femb, int a_gain, int a_shap, 
                 std::string a_tspat, bool a_isCold,
                 bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(opt, a_femb, a_tspat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock);
}

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int opt, string dir, string fpat,
                 int a_femb, int a_gain, int a_shap, 
                 bool a_isCold, bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(opt, a_femb, fpat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock, dir);
}

//**********************************************************************

FembTestAnalyzer::FembTestAnalyzer(int opt, int a_femb, string a_tspat, bool a_isCold)
: m_opt(CalibOption(opt%100)), m_doDraw(opt>99), m_femb(a_femb), m_tspat(a_tspat), m_isCold(a_isCold),
  m_ptreePulse(nullptr) {
  const string myname = "FembTestAnalyzer::ctor: ";
  DuneToolManager* ptm = DuneToolManager::instance("dunefemb.fcl");
  if ( ptm == nullptr ) {
    cout << myname << "Unable to retrieve tool manager." << endl;
    return;
  }
  adcModifierNames.push_back("adcPedestalFit");
  if ( isNoCalib() ) {
    adcModifierNames.push_back("adcSampleFiller");
    adcModifierNames.push_back("adcThresholdSignalFinder");
  }
  if ( isHeightCalib() ) {
     adcModifierNames.push_back("fembCalibrator");
     adcModifierNames.push_back("keThresholdSignalFinder");
  }
  for ( string modname : adcModifierNames ) {
    auto pmod = ptm->getPrivate<AdcChannelDataModifier>(modname);
    if ( ! pmod ) {
      cout << myname << "Unable to find modifier " << modname << endl;
      adcModifiers.clear();
      return;
    }
    adcModifiers.push_back(std::move(pmod));
  }
  vector<string> acdViewerNames = {"adcRoiViewer"};
  acdViewerNames.push_back("adcPlotRaw");
  acdViewerNames.push_back("adcPlotRawDist");
  acdViewerNames.push_back("adcPlotPrepared");
  for ( string vwrname : acdViewerNames ) {
    auto pvwr = ptm->getPrivate<AdcChannelViewer>(vwrname);
    if ( ! pvwr ) {
      cout << myname << "Unable to find viewer " << vwrname << endl;
      adcModifiers.clear();
      adcViewers.clear();
      return;
    }
    adcViewers.push_back(std::move(pvwr));
  }
}

//**********************************************************************

int FembTestAnalyzer::
find(int a_gain, int a_shap, bool a_extPulse, bool a_extClock, string dir) {
  const string myname = "FembTestAnalyzer::find: ";
  DuneFembFinder fdr;
  chanevtResults.clear();
  chanResults.clear();
  cout << myname << "Fetching reader." << endl;
  if ( dir.size() ) {
    m_reader = std::move(fdr.find(dir, tspat()));
  } else {
    m_reader = std::move(fdr.find(femb(), isCold(), tspat(), a_gain, a_shap, a_extPulse, a_extClock));
  }
  cout << myname << "Done fetching reader." << endl;
  chanevtResults.resize(nChannel(), vector<DataMap>(nEvent()));
  chanResults.resize(nChannel());
  return 0;
}

//**********************************************************************

string FembTestAnalyzer::optionName() const {
  if ( isNoCalib()     ) return "OptNoCalib";
  if ( isHeightCalib() ) return "OptHeightCalib";
  if ( isAreaCalib()   ) return "OptAreaCalib";
  return "OptUnknown";
}

//**********************************************************************

string FembTestAnalyzer::calibName(bool capitalize) const {
  if ( isNoCalib()     ) return capitalize ? "Uncalibrated" : "uncalibrated";
  if ( isHeightCalib() ) return capitalize ? "Height" : "height";
  if ( isAreaCalib()   ) return capitalize ? "Area" : "area";
  return "UnknownCalibration";
}

//**********************************************************************

double FembTestAnalyzer::chargeFc(Index ievt) {
  const DuneFembReader* prdr = reader();
  if ( prdr == nullptr ) return 0.0;
  double pulseQfC = 0.0;
  if ( prdr->extPulse() ) {
    // Calibration from Brian 25Oct2017
    static double calibFembV[64] = {
      0.606,0.625,0.644,0.663,0.682,0.701,0.720,0.739,0.758,0.777,0.796,0.815,0.834,
      0.853,0.872,0.891,0.909,0.928,0.947,0.966,0.985,1.004,1.023,1.042,1.061,1.080,1.099,1.118,1.137,
      1.156,1.175,1.194,1.213,1.232,1.251,1.269,1.288,1.307,1.326,1.345,1.364,1.383,1.402,1.421,1.440,
      1.459,1.478,1.497,1.516,1.535,1.554,1.573,1.592,1.611,1.629,1.648,1.667,1.686,1.705,1.724,1.743,
      1.762,1.781,1.800
    };
    if ( ievt > 1 ) {
      Index ipt = ievt - 1;
      pulseQfC = fembCfF()*(calibFembV[ipt] - calibFembV[0]);
    }
  } else {
    pulseQfC = (ievt)*fembCfF()*0.001*fembVmV();
  }
  return pulseQfC;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::
processChannelEvent(Index icha, Index ievt) {
  const string myname = "FembTestAnalyzer::processChannelEvent: ";
  if ( dbg > 2 ) cout << myname << "Processing channel " << icha << ", event " << ievt << endl;
  if ( reader() == nullptr ) {
    cout << myname << "Reader is not defined." << endl;
    static DataMap res1(1);
    return res1;
  }
  if ( adcModifiers.size() == 0 ) {
    cout << myname << "ADC modifiers are not defined." << endl;
    static DataMap res2(2);
    return res2;
  }
  ostringstream sscha;
  sscha << icha;
  string scha = sscha.str();
  ostringstream ssevt;
  ssevt << ievt;
  string sevt = ssevt.str();
  DataMap& res = chanevtResults[icha][ievt];
  if ( res.haveInt("channel") ) return res;
  res.setInt("channel", icha);
  res.setInt("event", ievt);
  const DuneFembReader* prdr = reader();
  if ( prdr == nullptr ) return res.setStatus(1);
  double pulseQfC = chargeFc(ievt);
  double pulseQe = pulseQfC*elecPerFc();
  res.setFloat("nElectron", pulseQe);
  AdcChannelData acd;
  acd.run = femb();
  acd.event = ievt;
  acd.channel = icha;
  acd.fembID = femb();
  // Read raw data.
  reader()->read(ievt, icha, &acd);
  // Process the data.
  DataMap resmod;
  if ( dbg > 2 ) cout << myname << "Applying modifiers." << endl;
  Index imod = 0;
  for ( const std::unique_ptr<AdcChannelDataModifier>& pmod : adcModifiers ) {
    string modName = adcModifierNames[imod];
   if ( dbg > 2 ) cout << "Applying modifier " << modName << endl;
    resmod += pmod->update(acd);
    if ( resmod.status() ) {
      cout << myname << "Modifier " << modName << " returned error "
           << resmod.status() << endl;
      return res.setStatus(2);
    }
    ++imod;
  }
  if ( dbg > 3 ) {
    cout << myname << "Result:" << endl;
    cout << myname << "----------------------------------" << endl;
    resmod.print();
    cout << myname << "----------------------------------" << endl;
  }
  if ( dbg > 2 ) cout << myname << "Applying viewers." << endl;
  for ( const std::unique_ptr<AdcChannelViewer>& pvwr : adcViewers ) {
    resmod += pvwr->view(acd);
  }
  res += resmod;
  if ( dbg >= 2 ) {
    cout << myname << "Begin display of processing result. ----------------" << endl;
    resmod.print();
    cout << myname << "End display of processing result. ------------------" << endl;
  }
  // Record the pedestal.
  res.setFloat("pedestal", acd.pedestal);
  // Check units.
  string sigunit = acd.sampleUnit;
  if ( sigunit == "" ) {
    cout << myname << "ERROR: Sample has no units." << endl;
    return res.setStatus(2);
  }
  if ( m_signalUnit == "" ) {
    m_signalUnit = sigunit;
    if ( sigunit == "ADC counts" ) {
      m_dsigmin = 4.0;
      m_dsigflow = 500.0;
    } else if ( sigunit == "ke" ) {
      m_sigDevHistBinCount = 200;
      m_sigDevHistMax = 20.0;
      m_dsigmin = 0.5;
      m_dsigflow = 100.0;
    }
    if ( isNoCalib() ) {
      if ( sigunit != "ADC counts" ) {
        cout << myname << "ERROR: Uncalibrated sample has unexpected units: " << sigunit << endl;
        return res.setStatus(3);
      }
    } else {
      if ( sigunit != "ke" ) {
        cout << myname << "ERROR: Uncalibrated sample has unexpected units: " << sigunit << endl;
        return res.setStatus(4);
      }
    }
  }
  if ( sigunit != m_signalUnit ) {
    cout << myname << "ERROR: Sample has inconsistent unit: " << sigunit
         << " != " << m_signalUnit << endl;
    return res.setStatus(5);
  }
  if ( dbg >= 2 ) cout << myname << "Units for samples are " << sigunit << endl;
  // Process the ROIs.
  bool haverois = resmod.haveInt("roiCount");
  Index nroi = haverois ? resmod.getInt("roiCount") : 0;
  if ( dbg > 2 ) cout << myname << "ROI count is " << nroi << endl;
  if ( nroi ) {
    vector<float> sigAreas[2];
    vector<float> sigHeights[2];
    vector<float> sigCals[2];    // calibrated charge
    vector<float> sigDevs[2];    // Deviation of calibrated value from expected value
    int sigNundr[2] = {0, 0};
    int sigNover[2] = {0, 0};
    float areaMin[2] = {1.e9, 1.e9};
    float areaMax[2] = {0.0, 0.0};
    float heightMin[2] = {1.e9, 1.e9};
    float heightMax[2] = {0.0, 0.0};
    // Set range of ROIs removing edge ROIs.
    Index iroi1 = 0;
    Index iroi2 = nroi;
    Index firstRoiFirstTick = resmod.getIntVector("roiTick0s")[0];
    Index lastRoiLastTick = resmod.getIntVector("roiTick0s")[nroi-1]
                          + resmod.getIntVector("roiNTicks")[nroi-1];
    if ( firstRoiFirstTick <= 0 ) ++iroi1;
    if ( lastRoiLastTick >= resmod.getInt("roiNTickChannel") ) --iroi2;
    // Loop over ROIs and fetch the height and area.
    // Also count the number of ROIs with under and overflow bins.
    // All are done separately for each sign.
    float expSig = 0.001*pulseQe;
    vector<bool> roiIsPos(nroi, false);
    for ( Index iroi=iroi1; iroi<iroi2; ++iroi ) {
      float area = resmod.getFloatVector("roiSigAreas")[iroi];
      float sigmin = resmod.getFloatVector("roiSigMins")[iroi];
      float sigmax = resmod.getFloatVector("roiSigMaxs")[iroi];
      int nundr = resmod.getIntVector("roiNUnderflows")[iroi];
      int nover = resmod.getIntVector("roiNOverflows")[iroi];
      Index isPos = area >= 0.0;
      roiIsPos[iroi] = isPos;
      float sign = isPos ? 1.0 : -1.0;
      area *= sign;
      float height = isPos ? sigmax : -sigmin;
      sigAreas[isPos].push_back(area);
      sigHeights[isPos].push_back(height);
      if ( nundr > 0 ) ++sigNundr[isPos];
      if ( nover > 0 ) ++sigNover[isPos];
      if ( area < areaMin[isPos] ) areaMin[isPos] = area;
      if ( area > areaMax[isPos] ) areaMax[isPos] = area;
      if ( height < heightMin[isPos] ) heightMin[isPos] = height;
      if ( height > heightMax[isPos] ) heightMax[isPos] = height;
      float qcal = 0.0;
      if ( isHeightCalib() ) qcal = height;
      if ( isAreaCalib() )   qcal = area;
      qcal *= sign;
      sigCals[isPos].push_back(qcal);
      float dev = -999.0;
      if ( isCalib() ) dev = sign*(qcal - expSig);
      sigDevs[isPos].push_back(dev);
    }
    // For each sign...
    for ( Index isgn=0; isgn<2; ++isgn ) {
      string ssgn = isgn ? "Pos" : "Neg";
      string cntname = "sigCount" + ssgn;
      res.setInt(cntname, sigAreas[isgn].size());
      // Build area histogram. Record it and its mean and RMS.
      if ( sigAreas[isgn].size() ) {
        string hnam = "hsigArea" + ssgn;
        string httl = "FEMB test signal area " + ssgn +
                      " channel " + scha + " event " + sevt +
                      "; " + sigunit + "; # signals";
        float xmin = areaMin[isgn];
        float xmax = areaMax[isgn];
        float xavg = 0.5*(xmin + xmax);
        float xcen = int(xavg + 0.5);
        int   nbin = isNoCalib() ?  64 : 100;
        float dbin = isNoCalib() ? 1.0 : 0.5;
        xmin = xcen - 0.5*nbin*dbin;
        xmax = xcen + 0.5*nbin*dbin;
        TH1* ph = new TH1F(hnam.c_str(), httl.c_str(), nbin, xmin, xmax);
        ph->SetStats(0);
        ph->SetLineWidth(2);
        for ( float val : sigAreas[isgn] ) ph->Fill(val);
        res.setHist(hnam, ph, true);
        string meanName = "sigAreaMean" + ssgn;
        string rmsName = "sigAreaRms" + ssgn;
        res.setFloat(meanName, ph->GetMean());
        res.setFloat(rmsName, ph->GetRMS());
      } else {
        cout << myname << "No " + ssgn + " area ROIS " + " for channel " << icha
             << " event " << ievt << endl;
      }
      // Build height histogram. Record it and its mean and RMS.
      if ( sigHeights[isgn].size() ) {
        string hnam = "hsigHeight" + ssgn;
        string httl = "FEMB test signal height " + ssgn +
                      " channel " + scha + " event " + sevt +
                      "; " + sigunit + "; # signals";
        float xmin = heightMin[isgn];
        float xmax = heightMax[isgn];
        float xavg = 0.5*(xmin + xmax);
        float xcen = int(xavg + 0.5);
        int nbin   = isNoCalib() ? 64 : 50;
        float dbin = isNoCalib() ? 1.0 : 0.2;
        xmin = xcen - 0.5*nbin*dbin;
        xmax = xcen + 0.5*nbin*dbin;
        TH1* ph = new TH1F(hnam.c_str(), httl.c_str(), nbin, xmin, xmax);
        ph->SetStats(0);
        ph->SetLineWidth(2);
        for ( float val : sigHeights[isgn] ) ph->Fill(val);
        res.setHist(hnam, ph, true);
        string meanName = "sigHeightMean" + ssgn;
        string rmsName = "sigHeightRms" + ssgn;
        res.setFloat(meanName, ph->GetMean());
        res.setFloat(rmsName, ph->GetRMS());
      } else {
        cout << myname << "No " + ssgn + " height ROIS " + " for channel " << icha
             << " event " << ievt << endl;
      }
      // Build the deviation histogram. Record it and its mean and RMS.
      if ( sigDevs[isgn].size() ) {
        string hnam = "hsigDev" + ssgn;
        string httl = calibName(true) + " deviation for event " + sevt + " " + ssgn +
                      "; #DeltaQ [" + sigunit + "]; # signals";
        float xmax = m_sigDevHistMax;
        float xmin = -xmax;
        float nbin = m_sigDevHistBinCount;
        TH1* ph = new TH1F(hnam.c_str(), httl.c_str(), nbin, xmin, xmax);
        ph->SetStats(0);
        ph->SetLineWidth(2);
        for ( float val : sigDevs[isgn] ) ph->Fill(val);
        res.setHist(hnam, ph, true);
        string meanName = "sigDevMean" + ssgn;
        string rmsName = "sigDevRms" + ssgn;
        res.setFloat(meanName, ph->GetMean());
        res.setFloat(rmsName, ph->GetRMS());
      }
      // Evaluate the sticky-code metrics.
      //    S1 = fraction of codes in most populated bin
      //    S2 = fraction of codes with classic sticky values
      if ( sigHeights[isgn].size() ) {
        string vecname = isgn ? "roiTickMaxs" : "roiTickMins";
        const DataMap::IntVector& roiTicks = resmod.getIntVector(vecname);
        const DataMap::IntVector& roiTick0s = resmod.getIntVector("roiTick0s");
        map<AdcIndex, Index> adcCounts;   // map of counts for each ADC code
        for ( Index iroi=iroi1; iroi<iroi2; ++iroi ) {
          if ( roiIsPos[iroi] != isgn ) continue;
          Index tick = roiTicks[iroi] + roiTick0s[iroi];
          AdcCount adcCode = acd.raw[tick];
          if ( adcCounts.find(adcCode) == adcCounts.end() ) adcCounts[adcCode] = 0;
          adcCounts[adcCode] += 1;
        }
        Index maxCount = 0;
        Index sumCount = 0;
        Index stickyCount = 0;
        for ( auto ent : adcCounts ) {
          const AdcIndex adcCode = ent.first;
          const Index count = ent.second;
          if ( count > maxCount ) maxCount = count;
          sumCount += count;
          const AdcIndex adcCodeMod = adcCode%64;
          if ( adcCodeMod == 0 || adcCodeMod == 63 ) stickyCount += count;
        }
        float s1 = sumCount > 0 ? float(maxCount)/float(sumCount) : -1.0;
        float s2 = sumCount > 0 ? float(stickyCount)/float(sumCount) : -1.0;
        res.setFloat("stickyFraction1" + ssgn, s1);
        res.setFloat("stickyFraction2" + ssgn, s2);
      }
      // Record the # of ROIS with underflow and overflow bins.
      string unam = "sigNUnderflow" + ssgn;
      string onam = "sigNOverflow" + ssgn;
      res.setInt(unam, sigNundr[isgn]);
      res.setInt(onam, sigNover[isgn]);
      // Record deviations if this signal is calibrated.
      if ( isCalib() ) {
        string varname = "roiSigCal" + ssgn;
        res.setFloatVector(varname, sigCals[isgn]);
        varname = "roiSigDev" + ssgn;
        res.setFloatVector(varname, sigDevs[isgn]);
      }
      // Record mean and RMS of the calibrated signal.
      if ( isCalib() ) {
        Index nsum = 0;
        double xsum = 0.0;
        double xxsum = 0.0;
        for ( float sig : sigCals[isgn] ) {
          ++nsum;
          xsum += sig;
          xxsum += sig*sig;
        }
        double xmean = nsum>0 ? xsum/nsum : 0.0;
        double xxmean = nsum>0 ? xxsum/nsum : 0.0;
        float sigMean = xmean;
        float sigRms = sqrt(xxmean - xmean*xmean);
        res.setFloat("roiSigCalMean" + ssgn, sigMean);
        res.setFloat("roiSigCalRms" + ssgn,  sigRms);
      }
    }  // End loop over isgn
  } else if ( haverois ) {
    if ( ievt > 1 ) cout << myname << "ERROR: No ROIs found for channel " << icha
                         << " event " << ievt << "." << endl;
  } else {
    cout << myname << "ERROR: roiCount does not appear in output." << endl;
  }
  return res;
}

//**********************************************************************

DataMap FembTestAnalyzer::getChannelResponse(Index icha, string usePosOpt, bool useArea) {
  const string myname = "FembTestAnalyzer::getChannelResponse: ";
  DataMap res;
  const DuneFembReader* prdr = reader();
  if ( prdr == nullptr ) {
    cout << myname << "No reader found." << endl;
    return DataMap(1);
  }
  Index nevt = nEvent();
  Index ievt0 = 0;  // Event with zero charge, i.e. ievt0+1 is one unit.
  if ( !prdr->extPulse()  && !prdr->intPulse() ) {
    cout << myname << "Reader does not specify if pulse is internal or external." << endl;
    return DataMap(2);
  }
  bool pulseIsExternal = prdr->extPulse();
  if ( pulseIsExternal ) ievt0 = 1;
  bool useBoth = usePosOpt == "Both";
  vector<bool> usePosValues;
  if ( usePosOpt == "Neg" || useBoth ) usePosValues.push_back(false);
  if ( usePosOpt == "Pos" || useBoth ) usePosValues.push_back(true);
  if ( usePosValues.size() == 0 ) {
    cout << myname << "Invalid value for usePosOpt: " << usePosOpt << endl;
    return DataMap(3);
  }
  string styp = useArea ? "Area" : "Height";
  ostringstream sscha;
  sscha << icha;
  string scha = sscha.str();
  string hnam = "hchaResp" + styp + usePosOpt;
  string usePosLab = (usePosOpt == "Both") ? "" : " " + usePosOpt;
  string gttl = styp + " response" + usePosLab + " channel " + scha;
  string gttlr = styp + " response residual" + usePosLab + " channel " + scha;
  string httl = gttl + " ; charge factor; Mean ADC " + styp;
  // Create arrays to hold pulse ADC count vs. # ke.
  // For external pulser, include (0,0).
  Index npt = prdr->extPulse() ? 1 : 0;
  npt = 0;  // Don't use the point at zero
  vector<float> x(npt, 0.0);
  vector<float> dx(npt, 0.0);
  vector<float> y(npt, 0.0);
  vector<float> dy(npt, 0.0);
  vector<float> dyFit(npt, 0.0);
  vector<float> fitPed(npt, 0.0);
  vector<float> sticky1[2];
  vector<float> sticky2[2];
  vector<bool> fitkeep(npt, true);
  double ymax = 1000.0;
  vector<float> peds(nevt, 0.0);
  vector<float> nkeles(nevt, 0.0);
  Index iptPosGood = 0;  // Last good point for positive signals.
  Index iptNegGood = 0;
  Index nptPosBad = 0;
  Index nptNegBad = 0;
  if ( dbg > 1 ) cout << myname << "Looping over " << nevt << " events." << endl;
  for ( Index ievt=0; ievt<nevt; ++ievt ) {
    const DataMap& resevt = processChannelEvent(icha, ievt);
    if ( dbg > 1 ) {
      cout << myname << "Event " << ievt << endl;
      if ( dbg > 3 ) resevt.print();
    }
    peds[ievt] = resevt.getFloat("pedestal");
    int roiCount = resevt.getInt("roiCount");
    float nele = resevt.getFloat("nElectron");
    float nkele = 0.001*nele;
    nkeles[ievt] = nkele;
    if ( ievt > ievt0 ) {
      for ( bool usePos : usePosValues ) {
        string ssgn = usePos ? "Pos" : "Neg";
        string meaName = "sig" + styp + "Mean" + ssgn;
        string rmsName = "sig" + styp + "Rms" + ssgn;
        string ovrName = "sigNOverflow" + ssgn;
        string udrName = "sigNUnderflow" + ssgn;
        string sticky1Name = "stickyFraction1" + ssgn;
        string sticky2Name = "stickyFraction2" + ssgn;
        Index ipt = x.size();
        int nudr = resevt.getInt(udrName);
        int novr = resevt.getInt(ovrName);   // # ROI with ADC overflows
        if ( resevt.haveFloat(meaName) ) {
          float sigmean = resevt.getFloat(meaName);
          float sigrms  = resevt.getFloat(rmsName);
          bool isUnderOver = nudr>0 || novr>0;
          if ( isUnderOver ) {
            if ( usePos ) ++nptPosBad;
            else          ++nptNegBad;
          } else {
            if ( usePos ) iptPosGood = ipt;
            else          iptNegGood = ipt;
          }
          float dsigFitMin = m_dsigmin;  // Intrinsic ADC uncertainty.
          if ( isUnderOver ) dsigFitMin = m_dsigflow;  // Big uncertainty for under/overflows.
          float sigsign = 1.0;
          if ( useBoth && !usePos ) sigsign = -1.0;
          float dsig = sigrms;
          float dsigFit = dsig;
          if ( dsigFit < dsigFitMin ) dsigFit = dsigFitMin;
          x.push_back(sigsign*0.001*nele);
          dx.push_back(0.0);
          y.push_back(sigsign*sigmean);
          dy.push_back(dsig);
          dyFit.push_back(dsigFit);
          fitPed.push_back(peds[ievt]);
          bool fitpt = true;
          if ( isUnderOver && !useBoth ) fitpt = false;
          if ( isUnderOver ) fitpt = false;
          if ( !pulseIsExternal && ievt < 2 ) fitpt = false;
          fitkeep.push_back(fitpt);
          double ylim = y[ipt] + dyFit[ipt];
          if ( ylim > ymax ) ymax = ylim;
        } else {
          cout << myname << styp << " " << ssgn << " mean not found for channel "
               << icha << " event " << ievt << ". ROI count is " << roiCount
               << ". Point skipped." << endl;
        }
        if ( resevt.haveFloat(sticky1Name) ) {
          sticky1[usePos].push_back(resevt.getFloat(sticky1Name));
        } else {
          sticky1[usePos].push_back(-2.0);
        }
        if ( resevt.haveFloat(sticky2Name) ) {
          sticky2[usePos].push_back(resevt.getFloat(sticky2Name));
        } else {
          sticky2[usePos].push_back(-2.0);
        }
      }
    } else {
      for ( bool usePos : usePosValues ) {
        sticky1[usePos].push_back(-3.0);
        sticky2[usePos].push_back(-3.0);
      }
    }
  }
  npt = x.size();
  vector<float> xf;
  vector<float> dxf;
  vector<float> yf;
  vector<float> dyf;
  vector<float> dyfFit;
  vector<float> fitPedf;
  for ( Index ipt=0; ipt<npt; ++ipt ) {
    if ( fitkeep[ipt] ) {
      xf.push_back(x[ipt]);
      dxf.push_back(dx[ipt]);
      yf.push_back(y[ipt]);
      dyf.push_back(dy[ipt]);
      dyfFit.push_back(dyFit[ipt]);
      fitPedf.push_back(fitPed[ipt]);
    }
  }
  Index nptf = xf.size();
  if ( nptf < 5 ) cout << myname << "WARNING: Fitted point count is " << nptf << endl;
  res.setInt("channel", icha);
  res.setFloatVector("peds", peds);
  res.setFloatVector("nkes", nkeles);
  for ( bool usePos : usePosValues ) {
    string ssgn = usePos ? "Pos" : "Neg";
    res.setFloatVector("sticky1" + ssgn + "s", sticky1[usePos]);
    res.setFloatVector("sticky2" + ssgn + "s", sticky2[usePos]);
  }
  httl = "Response " + styp + usePosLab + "; Charge [ke]; Mean ADC " + styp;
  string gnam = "gchaResp" + styp + usePosOpt;
  // Create graph with all points.
  double qmaxke = 0.001*elecPerFc()*chargeFc(nevt-1);
  double xmax = 50*int(qmaxke/50.0 + 1.3);
  TGraph* pg = new TGraphErrors(npt, &x[0], &y[0], &dx[0], &dy[0]);
  pg->SetName(gnam.c_str());
  pg->SetTitle(gttl.c_str());
  pg->SetLineWidth(2);
  pg->SetMarkerStyle(5);
  pg->GetXaxis()->SetTitle("Input charge [ke]");
  pg->GetXaxis()->SetLimits(-xmax, xmax);
  string ylab = "Output signal [" + m_signalUnit + "]";
  pg->GetYaxis()->SetTitle(ylab.c_str());
  // Create graph with fitted points.
  string gnamf = gnam + "FitPoints";
  string gttlf = gttl + " (fit points)";
  TGraph* pgf = new TGraphErrors(nptf, &xf[0], &yf[0], &dxf[0], &dyf[0]);
  pgf->SetName(gnamf.c_str());
  pgf->SetTitle(gttlf.c_str());
  pgf->SetLineWidth(2);
  pgf->SetMarkerStyle(4);
  pgf->GetXaxis()->SetTitle("Input charge [ke]");
  pgf->GetXaxis()->SetLimits(-xmax, xmax);
  pgf->GetYaxis()->SetTitle(ylab.c_str());
  // Create graph with fitted points and fit uncertainties.
  string gnamfFit = gnam + "FitPointsUnc";
  string gttlfFit = gttl + " (fit points, fit uncs)";
  TGraph* pgfFit = new TGraphErrors(nptf, &xf[0], &yf[0], &dxf[0], &dyfFit[0]);
  pgfFit->SetName(gnamfFit.c_str());
  pgfFit->SetTitle(gttlfFit.c_str());
  pgfFit->SetLineWidth(2);
  pgfFit->SetMarkerStyle(4);
  pgfFit->SetMarkerColor(2);
  pgfFit->GetXaxis()->SetTitle("Input charge [ke]");
  pgfFit->GetXaxis()->SetLimits(-xmax, xmax);
  pgfFit->GetYaxis()->SetTitle(ylab.c_str());
  // Set starting values for fit.
  Index iptGain = nptf > 0 ? nptf - 1 : 0;
  if ( iptPosGood ) iptGain = iptPosGood;
  else if ( iptNegGood ) iptGain = iptNegGood;
  float gain = nptf > 0 ? y[iptGain]/x[iptGain] : 0.0;
  float ped = 0.0;
  float adcmax = 100000;
  float adcmin = 0.0;
  float keloff = 0.0;
  bool fitAdcMin = false;
  bool fitAdcMax = false;
  bool fitKelOff = false;
  // Choose fit function.
  string sfit = "fgain";
  if ( isNoCalib() ) {
    if      ( usePosOpt == "Pos" )  sfit = "fgain";
    else if ( usePosOpt == "Neg" )  sfit = "fgainMax";
    else if ( useBoth ) {
      if ( prdr->extPulse() ) {
        if ( false && nptPosBad ) sfit = "fgainMinMax";
        else sfit = "fgainMin";
      } else if ( prdr->intPulse() ) {
        if ( false && nptPosBad ) sfit = "fgainOffMinMax";
        else sfit = "fgainOffMin";
      }
    }
  }
  TF1* pfit = nullptr;
  if ( nptf > 0 ) {
    if ( sfit == "fgain" ) {
      pfit = new TF1("fgain", "[0]*x");
    } else if ( sfit == "fgainMin" ) {
      pfit = new TF1("fgainMin", "x>[1]/[0]?[0]*x:[1]");
      adcmin = yf[nptf-2];
      pfit->SetParameter(1, adcmin);
      pfit->SetParName(1, "adcmin");
      fitAdcMin = true;
    } else if ( sfit == "fgainMax" ) {
      pfit = new TF1("fgainMax", "x<[1]/[0]?[0]*x:[1]");
      adcmax = yf[nptf-1];
      pfit->SetParameter(1, adcmax);
      pfit->SetParName(1, "adcmax");
      fitAdcMax = true;
    } else if ( sfit == "fgainMinMax" ) {
      pfit = new TF1("fgainMinMax", "x>[2]/[0]?(x<[1]/[0]?[0]*x:[1]):[2]");
      adcmin = yf[nptf-2];
      adcmax = yf[nptf-1];
      pfit->SetParName(1, "adcmax");
      pfit->SetParameter(1, adcmax);
      pfit->SetParLimits(1, adcmax-100, adcmax+100);
      pfit->SetParName(2, "adcmin");
      pfit->SetParameter(2, adcmin);
      pfit->SetParLimits(2, adcmin-100, adcmin+100);
      fitAdcMin = true;
      fitAdcMax = true;
    } else if ( sfit == "fgainOffMin" ) {
      pfit = new TF1("fgainOffMin", "fabs(x)>[2]?(x>([1]/[0]-[2])?([0]*(1-[2]/fabs(x))*x):[1]):0.0");
      adcmin = yf[nptf-2];
      adcmax = yf[nptf-1];
      pfit->SetParName(1, "adcmin");
      pfit->SetParameter(1, adcmin);
      pfit->SetParLimits(1, adcmin-100, adcmin+100);
      pfit->SetParName(2, "keloff");
      pfit->SetParameter(2, 10.0);
      pfit->SetParLimits(2, 1, 50);
      fitAdcMin = true;
      fitKelOff = true;
    } else if ( sfit == "fgainOffMinMax" ) {
      pfit = new TF1("fgainOffMinMax", "fabs(x)>[3]?(x>([2]/[0]-[3])?(x<([1]/[0]+[3])?[0]*(1-[3]/fabs(x))*x:[1]):[2]):0.0");
      adcmin = yf[nptf-2];
      adcmax = yf[nptf-1];
      pfit->SetParName(1, "adcmax");
      pfit->SetParameter(1, adcmax);
      pfit->SetParLimits(1, adcmax-100, adcmax+100);
      pfit->SetParName(2, "adcmin");
      pfit->SetParameter(2, adcmin);
      pfit->SetParLimits(2, adcmin-100, adcmin+100);
      pfit->SetParName(3, "keloff");
      pfit->SetParameter(3, 10.0);
      pfit->SetParLimits(3, 1, 50);
      fitAdcMin = true;
      fitAdcMax = true;
      fitKelOff = true;
    } else {
      cout << myname << "Invalid value for sfit: " << sfit << endl;
      return DataMap(4);
    }
    // Fit.
    pfit->SetParName(0, "gain");
    pfit->SetParameter(0, gain);
    pfit->SetParLimits(0, 0.8*gain, 1.2*gain);
    Index npar = pfit->GetNpar();
    //pgf->Fit(pfit, "", "", xfmin, x[2]);
    pgfFit->Fit(pfit, "Q");
    pgf->GetListOfFunctions()->Add(pfit);
    gain = pfit->GetParameter(0);
    if ( fitAdcMin ) adcmin = pfit->GetParameter(pfit->GetParNumber("adcmin"));
    if ( fitAdcMax ) adcmax = pfit->GetParameter(pfit->GetParNumber("adcmax"));
    if ( fitKelOff ) keloff = pfit->GetParameter(pfit->GetParNumber("keloff"));
    float qmin = adcmin/gain - keloff;
    float qmax = adcmax/gain - keloff;
/*
    // Refit without min saturation.
    if ( fitAdcMin ) {
      int ipar = pfit->GetParNumber("adcmin");
      pfit->FixParameter(ipar, adcmin);
      pgf->Fit(pfit, "Q+");
    }
    // Refit without min saturation.
    if ( fitAdcMin ) {
      delete pfit;
      pfit = new TF1("fgain", "[0]*x");
      pfit->SetParName(0, "gain");
      pfit->SetParameter(0, gain);
      pgf->Fit(pfit, "Q+", "", qmin, qmax);
    }
*/
    // Record the gain.
    gain = pfit->GetParameter(0);
    double gainUnc = pfit->GetParError(0);
    string fpname = "fitGain" + styp + usePosOpt;
    string fename = fpname + "Unc";
    res.setFloat(fpname, gain);
    res.setFloat(fename, gainUnc);
    // Record the ADC saturation.
    if ( fitAdcMax ) {
      fpname = "fitSaturationMax" + styp + usePosOpt;
      res.setFloat(fpname, adcmax);
    }
    if ( fitAdcMin ) {
      fpname = "fitSaturationMin" + styp + usePosOpt;
      res.setFloat(fpname, adcmin);
    }
    if ( fitKelOff ) {
      fpname = "fitKelOff" + styp + usePosOpt;
      res.setFloat(fpname, keloff);
    }
    // If there is no calibration, record values that can be used to deduce the
    // the raw ADC level at which the signal saturates.
    //   adcminWithPed - fitted saturation level minus the pedestal for first point after
    //   lowSaturatedRawAdcs - raw ADC means for the saturated points
    //   lowSaturatedRawAdcMax - maximum raw ADC mean for the saturated points
    if ( isNoCalib() && fitAdcMin ) {
      // Find the pedestal above and closest to adcmin.
      float dadcNearest = 1.e10;
      Index iptNearest = 0;
      vector<float> lowSaturatedRawAdcs;
      float lowSaturatedRawAdcMax = 0.0;
      for ( Index ipt=0; ipt<xf.size(); ++ipt ) {
        float q = xf[ipt];
        double a = pfit->Eval(q);
        if ( q > qmin ) {
          float dadc = a - adcmin;
          if ( dadc < dadcNearest ) {
            iptNearest = ipt;
            dadcNearest = dadc;
          }
        } else {
          float rawAdc = yf[ipt] + fitPedf[ipt];
          lowSaturatedRawAdcs.push_back(rawAdc);
          if ( rawAdc > lowSaturatedRawAdcMax ) lowSaturatedRawAdcMax = rawAdc;
        }
      }
      float adcminWithPed = adcmin + fitPedf[iptNearest];
      res.setFloat("adcminWithPed", adcminWithPed);
      res.setFloatVector("lowSaturatedRawAdcs", lowSaturatedRawAdcs);
      res.setFloat("lowSaturatedRawAdcMax", lowSaturatedRawAdcMax);
    }
    // Copy function to original graph.
    //TF1* pfitCopy = (TF1*) pfit->Clone();
    TF1* pfitCopy = new TF1;
    pfit->Copy(*pfitCopy);
  }
  //pg->GetListOfFunctions()->Add(pfitCopy);
  //pg->GetListOfFunctions()->Add(pfit);
  // Create residual graph.
  // Include fitted points in the linear region.
  vector<float> xres;
  vector<float> yres;
  vector<float> dyres;
  vector<float> xrese;
  vector<float> yrese;
  vector<float> dyrese;
  double xlinMin = adcmin/gain;
  double xlinMax = adcmax/gain;
  for ( Index ipt=0; ipt<nptf; ++ipt ) {
    double xpt = xf[ipt];
    double ypt = yf[ipt];
    double yfit = pfit->Eval(xpt);
    double ydif = ypt - yfit;
    xres.push_back(xpt);
    yres.push_back(ydif/gain);
    dyres.push_back(dyf[ipt]/gain);
    if ( fitAdcMin && (yfit <= 0.9999*adcmin) ) continue;
    if ( fitAdcMax && (yfit >= 0.9999*adcmax) ) continue;
    xrese.push_back(xpt);
    yrese.push_back(ydif/gain);
    dyrese.push_back(dyf[ipt]/gain);
  }
  string gnamr = gnam + "Res";
  TGraph* pgr = new TGraphErrors(xrese.size(), &xrese[0], &yrese[0], &dx[0], &dyrese[0]);
  pgr->SetName(gnamr.c_str());
  pgr->SetTitle(gttlr.c_str());
  pgr->SetLineWidth(2);
  pgr->SetMarkerStyle(4);
  pgr->GetXaxis()->SetTitle("Input charge [ke]");
  pgr->GetXaxis()->SetLimits(-xmax, xmax);
  pgr->GetYaxis()->SetTitle("Output signal residual [ke]");
  // Create histograms of the RMS and abs(residual) of all fitted points
  // in the linear regions.
  // Set flag for deviation histogram. We do this if calibration is applied
  // and is of type useArea.
  string hnamBase = "hchaResp" + styp + usePosOpt;
  string httlBase = styp + " response ";
  string hylab = "# entries";
  string hnamRms = hnamBase + "Rms";
  string httlRms = httlBase + " local RMS channel " + scha + "; #DeltaQ [ke]; " + hylab;
  string hnamArs = hnamBase + "Ars";
  string httlArs = httlBase + "|residual| channel " + scha + "; #DeltaQ [ke]; " + hylab;
  string hnamGms = hnamBase + "Gms";
  string httlGms = httlBase + " global RMS channel " + scha + "; #DeltaQ [ke]; " + hylab;
  TH1* phrms = new TH1F(hnamRms.c_str(), httlRms.c_str(), 50, 0, 5);
  TH1* phars = new TH1F(hnamArs.c_str(), httlArs.c_str(), 50, 0, 5);
  TH1* phgms = new TH1F(hnamGms.c_str(), httlGms.c_str(), 50, 0, 5);
  vector<TH1*> rmsHists = {phrms, phars, phgms};
  for ( TH1* ph : rmsHists ) {
    ph->SetStats(0);
    ph->SetLineWidth(2);
  }
  phrms->SetLineColor(49);
  phars->SetLineColor(39);
  for ( Index ipt=0; ipt<xrese.size(); ++ipt ) {
    double yrms = dyrese[ipt];
    double yars = fabs(yrese[ipt]);
    double ygms = sqrt(yrms*yrms + yars*yars);
    phrms->Fill(yrms);
    phars->Fill(yars);
    phgms->Fill(ygms);
  }
  res.setGraph(gnam, pg);
  res.setGraph(gnamf, pgf);
  res.setGraph(gnamfFit, pgfFit);
  res.setGraph(gnamr, pgr);
  for ( TH1* ph : rmsHists ) res.setHist(ph->GetName(), ph, true);
  return res;
}

//**********************************************************************

DataMap FembTestAnalyzer::getChannelDeviations(Index icha) {
  const string myname = "FembTestAnalyzer::getChannelDeviations: ";
  ostringstream sscha;
  sscha << icha;
  string scha = sscha.str();
  DataMap res;
  res.setInt("channel", icha);
  string hylab = "# entries";
  string hnamDev = "hdev";
  string httlDev = calibName(true) + " deviation channel " + scha + "; #DeltaQ [ke]; " + hylab;
  string hnamAdv = "hadv";
  string httlAdv = "Signal |deviation| channel " + scha + "; #DeltaQ [ke]; " + hylab;
  TH1* phDev = new TH1F(hnamDev.c_str(), httlDev.c_str(), 200, -20, 20);
  TH1* phAdv = new TH1F(hnamAdv.c_str(), httlAdv.c_str(), 50, 0, 5);
  vector<TH1*> hists = {phDev, phAdv};
  Index nevt = nEvent();
  for ( Index ievt=0; ievt<nevt; ++ievt ) {
    if ( chargeFc(ievt) == 0.0 ) continue;
    const DataMap& resevt = processChannelEvent(icha, ievt);
    for ( string ssgn : {"Neg", "Pos"} ) {
      string devVecName = "roiSigDev" + ssgn;
      string nUnderName = "sigNUnderflow" + ssgn;
      string nOverName = "sigNOverflow" + ssgn;
      vector<string> missing;
      if ( ! resevt.haveFloatVector(devVecName) ) missing.push_back(devVecName);
      if ( ! resevt.haveInt(nUnderName) ) missing.push_back(nUnderName);
      if ( ! resevt.haveInt(nOverName) ) missing.push_back(nOverName);
      if ( missing.size() ) {
        cout << myname << "Event result is missing required " << ssgn
             << " data for event " << ievt << ":" << endl;
        for ( string name : missing ) cout << myname << "  " << name << endl;
        continue;
      }
      int nUnder = resevt.getInt(nUnderName);
      int nOver  = resevt.getInt(nOverName);
      if ( nUnder ) continue;
      if ( nOver ) continue;
      const vector<float> devs = resevt.getFloatVector(devVecName);
      for ( float dev : devs ) {
        phDev->Fill(dev);
        phAdv->Fill(fabs(dev));
      }
    }
  }
  for ( TH1* ph : hists ) {
    ph->SetLineWidth(2);
    ph->SetStats(0);
    res.setHist(ph, true);  
  }
  return res;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::processChannel(Index icha) {
  DataMap& res = chanResults[icha];
  if ( res.haveInt("channel") ) return res;
  bool useBoth = true;
  if ( useBoth ) {
    res += getChannelResponse(icha, "Both",  true);
    res += getChannelResponse(icha, "Both", false);
  } else {
    res += getChannelResponse(icha, "Pos",  true);
    res += getChannelResponse(icha, "Neg",  true);
    res += getChannelResponse(icha, "Pos", false);
    res += getChannelResponse(icha, "Neg", false);
  }
  if ( isCalib() ) res += getChannelDeviations(icha);
  // Create pedestal graph.
  const DataMap::FloatVector& nkes = res.getFloatVector("nkes");
  const DataMap::FloatVector& peds = res.getFloatVector("peds");
  Index npt = nkes.size();
  string gnamp = "gchaPeds";
  ostringstream ssttl;
  ssttl << "Pedestals for channel " << icha;
  string gttl = ssttl.str();
  TGraph* pgp = new TGraph(npt, &nkes[0], &peds[0]);
  pgp->SetName(gnamp.c_str());
  pgp->SetTitle(gttl.c_str());
  pgp->SetLineWidth(2);
  pgp->GetXaxis()->SetTitle("Input charge [ke]");
  pgp->GetYaxis()->SetTitle("Pedestal [ADC]");
  float pedMin = peds[0];
  float pedMax = pedMin;
  for ( float ped : peds ) {
    if ( ped < pedMin ) pedMin = ped;
    if ( ped > pedMax ) pedMax = ped;
  }
  float ymax = 10*int(pedMax/10.0+1.2);
  float ymin = 10*int(pedMin/10.0);
  if ( (ymax - ymin) < 50.0 ) ymin = ymax - 50.0;
  pgp->GetYaxis()->SetRangeUser(ymin, ymax);
  res.setFloat("pedMin", pedMin);
  res.setFloat("pedMax", pedMax);
  res.setGraph(gnamp, pgp);
  return res;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::processAll() {
  const string myname = "FembTestAnalyzer::processAll: ";
  if ( allResult.haveInt("ncha") ) return allResult;
  Index ncha = nChannel();
  allResult.setInt("ncha", ncha);
  map<string,TH1*> hists;
  // Histograms for area/height and pos/neg combos.
  TH1* phsatMin = nullptr;
  TH1* phsatMax = nullptr;
  bool useBoth = true;
  vector<string> posValues;
  if ( useBoth ) {
    posValues.push_back("Both");
  } else {
    posValues.push_back("Neg");
    posValues.push_back("Pos");
  }
  // Loop over types (height and area)
  for ( bool useArea : {false, true} ) {
    string sarea = useArea ? "Area" : "Height";
    // Loop over signs (pos, neg or both)
    for ( string spos : posValues ) {
      string hnam = "hgain" + sarea + spos;
      string httl = sarea + " " + spos + " Gain; Channel; Gain [ADC/ke]";
      TH1* phg = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
      hists[hnam] = phg;
      // Deviation histograms.
      string hnamRms = "hall" + sarea + spos + "Rms";
      string hnamArs = "hall" + sarea + spos + "Ars";
      string hnamGms = "hall" + sarea + spos + "Gms";
      TH1* phRms = nullptr;
      TH1* phArs = nullptr;
      TH1* phGms = nullptr;
      // Histogram saturation for negative pulse height.
      if ( !useArea ) {
        hnam = "hsatMin" + sarea + spos;
        httl = sarea + " " + spos + " Lower saturation level; Channel; Saturation level [ADC]";
        phsatMin = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
        hists[hnam] = phsatMin;
        hnam = "hsatMax" + sarea + spos;
        httl = sarea + " " + spos + " Upper saturation level; Channel; Saturation level [ADC]";
        phsatMax = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
        hists[hnam] = phsatMax;
      }
      // Loop over channels.
      for ( Index icha=0; icha<ncha; ++icha ) {
cout << myname << "Channel " << icha << endl;
        string fnam = "fitGain" + sarea + spos;
        string fenam = fnam + "Unc";
        const DataMap& resc = processChannel(icha);
        phg->SetBinContent(icha+1, resc.getFloat(fnam));
        phg->SetBinError(icha+1, resc.getFloat(fenam));
        if ( phsatMin != nullptr ) {
          fnam = "fitSaturationMinHeight" + spos;
          if ( resc.haveFloat(fnam) ) {
            phsatMin->SetBinContent(icha+1, resc.getFloat(fnam));
          }
        }
        if ( phsatMax != nullptr ) {
          fnam = "fitSaturationMaxHeight" + spos;
          if ( resc.haveFloat(fnam) ) {
            phsatMax->SetBinContent(icha+1, resc.getFloat(fnam));
          }
        }
        // Fill RMS histos.
        string hnamChaRms = "hchaResp" + sarea + spos + "Rms";
        string hnamChaArs = "hchaResp" + sarea + spos + "Ars";
        string hnamChaGms = "hchaResp" + sarea + spos + "Gms";
        TH1* pheRms = resc.getHist(hnamChaRms);
        TH1* pheArs = resc.getHist(hnamChaArs);
        TH1* pheGms = resc.getHist(hnamChaGms);
        if ( phRms == nullptr && pheRms != nullptr ) {
          phRms = dynamic_cast<TH1*>(pheRms->Clone(hnamRms.c_str()));
          string httl = sarea + " " + spos + " local RMS";
          phRms->SetTitle(httl.c_str());
        } else {
          phRms->Add(pheRms);
        }
        if ( phArs == nullptr && pheArs != nullptr ) {
          phArs = dynamic_cast<TH1*>(pheArs->Clone(hnamArs.c_str()));
          string httl = sarea + " " + spos + " residual";
          phArs->SetTitle(httl.c_str());
        } else {
          phArs->Add(pheArs);
        }
        if ( phGms == nullptr && pheGms != nullptr ) {
          phGms = dynamic_cast<TH1*>(pheGms->Clone(hnamGms.c_str()));
          string httl = sarea + " " + spos + " global RMS";
          phGms->SetTitle(httl.c_str());
        } else {
          phGms->Add(pheGms);
        }
      }
      hists[hnamRms] = phRms;
      hists[hnamArs] = phArs;
      hists[hnamGms] = phGms;
    }
  }
  // Pedestal histogram.
  string hnam = "hchaPed";
  ostringstream ssfemb;
  ssfemb << femb();
  string sfemb = ssfemb.str();
  string httl = "Pedestals for FEMB " + sfemb + "; Channel; Pedestal [ADC]";
  TH1* php = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
  for ( Index icha=0; icha<ncha; ++icha ) {
    const DataMap& resc = processChannel(icha);
    double ymin = resc.getFloat("pedMin");
    double ymax = resc.getFloat("pedMax");
    double y = 0.5*(ymin + ymax);
    double dy = 0.5*(ymax - ymin);
    php->SetBinContent(icha+1, y);
    php->SetBinError(icha+1, dy);
  }
  hists[hnam] = php;
  // Underflow histogram.
  // This is the ADC value at which each channel saturates for negative signals.
  hnam = "hchaAdcMin";
  httl = "ADC saturation; Channel; Lower saturation [ADC]";
  TH1* phu = nullptr;
  if ( phsatMin == nullptr ) {
    phu = dynamic_cast<TH1*>(php->Clone(hnam.c_str()));
  } else {
    phu = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
    double addfac = useBoth ? 1.0 : -1.0;
    for ( Index icha=0; icha<ncha; ++icha ) {
      double val = 0.0;
      double dval = 0.0;
      double satmin = phsatMin->GetBinContent(icha+1);
      if ( satmin ) {
        double ped = php->GetBinContent(icha+1);
        double dped = php->GetBinError(icha+1);
        double satval = ped + addfac*satmin;
        if ( satval > val ) {
          val = satval;
          dval = dped;
        }
      }
      phu->SetBinContent(icha+1, val);
      phu->SetBinError(icha+1, dval);
    }
  }
  if ( phu != nullptr ) {
    int icol = 17;
    phu->SetLineColor(icol);
    phu->SetMarkerColor(icol);
    phu->SetFillColor(icol);
    hists[hnam] = phu;
  }
  // Overflow histogram.
  // This is the ADC value at which each channel saturates for negative signals.
  hnam = "hchaAdcMax";
  httl = "ADC saturation; Channel; Upper saturation [ADC]";
  TH1* pho = nullptr;
  if ( phsatMax == nullptr ) {
    pho = dynamic_cast<TH1*>(php->Clone(hnam.c_str()));
  } else {
    pho = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
    for ( Index icha=0; icha<ncha; ++icha ) {
      double val = 4096;
      double dval = 0.0;
      double satmax = phsatMax->GetBinContent(icha+1);
      if ( satmax ) {
        double ped = php->GetBinContent(icha+1);
        double dped = php->GetBinError(icha+1);
        double satval = ped + satmax;
        if ( satval < val ) {
          val = satval;
          dval = dped;
        }
      }
      pho->SetBinContent(icha+1, val);
      pho->SetBinError(icha+1, dval);
    }
  }
  if ( pho != nullptr ) {
    int icol = 17;
    pho->SetLineColor(icol);
    pho->SetMarkerColor(icol);
    pho->SetFillColor(icol);
    hists[hnam] = pho;
  }
  // Signal deviation histograms.
  if ( isCalib() ) {
    TH1* phdev = nullptr;
    TH1* phadv = nullptr;
    for ( Index icha=0; icha<ncha; ++icha ) {
      const DataMap& resevt = processChannel(icha);
      for ( string hnam : {"hdev", "hadv"} ) {
        if ( ! resevt.haveHist(hnam) ) {
          cout << myname << "Result for channel " << icha << " does not have histogram "
               << hnam << endl;
          continue;
        }
      }
      TH1* phdevCha = resevt.getHist("hdev");
      TH1* phadvCha = resevt.getHist("hadv");
      if ( phdev == nullptr ) {
        phdev = dynamic_cast<TH1*>(phdevCha->Clone("hdev"));
        phadv = dynamic_cast<TH1*>(phadvCha->Clone("hadv"));
        for ( TH1* ph : {phdev, phadv} ) {
          ph->SetDirectory(nullptr);
          ph->SetLineWidth(2);
          ph->Reset();
        }
      }
      string sttlDev = calibName(true) + " deviation for FEMB " + sfemb;
      string sttlAdv = calibName(true) + " deviation magnitude for FEMB " + sfemb;
      phdev->SetTitle(sttlDev.c_str());
      phadv->SetTitle(sttlAdv.c_str());
      phdev->Add(phdevCha);
      phadv->Add(phadvCha);
    }
    allResult.setHist(phdev, true);
    allResult.setHist(phadv, true);
  }
  // Record histograms.
  for ( auto ient : hists ) {
    TH1* ph = ient.second;
    if ( ph == nullptr ) {
      cout << myname << "WARNING: Skipping null histogram " << ient.first << endl;
      continue;
    }
    ph->SetMinimum(0);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    allResult.setHist(ph->GetName(), ph, true);
  }
  return allResult;
}

//**********************************************************************

FembTestPulseTree* FembTestAnalyzer::pulseTree() {
  const string myname = "FembTestAnalyzer::pulseTree: ";
  if ( m_ptreePulse != nullptr ) return m_ptreePulse.get();
  if ( ! isCalib() ) {
    cout << myname << "Must have calibration for pulse tree." << endl;
    return nullptr;
  }
  ostringstream ssnam;
  ssnam << "femb_test_pulse_femb" << femb() << "_g" << gainIndex()
          << "_s" << shapingIndex()
          << "_p" << (extPulse() ? "ext" : "int");
  if ( ! extClock() ) ssnam << "_cint";
  string snam = ssnam.str() + ".root";
  m_ptreePulse.reset(new FembTestPulseTree(snam));
  Index ncha = nChannel();
  Index nevt = nEvent();
  FembTestPulseData data;
  data.femb = femb();
  data.gain = gainIndex();
  data.shap = shapingIndex();
  data.extp = extPulse();
  for ( Index icha=0; icha<ncha; ++icha ) {
    data.chan = icha;
    for ( Index ievt=0; ievt<nevt; ++ievt ) {
      DataMap res = processChannelEvent(icha, ievt);
      float nele = res.getFloat("nElectron");
      if ( nele == 0.0 ) continue;
      data.qexp = -0.001*nele;
      data.nsat = res.getInt("sigNUnderflowNeg");
      data.stk1 = res.getFloat("stickyFraction1Neg");
      data.stk2 = res.getFloat("stickyFraction2Neg");
      data.qcal = res.getFloatVector("roiSigCalNeg");
      data.cmea = res.getFloat("roiSigCalMeanNeg");
      data.crms = res.getFloat("roiSigCalRmsNeg");
      m_ptreePulse->fill(data);
      data.qexp = 0.001*nele;
      data.nsat = res.getInt("sigNOverflowPos");
      data.stk1 = res.getFloat("stickyFraction1Pos");
      data.stk2 = res.getFloat("stickyFraction2Pos");
      data.qcal = res.getFloatVector("roiSigCalPos");
      data.cmea = res.getFloat("roiSigCalMeanPos");
      data.crms = res.getFloat("roiSigCalRmsPos");
      m_ptreePulse->fill(data);
    }
  }
  return m_ptreePulse.get();
}

//**********************************************************************

int FembTestAnalyzer::writeCalibFcl() {
  const string myname = "FembTestAnalyzer::writeCalibFcl: ";
  if ( ! isNoCalib() ) {
    cout << myname << "Invalid calibration option: " << optionName() << endl;
    return 1;
  }
  ostringstream ssfemb;
  ssfemb << femb();
  string sfemb = ssfemb.str();
  string sclass = "FembLinearCalibration";
  string baseName = "calibFromFemb";
  string toolName = baseName + sfemb;
  string fclName = baseName + "/" + toolName + ".fcl";
  ofstream fout(fclName.c_str());
  ostringstream ssgain, ssamin;
  string gainName = "fitGainHeightBoth";
  vector<string> aminNames = {"adcminWithPed", "lowSaturatedRawAdcMax"};
  vector<string> checkNames = aminNames;
  checkNames.push_back(gainName);
  for ( Index icha=0; icha<nChannel(); ++icha ) {
    const DataMap& res = processChannel(icha);
    for ( string name : checkNames ) {
      if ( ! res.haveFloat(name) ) {
        cout << myname << "Result does not have float " << name
             << " for channel " << icha << endl;
        return 1;
      }
    }
    if ( icha ) {
      ssgain << ",";
      ssamin << ",";
    }
    if ( 10*(icha/10) == icha ) {
      ssgain << "\n    ";
      ssamin << "\n    ";
    } else {
      ssgain << " ";
      ssamin << " ";
    }
    float gain = res.getFloat(gainName);
    float gaininv = gain > 0.0 ? 1.0/gain : 0.0;
    ssgain << gaininv;
    float adcMin = 0;
    for ( string aminName : aminNames ) {
      float val = res.getFloat(aminName);
      if ( val > adcMin ) adcMin = val;
    }
    ssamin << int(adcMin + 2);
  }
  fout << "tools." << toolName << ": {" << endl;
  fout << "  tool_type: " << sclass << endl;
  fout << "  LogLevel: 0" << endl;
  fout << "  Units: ke" << endl;
  fout << "  FembID: " << femb() << endl;
  fout << "  Gains: [";
  fout << ssgain.str() << endl;
  fout << "  ]" << endl;
  fout << "  AdcMin: 0" << endl;
  fout << "  AdcMins: [";
  fout << ssamin.str() << endl;
  fout << "  ]" << endl;
  fout << "  AdcMax: 4095" << endl;
  fout << "  AdcMaxs: []" << endl;
  fout << "}" << endl;
  cout << myname << "Calibration written to " << fclName << endl;
  return 0;
}

//**********************************************************************

TPadManipulator* FembTestAnalyzer::draw(string sopt, int icha, int ievt) {
  const string myname = "FembTestAnalyzer::draw: ";
  if ( sopt == "help" ) {
    cout << myname << "     draw(\"raw\", ich, iev) - Raw ADC data for channel ich event iev" << endl;
    cout << myname << "     draw(\"ped\", ich, iev) - Pedestal for channel ich event iev" << endl;
    cout << myname << "          draw(\"ped\", ich) - Pedestal vs. Qin for channel ich" << endl;
    cout << myname << "               draw(\"ped\") - Pedestal vs. channel" << endl;
    cout << myname << "            draw(\"pedlim\") - Pedestal vs. channel with ADC ranges" << endl;
    cout << myname << "              draw(\"rmsh\") - Height deviations." << endl;
    cout << myname << "              draw(\"rmsa\") - Height deviations." << endl;
    cout << myname << "             draw(\"gainh\") - Height gains vs. channel" << endl;
    cout << myname << "             draw(\"gaina\") - Area gains vs. channel" << endl;
    cout << myname << "               draw(\"dev\") - ADC calibration deviations" << endl;
    cout << myname << "               draw(\"adv\") - ADC calibration deviation magnitudes" << endl;
    cout << myname << "        draw(\"resph\", ich) - ADC height vs. Qin for channel ich" << endl;
    cout << myname << "        draw(\"respa\", ich) - ADC area vs. Qin for channel ich" << endl;
    cout << myname << "     draw(\"respresh\", ich) - ADC height residual vs. Qin for channel ich" << endl;
    cout << myname << "     draw(\"respresa\", ich) - ADC area residual vs. Qin for channel ich" << endl;
    cout << myname << "          draw(\"dev\", ich) - ADC calibration deviations for channel ich" << endl;
    cout << myname << "          draw(\"adv\", ich) - ADC calibration deviation magnitudes for channel ich" << endl;
    cout << myname << "  draw(\"resphp\", ich, iev) - ADC positive height distribution for channel ich event iev" << endl;
    cout << myname << "  draw(\"resphn\", ich, iev) - ADC negative height distribution for channel ich event iev" << endl;
    cout << myname << "  draw(\"respap\", ich, iev) - ADC positive area distribution for channel ich event iev" << endl;
    cout << myname << "  draw(\"respan\", ich, iev) - ADC negative area distribution for channel ich event iev" << endl;
    cout << myname << "    draw(\"devp\", ich, iev) - Deviation from calibration for channel ich event iev" << endl;
    cout << myname << "    draw(\"devn\", ich, iev) - Deviation from calibration for channel ich event iev" << endl;
    return nullptr;
  }
  //using Fun = TPadManipulator* (*)(TPadManipulator&);
  const int wx = 700;
  const int wy = 500;
  string mnam = sopt;
  if ( icha >= 0 ) {
    ostringstream ssopt;
    ssopt << mnam << "_ch" << icha;
    mnam = ssopt.str();
  }
  if ( ievt >= 0 ) {
    ostringstream ssopt;
    ssopt << mnam << "_ev" << ievt;
    mnam = ssopt.str();
  }
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  ostringstream ssfemb;
  ssfemb << femb();
  string sfemb = ssfemb.str();
  // Create sample label.
  TLatex* plab = new TLatex(0.01, 0.015, reader()->label().c_str());
  plab->SetNDC();
  plab->SetTextSize(0.035);
  plab->SetTextFont(42);
  TPadManipulator& man = m_mans[mnam];
  man.add(plab, "");
  man.addAxis();
  // Plots 
  if ( icha < 0 && ievt < 0 ) {
    // Pedestal vs. channel
    if ( sopt == "ped" ) {
      TH1* ph = processAll().getHist("hchaPed");
      if ( ph == nullptr ) {
      cout << myname << "Unable to find pedestal histogram hchaPed in this result:" << endl;
        processAll().print();
      } else {
        man.add(ph, "P");
        man.setRangeY(0, 4100);
        man.addVerticalModLines(16);
        return draw(&man);
      }
    // Pedestal vs. channel with ADC ranges
    } else if ( sopt == "pedlim" ) {
      TH1* ph1 = processAll().getHist("hchaAdcMin");
      TH1* ph2 = processAll().getHist("hchaAdcMax");
      TH1* php = processAll().getHist("hchaPed");
      if ( php == nullptr || ph1 == nullptr || ph2 == nullptr ) {
        cout << myname << "Unable to find hchaAdcMin, hchaAdcMax or hchaPed in this result:" << endl;
        processAll().print();
      } else if ( int rstat = man.add(ph2, "H") ) {
        cout << myname << "Manipulator returned error " << rstat << endl;
      } else {
        //ph1->SetFillStyle(3001);
        man.add(ph1, "H same");
        man.add(php, "P same");
        man.hist()->SetFillColor(10);
        man.hist()->SetLineColor(10);
        man.hist()->SetLineWidth(0);
        string sttl = "ADC pedestals and range for FEMB " + sfemb;
        man.hist()->SetTitle(sttl.c_str());
        man.hist()->GetYaxis()->SetTitle("ADC count");
        man.setFrameFillColor(17);
        man.setRangeY(0, 4100);
        man.addVerticalModLines(16);
        return draw(&man);
      }
    // Gain vs. channel.
    } else if ( sopt == "gainh" || sopt == "gaina" ) {
      bool useArea = sopt == "gaina";
      string sarea = useArea ? "area" : "height";
      string hname = useArea ? "hgainAreaBoth" : "hgainHeightBoth";
      TH1* ph = processAll().getHist(hname);
      if ( ph == nullptr ) {
        cout << myname << "Unable to find histogram " << hname << " in result:" << endl;
        processAll().print();
      } else if ( int rstat = man.add(ph, "H") ) {
        cout << myname << "Manipulator returned error " << rstat << endl;
      } else {
        string sttl = "ADC " + sarea + " gains for FEMB " + sfemb;
        man.hist()->SetTitle(sttl.c_str());
        man.addVerticalModLines(16);
        double gnom = 1000.0*preampGain()*adcGain()/elecPerFc();  // Nominal gain [ADC/ke]
        if ( useArea ) {
          gnom *= 8.0*shapingTime()/3.0;
        }
        if ( gnom > 0.0 ) {
          double ymax = int(1.4*gnom + 0.5);
          man.setRangeY(0.0, ymax);
          man.addHorizontalLine(gnom, 1.0, 3);
        }
        return draw(&man);
      }
    // Deviations.
    } else if ( sopt == "rmsh" || sopt == "rmsa" ) {
      string hnamdev = sopt == "rmsh" ? "hallHeightBothGms" : "hallAreaBothGms";
      string hnamrms = sopt == "rmsh" ? "hallHeightBothRms" : "hallAreaBothRms";
      string hnamars = sopt == "rmsh" ? "hallHeightBothArs" : "hallAreaBothArs";
      vector<string> missing;
      for ( string name : {hnamdev, hnamrms, hnamars} )
        if ( ! processAll().haveHist(name) ) missing.push_back(name);
      if ( missing.size() ) {
        cout << myname << "Unable to find histogram" << ( missing.size() ==1 ? "" : "s") << ":" << endl;
        for ( string name : missing ) cout << myname << "  " << name << endl;
        return nullptr;
      }
      TH1* phdev = processAll().getHist(hnamdev);
      TH1* phrms = processAll().getHist(hnamrms);
      TH1* phars = processAll().getHist(hnamars);
      man.add(phdev);
      man.add(phrms, "same");
      man.add(phars, "same");
      phdev = man.getHist(hnamdev);
      phrms = man.getHist(hnamrms);
      phars = man.getHist(hnamars);
      double ymax = phdev->GetMaximum();
      if ( phrms->GetMaximum() > ymax ) ymax = phrms->GetMaximum();
      if ( phars->GetMaximum() > ymax ) ymax = phars->GetMaximum();
      man.setRangeY(0, 1.05*ymax);
      phrms->SetLineStyle(3);
      phars->SetLineStyle(2);
      man.showOverflow();
      // Add legend.
      TLegend* pleg = man.addLegend(0.60, 0.73, 0.93, 0.88);
      ostringstream ssrms;
      ostringstream ssars;
      ostringstream ssdev;
      float meanRms = phrms->GetMean();
      float meanArs = phars->GetMean();
      float meanDev = phdev->GetMean();
      ssrms << "Local RMS ("    << setprecision(2) << fixed << meanRms << " ke)";
      ssars << "|<residual>| (" << setprecision(2) << fixed << meanArs << " ke)";
      ssdev << "Global RMS ("   << setprecision(2) << fixed << meanDev << " ke)";
      pleg->AddEntry(phrms, ssrms.str().c_str(), "l");
      pleg->AddEntry(phars, ssars.str().c_str(), "l");
      pleg->AddEntry(phdev, ssdev.str().c_str(), "l");
      // Set title.
      ostringstream ssttl;
      ssttl << (sopt == "rmsh" ? "Height" : "Area");
      ssttl << " deviations for FEMB " << femb();
      man.hist()->SetTitle(ssttl.str().c_str());
      return draw(&man);
    } else if ( sopt == "dev" || sopt == "adv" ) {
      if ( ! isCalib() ) {
        cout << myname << "Drawing " << sopt << " is only available for calibrated samples." << endl;
        return nullptr;
      }
      string hnam = "h" + sopt;
      TH1* ph = processAll().getHist(hnam);
      if ( ph == nullptr ) {
        cout << myname << "Unable to find histogram " << hnam << ". " << endl;
        return nullptr;
      }
      man.add(ph);
      if ( sopt == "dev" ) man.setRangeX(-5.0, 5.0);
      man.showUnderflow();
      man.showOverflow();
      return draw(&man);
    }
  // Plots for channel icha
  } else if ( icha >= 0 && ievt < 0 ) {
    if ( sopt == "ped" ) {
      const DataMap& res = processChannel(icha);
      TGraph* pg = res.getGraph("gchaPeds");
      float pedMin = res.getFloat("pedMin");
      float pedMax = res.getFloat("pedMax");
      if ( pg != nullptr ) {
        man.add(pg, "P*");
        int iymin = (pedMin -  2.0)/10;
        int iymax = (pedMax + 12.0)/10;
        int ny = iymax - iymin;
        float ymin = 10*iymin;
        float ymax = 10*iymax;
        while ( ny < 10 ) {
          if ( fmod(ymax, pedMax) < fmod(pedMin, ymin) ) ymax += 10.0;
          else ymin -= 10.0;
          ++ny;
        }
        int npt = pg->GetN();
        float x1 = pg->GetX()[0];
        float x2 = pg->GetX()[npt-1];
        float dx = 0.04*(x2 - x1);
        float sx = 10.0;
        float xmin = 10.0*int(x1/10) + sx;
        float xmax = 10.0*int(x2/10) - sx;
        while ( x1 - xmin < dx ) xmin -= sx;
        while ( xmax - x2 < dx ) xmax += sx;
        man.setRangeX(xmin, xmax);
        man.setRangeY(ymin, ymax);
        man.addHorizontalModLines(10);
        return draw(&man);
      }
    } else if ( sopt == "resph" || 
                sopt == "respa" ||
                sopt == "respresh" ||
                sopt == "respresa" ) {
      vector<string> gnams;
      double ymin = -1500;
      double ymax = 3000;
      if ( isHeightCalib() ) {
        ymin = -200;
        ymax = 400;
      }
      if ( sopt == "resph" ) gnams = {"gchaRespHeightBothFitPointsUnc", "gchaRespHeightBothFitPointsUnc"};
      if ( sopt == "respa" ) {
        gnams = {"gchaRespAreaBothFitPointsUnc", "gchaRespAreaBoth"};
        if ( isNoCalib() ) {
          ymin = -15000;
          ymax = 25000;
        } else {
          ymin = -1000;
          ymax = 2000;
        }
      }
      if ( sopt == "respresh" ) {
        gnams = {"gchaRespHeightBothRes"};
        ymin = -5;
        ymax = 5;
      }
      if ( sopt == "respresa" ) {
        gnams = {"gchaRespAreaBothRes"};
        ymin = -5;
        ymax = 5;
      }
      Index ngrf = gnams.size();
      const DataMap& res = processChannel(icha);
      vector<TGraph*> pgs;
      for ( string gnam : gnams ) pgs.push_back(res.getGraph(gnam));
      int nbad = 0;
      for ( Index igrf=0; igrf<ngrf; ++igrf ) {
        if ( pgs[igrf] == nullptr ) {
          cout << myname << "Unable to find graph " << gnams[igrf] << " for result:" << endl;
          ++nbad;
        }
      }
      if ( nbad ) {
        res.print();
      } else {
        man.add(pgs[0], "P");
        for ( Index igrf=1; igrf<ngrf; ++igrf ) man.add(pgs[igrf], "P");
        man.setRangeY(ymin, ymax);
        man.setGrid();
        man.addHorizontalLine();
        man.addVerticalLine();
        return draw(&man);
      }
    } else if ( sopt == "dev" || 
                sopt == "adv" ) {
      if ( ! isCalib() ) {
        cout << myname << "Drawing " << sopt << " is only available for calibrated samples." << endl;
        return nullptr;
      }
      const DataMap& res = processChannel(icha);
      string hnam = "h" + sopt;
      if ( ! res.haveHist(hnam) ) {
        cout << myname << "Unable to find histogram " << hnam << " for channel " << icha << "." << endl;
        return nullptr;
      }
      TH1* ph = res.getHist(hnam);
      man.add(ph, "");
      man.showUnderflow();
      man.showOverflow();
      if ( sopt == "dev" ) {
        man.setRangeX(-5.0, 5.0);
      }
      return draw(&man);
    }
  // Plots for event ievt in channel icha
  } else if ( icha >= 0 && ievt >= 0 ) {
    if ( sopt == "raw" ) {
      const DataMap& res = processChannelEvent(icha, ievt);
      TH1* ph = res.getHist("raw");
      if ( ph != nullptr ) {
        man.add(ph, "");
        man.setCanvasSize(1400, 500);
        man.addHorizontalModLines(64);
        return draw(&man);
      }
    } else if ( sopt == "prep" ) {
      const DataMap& res = processChannelEvent(icha, ievt);
      TH1* ph = res.getHist("prepared");
      if ( ph != nullptr ) {
        man.add(ph, "");
        man.setCanvasSize(1400, 500);
        if ( isNoCalib() ) {
          float xoff = -res.getFloat("pedestal");
          man.addHorizontalModLines(64, xoff);
        }
        return draw(&man);
      }
    } else if ( sopt == "ped" ) {
      const DataMap& res = processChannelEvent(icha, ievt);
      TH1* ph = res.getHist("pedestal");
      if ( ph != nullptr ) {
        man.add(ph, "");
        man.addHistFun(1);
        man.addHistFun(0);
        man.addVerticalModLines(64);
        return draw(&man);
      }
    } else if ( sopt == "resphp" || sopt == "resphn" ||
                sopt == "respap" || sopt == "respan" ) {
      const DataMap& res = processChannelEvent(icha, ievt);
      string hname = sopt == "resphp" ? "hsigHeightPos" :
                     sopt == "resphn" ? "hsigHeightNeg" :
                     sopt == "respap" ? "hsigAreaPos" :
                     sopt == "respan" ? "hsigAreaNeg" :
                                        "NoSuchHist";
      TH1* ph = res.getHist(hname);
      if ( ph != nullptr ) {
        man.add(ph, "");
        if ( sopt == "resphp" || sopt == "resphn" ) {
          if ( isNoCalib() ) {
            float xoff = res.getFloat("pedestal");
            if ( sopt == "resphp" ) xoff *= -1.0;
            man.addVerticalModLines(64, xoff);
          }
        }
        return draw(&man);
      }
    } else if ( sopt == "devp" || sopt == "devn" ) {
      const DataMap& res = processChannelEvent(icha, ievt);
      string hname = sopt == "devp" ? "hsigDevPos" :
                     sopt == "devn" ? "hsigDevNeg" :
                                      "NoSuchHist";
      TH1* ph = res.getHist(hname);
      if ( ph != nullptr ) {
        man.add(ph, "");
        man.setRangeX(-5, 5);
        man.showUnderflow();
        man.showOverflow();
        return draw(&man);
      }
    }
  }
  cout << myname << "Invalid plot name: " << mnam << endl;
  m_mans.erase(mnam);
  return nullptr;
}

//**********************************************************************

TPadManipulator* FembTestAnalyzer::drawAdc(string sopt, int iadc, int ievt) {
  const string myname = "FembTestAnalyzer::drawAdc: ";
  ostringstream ssnam;
  if ( iadc < 0 || iadc > 7 ) return nullptr;
  ssnam << "adc" << iadc << "_" << sopt;
  string mnam = ssnam.str();
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  TPadManipulator& man = m_mans[mnam];
  int wy = gROOT->IsBatch() ? 2000 : 1000;
  man.setCanvasSize(1.4*wy, wy);
  man.split(4);
  bool doDrawSave = doDraw();
  setDoDraw(false);
  for ( Index kcha=0; kcha<16; ++kcha ) {
    Index icha = 16*iadc + kcha;
    TPadManipulator* pman = draw(sopt, icha, ievt);
    if ( pman == nullptr ) {
      cout << myname << "Unable to find plot " << sopt << " for channel " << icha << endl;
      m_mans.erase(mnam);
      setDoDraw(doDrawSave);
      return nullptr;
    }
    *man.man(kcha) = *pman;
  }
  setDoDraw(doDrawSave);
  return draw(&man);
}

//**********************************************************************

TPadManipulator* FembTestAnalyzer::draw(TPadManipulator* pman) const {
  if ( doDraw() ) pman->draw();
  return pman;
}

//**********************************************************************
