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
#include "TSystem.h"

using std::string;
using std::cout;
using std::endl;
using std::map;
using std::setprecision;
using std::fixed;
using std::ofstream;

//**********************************************************************

FembTestAnalyzer::Index FembTestAnalyzer::typeIndex(SignOption isgn, bool useArea) {
  if ( isgn == OptBothSigns ) {
    if ( useArea ) return 1;
    else return 0;
  } else if ( isgn == OptPositive ) {
    if ( useArea ) return 3;
    else return 2;
  } else if ( isgn == OptNegative ) {
    if ( useArea ) return 5;
    else return 4;
  }
  return typeSize();
}

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int opt, int a_femb, int a_gain, int a_shap, 
                 std::string a_tspat, bool a_isCold,
                 bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(opt, a_femb, a_tspat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock);
  getTools();
}

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int opt, string dir, string fpat,
                 int a_femb, int a_gain, int a_shap, 
                 bool a_isCold, bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(opt, a_femb, fpat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock, dir);
  getTools();
}

//**********************************************************************

FembTestAnalyzer::FembTestAnalyzer(int opt, int a_femb, string a_tspat, bool a_isCold)
: m_copt(CalibOption(opt%10)), m_ropt(RoiOption((opt%100)/10)), m_doDraw(opt>99),
  m_femb(a_femb), m_tspat(a_tspat), m_isCold(a_isCold),
  m_ptreePulse(nullptr), m_tickPeriod(0),
  m_nChannelEventProcessed(0) {
  const string myname = "FembTestAnalyzer::ctor: ";
  cout << myname << "  Calib option: " << calibOptionName() << endl;
  cout << myname << "    ROI option: " << roiOptionName() << endl;
  cout << myname << "       Do draw: " << (m_doDraw ? "true" : "false") << endl;
  adcModifierNames.push_back("adcPedestalFit");
  string roiPeakName;
  if ( isNoCalib() ) {
    adcModifierNames.push_back("adcSampleFiller");
    roiPeakName = "adcThresholdSignalFinder";
  }
  if ( isHeightCalib() ) {
     adcModifierNames.push_back("fembCalibratorG%GAIN%S%SHAP%%WARM%");
     roiPeakName = "keThresholdSignalFinder";
  }
  if ( doPeakRoi() ) {
    adcModifierNames.push_back(roiPeakName);
  } else if ( doTickModRoi() ) {
    adcModifierNames.push_back("tickModSignalFinder");
  }
  if ( doRoi() ) adcViewerNames.push_back("adcRoiViewer");
  adcViewerNames.push_back("adcPlotRaw");
  adcViewerNames.push_back("adcPlotRawDist");
  adcViewerNames.push_back("adcPlotPrepared");
}

//**********************************************************************

void FembTestAnalyzer::getTools() {
  const string myname = "FembTestAnalyzer::getTools: ";
  DuneToolManager* ptm = DuneToolManager::instance("dunefemb.fcl");
  if ( ptm == nullptr ) {
    cout << myname << "Unable to retrieve tool manager." << endl;
    return;
  }
  fixToolNames(adcModifierNames);
  for ( string modname : adcModifierNames ) {
    std::unique_ptr<AdcChannelDataModifier> pmod =
      ptm->getPrivate<AdcChannelDataModifier>(modname);
    if ( ! pmod ) {
      cout << myname << "Unable to find modifier " << modname << endl;
      adcModifiers.clear();
      return;
    }
    adcModifiers.push_back(std::move(pmod));
  }
  fixToolNames(adcViewerNames);
  for ( string vwrname : adcViewerNames ) {
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

void FembTestAnalyzer::fixToolNames(vector<string>& names) const {
  map<string, string> subs;
  ostringstream ssgain;
  ssgain << gainIndex();
  subs["%GAIN%"] = ssgain.str();
  ostringstream ssshap;
  ssshap << shapingIndex();
  subs["%SHAP%"] = ssshap.str();
  subs["%WARM%"] = isCold() ? "" : "Warm";
  for ( string& name : names ) {
    for ( auto ent : subs ) {
      string sold = ent.first;
      string snew = ent.second;
      string::size_type ipos = 0;
      while ( ipos < sold.size() ) {
        ipos = name.find(sold, ipos);
        if ( ipos == string::npos ) break;
        name.replace(ipos, sold.size(), snew);
        ipos += snew.size();
      }
    }
  }
}

//**********************************************************************

int FembTestAnalyzer::
find(int a_gain, int a_shap, bool a_extPulse, bool a_extClock, string dir) {
  const string myname = "FembTestAnalyzer::find: ";
  DuneFembFinder fdr;
  chanevtResults.clear();
  chanResults.clear();
  chanResponseResults.clear();
  cout << myname << "Fetching reader." << endl;
  if ( dir.size() ) {
    m_reader = std::move(fdr.find(dir, tspat()));
  } else {
    m_reader = std::move(fdr.find(femb(), isCold(), tspat(), a_gain, a_shap, a_extPulse, a_extClock));
  }
  cout << myname << "Done fetching reader." << endl;
  chanevtResults.resize(nChannel(), vector<DataMap>(nEvent()));
  chanResults.resize(nChannel());
  chanResponseResults.resize(typeSize(), vector<DataMap>(nChannel()));
  return 0;
}

//**********************************************************************

int FembTestAnalyzer::setTickPeriod(Index val) {
  const string myname = "FembTestAnalyzer::setTickPeriod: ";
  if ( nChannelEventProcessed() != 0 ) {
    cout << myname << "Period cannot be set after processing has begun." << endl;
    return 1;
  }
  m_tickPeriod = val;
  return 0;
}

//**********************************************************************

string FembTestAnalyzer::calibOptionName() const {
  if ( isNoCalib()     ) return "OptNoCalib";
  if ( isHeightCalib() ) return "OptHeightCalib";
  if ( isAreaCalib()   ) return "OptAreaCalib";
  return "OptUnknownCalib";
}

//**********************************************************************

string FembTestAnalyzer::roiOptionName() const {
  if ( ! doRoi()      ) return "OptNoRoi";
  if ( doPeakRoi() )    return "OptPeakRoi";
  if ( doTickModRoi() ) return "OptTickModRoi";
  return "OptUnknownRoi";
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

bool FembTestAnalyzer::doResponseFit() const {
  return doRoi();
}

//**********************************************************************

string FembTestAnalyzer::responseFitFunctionName(SignOption isgn) const {
  if ( isgn == OptNoSign ) return "fnone";
  string sfit = "fgain";
  if ( isNoCalib() ) {
    if      ( isgn == OptPositive )  sfit = "fgain";
    else if ( isgn == OptNegative ) sfit = "fgainMax";
    else if ( isgn == OptBothSigns ) {
      if ( reader()->extPulse() ) {
        sfit = "fgainMin";
      } else if ( reader()->intPulse() ) {
        sfit = "fgainOffMin";
      }
    }
  }
  return sfit;
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
  if ( ! haveTools() ) {
    cout << myname << "ADC processing tools are missing." << endl;
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
  ++m_nChannelEventProcessed;
  res.setInt("event", ievt);
  if ( reader() == nullptr ) return res.setStatus(1);
  // Fetch the expected charge for this sample.
  double pulseQfC = chargeFc(ievt);
  double pulseQe = pulseQfC*elecPerFc();
  res.setFloat("nElectron", pulseQe);
  // Read the raw data.
  AdcChannelData acd;
  acd.run = femb();
  acd.event = ievt;
  acd.channel = icha;
  acd.fembID = femb();
  reader()->read(ievt, icha, &acd);
  // Process the data, i.e. subtract pedestal, calibrate, find ROIs, etc.
  DataMap resmod;
  if ( dbg > 2 ) cout << myname << "Applying modifiers." << endl;
  Index imod = 0;
  for ( const std::unique_ptr<AdcChannelDataModifier>& pmod : adcModifiers ) {
    string modName = adcModifierNames[imod];
    // For the tickmod ROI builder, if we have a tick period, add it to the channel data.
    if ( modName == "tickModSignalFinder" && tickPeriod() > 0 ) {
      acd.rois.clear();
      acd.rois.emplace_back(0, tickPeriod()-1);
    }
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
    cout << myname << "ADC data:"  << endl;
    cout << myname << "     raw: size=" << acd.raw.size()     << endl;
    cout << myname << " samples: size=" << acd.samples.size() << endl;
    cout << myname << "   flags: size=" << acd.flags.size()   << endl;
    cout << myname << "  signal: size=" << acd.signal.size()  << endl;
    cout << myname << "    rois: size=" << acd.rois.size()    << endl;
    cout << myname << "----------------------------------" << endl;
  }
  if ( dbg > 2 ) cout << myname << "Applying viewers." << endl;
  for ( Index ivwr=0; ivwr<adcViewers.size(); ++ivwr ) {
    string vwrName = adcViewerNames[ivwr];
    const std::unique_ptr<AdcChannelViewer>& pvwr = adcViewers[ivwr];
    if ( dbg > 2 ) cout << "Applying viewer " << vwrName << endl;
    DataMap resvwr = pvwr->view(acd);
    resmod += resvwr;
    if ( dbg > 2 ) cout << "Viewer returned status " << resvwr.status() << endl;
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
  // Fill the tickmod tree.
  // This includes finding the tickmods with the min and max signals.
  if ( tickPeriod() > 0 ) {
    FembTestTickModTree& ftt = *tickModTree(false);
    ftt.addTree(icha);
    ftt.clear();
    ftt.data().femb = femb();
    ftt.data().gain = gainIndex();
    ftt.data().shap = shapingIndex();
    ftt.data().extp = extPulse();;
    ftt.data().ntmd = tickPeriod();
    ftt.data().ped0 = ievt==0 ? acd.pedestal : processChannelEvent(icha, 0).getFloat("pedestal");
    ftt.data().qexp = 0.001*pulseQe;
    ftt.data().ievt = ievt;
    DataMap rest = ftt.fill(acd);
    res.extend(rest);
  }
  // Process the ROIs.
  bool haverois = resmod.haveInt("roiCount");
  Index nroi = haverois ? resmod.getInt("roiCount") : 0;
  if ( nroi ) {
    vector<float> sigAreas[2];   // Calibrated area for each signal
    vector<float> sigHeights[2]; // Calibrated height for each signal
    vector<float> sigCals[2];    // Calibrated charge (height or area depending on calib option)
    vector<float> sigDevs[2];    // Deviation of calibrated value from expected value
    int sigNundr[2] = {0, 0};
    int sigNover[2] = {0, 0};
    float areaMin[2] = {1.e9, 1.e9};
    float areaMax[2] = {0.0, 0.0};
    float heightMin[2] = {1.e9, 1.e9};
    float heightMax[2] = {0.0, 0.0};
    // Set range of ROIs.
    Index iroi1 = 0;
    Index iroi2 = nroi;
    // Remove edge ROIs for peak ROIs.
    if ( doPeakRoi() ) {
      Index firstRoiFirstTick = resmod.getIntVector("roiTick0s")[0];
      Index lastRoiLastTick = resmod.getIntVector("roiTick0s")[nroi-1]
                            + resmod.getIntVector("roiNTicks")[nroi-1];
      if ( firstRoiFirstTick <= 0 ) ++iroi1;
      if ( lastRoiLastTick >= resmod.getInt("roiNTickChannel") ) --iroi2;
    }
    // Remove the short ROI for tickmod ROIs.
    if ( doTickModRoi() ) {
      if ( resmod.getIntVector("roiNTicks")[iroi2-1] != tickPeriod() ) --iroi2;
    }
    // Flag indicationg if we have a calibrated value.
    // If so, we will record calibrated values and their deviations from the
    // true peak charge.
    bool doCalib = isHeightCalib() || (isAreaCalib() && doPeakRoi());
    // Loop over ROIs and fetch the height and area.
    // Also count the number of ROIs with under and overflow bins.
    // All are done separately for each sign.
    float expSig = 0.001*pulseQe;
    vector<bool> roiIsPos(nroi, false);
    if ( dbg >= 3 ) cout << myname << "Processing " << nroi << " ROIs." << endl;
    for ( Index iroi=iroi1; iroi<iroi2; ++iroi ) {
      if ( dbg >= 4 ) cout << myname << "Processing ROI " << iroi << endl;
      float area = resmod.getFloatVector("roiSigAreas")[iroi];
      float sigmin = resmod.getFloatVector("roiSigMins")[iroi];
      float sigmax = resmod.getFloatVector("roiSigMaxs")[iroi];
      int nundr = resmod.getIntVector("roiNUnderflows")[iroi];
      int nover = resmod.getIntVector("roiNOverflows")[iroi];
      if ( dbg >= 5 ) {
        cout << myname << "  Min height: " << sigmin << endl;
        cout << myname << "  Max height: " << sigmax << endl;
        cout << myname << "        Area: " << area << endl;
      }
      vector<bool> isPoss;
      // Build vector of signs.
      // For peak ROIs, there is only one based on the area.
      // For tickmod ROIS, we have both signs
      if ( doPeakRoi() ) isPoss.push_back(area >= 0.0);
      else if ( doTickModRoi() ) {
        isPoss.push_back(false);
        isPoss.push_back(true);
      }
      // Loop over signs.
      for ( bool isPos : isPoss ) {
        roiIsPos[iroi] = isPos;
        float sign = isPos ? 1.0 : -1.0;
        area *= sign;
        float height = isPos ? sigmax : -sigmin;
        if ( nundr > 0 ) ++sigNundr[isPos];
        if ( nover > 0 ) ++sigNover[isPos];
        // Area only for peak ROIs (for now)
        if ( doPeakRoi() ) {
          sigAreas[isPos].push_back(area);
          if ( area < areaMin[isPos] ) areaMin[isPos] = area;
          if ( area > areaMax[isPos] ) areaMax[isPos] = area;
        }
        sigHeights[isPos].push_back(height);
        if ( height < heightMin[isPos] ) heightMin[isPos] = height;
        if ( height > heightMax[isPos] ) heightMax[isPos] = height;
        // Calib value and its deviation.
        if ( doCalib ) {
          float qcal = 0.0;
          if ( isHeightCalib() ) qcal = height;
          if ( isAreaCalib() )   qcal = area;
          qcal *= sign;
          sigCals[isPos].push_back(qcal);
          float dev = -999.0;
          if ( isCalib() ) dev = sign*(qcal - sign*expSig);
          sigDevs[isPos].push_back(dev);
        }
      }
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
        int   nbin = isNoCalib() ? 100 : 100;
        float dbin = isNoCalib() ? 5.0 : 0.5;
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
        // We don't expect these for tickmod ROIs.
        if ( ! doTickModRoi() ) {
          cout << myname << "No " + ssgn + " area ROIs for channel " << icha
               << " event " << ievt << endl;
        }
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
        cout << myname << "No " + ssgn + " height ROIs for channel " << icha
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
      // Evaluate the periods.
      // The period is # ticks between each adjacent pair of peaks of the same sign.
      const DataMap::IntVector& roiTick0s = resmod.getIntVector("roiTick0s");
      if ( roiTick0s.size()) {
        vector<int> roiPeriods;
        string vecname = isgn ? "roiTickMaxs" : "roiTickMins";
        const DataMap::IntVector& roiTicks = resmod.getIntVector(vecname);
        Index tickLast = 0;
        Index count = 0;
        map<Index, Index> countPeriod;
        Index periodMax = 0;
        for ( Index iroi=iroi1; iroi<iroi2; ++iroi ) {
          if ( roiIsPos[iroi] != isgn ) continue;
          Index tick = roiTicks[iroi] + roiTick0s[iroi];
          if ( tickLast ) {
            Index period = tick - tickLast;
            roiPeriods.push_back(period);
            if ( countPeriod.find(period) == countPeriod.end() ) countPeriod[period] = 0;
            ++countPeriod[period];
            ++count;
            if ( periodMax == 0 ) periodMax = period;
            else if ( countPeriod[period] > countPeriod[periodMax] ) periodMax = period;
          }
          tickLast = tick;
        }
        float periodMaxFraction = countPeriod[periodMax]/float(count);
        string periodName = "roiPeriod" + ssgn;
        string periodsName = "roiPeriods" + ssgn;
        string periodMaxName = "roiPeriodMaxFraction" + ssgn;
        res.setIntVector(periodsName, roiPeriods);       // All period measurements
        res.setInt(periodName, periodMax);               // Most frequent period
        res.setFloat(periodMaxName, periodMaxFraction);  // Fraction of periods with most frequent value
      }
      // Evaluate the sticky-code metrics.
      //    S1 = fraction of codes in most populated bin
      //    S2 = fraction of codes with classic sticky values
      if ( sigHeights[isgn].size() ) {
        string vecname = isgn ? "roiTickMaxs" : "roiTickMins";
        const DataMap::IntVector& roiTicks = resmod.getIntVector(vecname);
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
          if ( adcCodeMod == 63 ) stickyCount += count;
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
  } else if ( doRoi() ) {
    cout << myname << "ERROR: It appears no ROI finder was run (no roiCount in result)." << endl;
  }
  return res;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::getChannelResponse(Index icha, SignOption isgn, bool useArea) {
  const string myname = "FembTestAnalyzer::getChannelResponse: ";
  Index ityp = typeIndex(isgn, useArea);
  if ( ityp == typeSize() ) {
    static DataMap empty(1);
    return empty;
  }
  DataMap& res = chanResponseResults[ityp][icha];
  if ( res.haveInt("channel") ) return res;
  const DuneFembReader* prdr = reader();
  if ( prdr == nullptr ) {
    cout << myname << "No reader found." << endl;
    static DataMap badres(1);
    return badres;
  }
  Index nevt = nEvent();
  Index ievt0 = 0;  // Event with zero charge, i.e. ievt0+1 is one unit.
  if ( !prdr->extPulse()  && !prdr->intPulse() ) {
    cout << myname << "Reader does not specify if pulse is internal or external." << endl;
    static DataMap badres(2);
    return badres;
  }
  if ( ! haveTools() ) {
    cout << myname << "ADC processing tools are missing." << endl;
    static DataMap badres(3);
    return badres;
  }
  // Note that we have to proceed without ROIs because we want to record peds and nkes
  bool pulseIsExternal = prdr->extPulse();
  if ( pulseIsExternal ) ievt0 = 1;
  bool useBoth = isgn = OptBothSigns;
  vector<bool> usePosValues;
  if ( useBoth || isgn == OptNegative ) usePosValues.push_back(false);
  if ( useBoth || isgn == OptPositive ) usePosValues.push_back(true);
  if ( usePosValues.size() == 0 ) {
    cout << myname << "Invalid value for isgn: " << isgn << endl;
    static DataMap badres(4);
    return badres;
  }
  string styp = useArea ? "Area" : "Height";
  ostringstream sscha;
  sscha << icha;
  string scha = sscha.str();
  string signLabel;
  if ( isgn == OptNegative ) signLabel = "Neg";
  if ( isgn == OptPositive ) signLabel = "Pos";
  string hnam = "hchaResp" + styp + signLabel;
  string usePosLab = useBoth ? "" : " " + signLabel;
  string gttl = styp + " response" + usePosLab + " channel " + scha;
  string gttlr = styp + " response residual" + usePosLab + " channel " + scha;
  string httl = gttl + " ; charge factor; Mean ADC " + styp;
  // Create arrays use to evaluate the response.
  // For external pulser, include (0,0).
  Index npt = prdr->extPulse() ? 1 : 0;
  npt = 0;  // Don't use the point at zero
  vector<float> x(npt, 0.0);
  vector<float> dx(npt, 0.0);
  vector<float> y(npt, 0.0);
  vector<float> dy(npt, 0.0);
  vector<float> dyFit(npt, 0.0);
  vector<float> fitPed(npt, 0.0);
  // End of response arrays
  vector<float> sticky1[2];
  vector<float> sticky2[2];
  vector<bool> fitkeep(npt, true);
  double ymax = 1000.0;
  vector<float> peds(nevt, 0.0);
  vector<float> nkeles(nevt, 0.0);
  vector<int> adcmins(nevt, -2);
  vector<int> adcmaxs(nevt, -1);
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
    if ( isCalib() ) {
      adcmins[ievt] = resevt.getInt("calibAdcMin", -1);
      adcmaxs[ievt] = resevt.getInt("calibAdcMax", -1);
    }
    int roiCount = resevt.getInt("roiCount");
    float nele = resevt.getFloat("nElectron");
    float nkele = 0.001*nele;
    nkeles[ievt] = nkele;
    if ( ievt > ievt0 && doRoi() ) {
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
          if ( doTickModRoi() ) {
            isUnderOver = usePos ? novr > 0 : nudr > 0;
          }
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
      if ( doRoi() ) {
        for ( bool usePos : usePosValues ) {
          sticky1[usePos].push_back(-3.0);
          sticky2[usePos].push_back(-3.0);
        }
      }
    }
  }
  res.setInt("channel", icha);
  res.setFloatVector("peds", peds);
  if ( isCalib() ) {
    int calibAdcMin = -1;
    for ( int adcmin : adcmins ) {
      if ( adcmin >= 0 ) {
        if ( calibAdcMin < 0 ) calibAdcMin = adcmin;
        else if ( adcmin != calibAdcMin ) {
          calibAdcMin = -2;
          break;
        }
      }
    }
    int calibAdcMax = -1;
    for ( int adcmax : adcmaxs ) {
      if ( adcmax >= 0 ) {
        if ( calibAdcMax < 0 ) calibAdcMax = adcmax;
        else if ( adcmax != calibAdcMax ) {
          calibAdcMax = -2;
          break;
        }
      }
    }
    res.setIntVector("calibAdcMins", adcmins);
    res.setIntVector("calibAdcMaxs", adcmaxs);
    res.setInt("calibAdcMin", calibAdcMin);
    res.setInt("calibAdcMax", calibAdcMax);
  }
  res.setFloatVector("nkes", nkeles);
  // The rest can be skipped if we do ROIs.
  if ( ! doResponseFit() )  return res;
  // Sort the reponse arrays and discard flagged points.
  map<float, int> iptOrderMap;
  for ( Index ipt=0; ipt<x.size(); ++ipt ) iptOrderMap[x[ipt]] = ipt;
  vector<Index> iptOrdered;
  for ( auto ent : iptOrderMap ) iptOrdered.push_back(ent.second);
  npt = x.size();
  vector<float> xf;
  vector<float> dxf;
  vector<float> yf;
  vector<float> dyf;
  vector<float> dyfFit;
  vector<float> fitPedf;
  for ( Index ipt : iptOrdered ) {
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
  if ( nptf < 5 ) {
    int wf = 12;
    cout << myname << "WARNING: Fitted point count is " << nptf << endl;
    cout << setw(4) << "i" << setw(wf) << "q [ke]" << setw(wf) << "signal" << setw(wf) << "keep" << endl;
    for ( Index ipt : iptOrdered ) {
      cout << setw(4) << ipt << setw(wf) << x[ipt] << setw(wf) << y[ipt] << setw(wf) << fitkeep[ipt] << endl;
    }
  }
  for ( bool usePos : usePosValues ) {
    string ssgn = usePos ? "Pos" : "Neg";
    res.setFloatVector("sticky1" + ssgn + "s", sticky1[usePos]);
    res.setFloatVector("sticky2" + ssgn + "s", sticky2[usePos]);
  }
  httl = "Response " + styp + usePosLab + "; Charge [ke]; Mean ADC " + styp;
  string gnam = "gchaResp" + styp + signLabel;
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
  float adcmin = -100000;
  float adcfloor = adcmin;   // Points with ADC below this are not used in refit.
  float keloff = 0.0;
  float slowoff = 10.0;
  float slowfac = 0.8;
  float adctanhMin = 0.1;
  float adctanh = adctanhMin;
  bool fitAdcMin = false;
  bool fitAdcMax = false;
  bool fitKelOff = false;
  bool fitSlow = false;
  bool fitTanh = 1;
  // Choose fit function.
  string sfit = responseFitFunctionName(isgn);
  TF1* pfit0 = nullptr;
  TF1* pfit = nullptr;
  TF1* prefit = nullptr;
  // Evaluate fit range.
  // For areas, we take this from the height fit.
  // For heights, we first fit with saturation to determine the range.
  double xfmin = xf[0] - 1.0;
  double xfmax = xf[nptf-1] + 1.0;
  if ( useArea ) {
    const DataMap& reshgt = getChannelResponse(icha, isgn, false);
    xfmin = reshgt.getFloat("refitXminHeight" + signLabel, xfmin);
    xfmax = reshgt.getFloat("refitXmaxHeight" + signLabel, xfmax);
  }
  if ( nptf > 0 ) {
    if ( sfit == "fgain" ) {
      pfit = new TF1("fgain", "[0]*x");
    } else if ( sfit == "fgainMin" ) {
      pfit = new TF1("fgainMin", "x>[1]/[0]?[0]*x:[1]");
      adcmin = yf[nptf-2];
      pfit->SetParameter(1, adcmin);
      pfit->SetParName(1, "adcmin");
      fitAdcMin = true;
    } else if ( sfit == "fgainMinSlow" ) {
      pfit = new TF1("fgainMinSlow", "x>([1]/[0]+[2])?[0]*x:(x<([1]/[0]-[2])?[1]:0.5*([0]*(x+[2])+[1]))");
      adcmin = yf[nptf-2];
      pfit->SetParameter(1, adcmin);
      pfit->SetParName(1, "adcmin");
      pfit->SetParameter(2, slowoff);
      pfit->SetParName(2, "slowoff");
      pfit->SetParLimits(2, 0, 50);
      fitAdcMin = true;
      fitSlow = true;
    } else if ( sfit == "fgainMinSlow2" ) {
      pfit = new TF1("fgainMinSlow2", "x>([1]/[0]-[3]*[2])/(1-[3])?[0]*x:(x<[2]?[1]:([3]*[0]*(x-[2])+[1]))");
      pfit = new TF1("fgainMinSlow2", "x>([1]/[0]+[2]/(1/[3]-1))?[0]*x:(x<([1]/[0]-[2])?[1]:([3]*[0]*(x-[1]/[0]+[2])+[1]))");
      adcmin = yf[nptf-2];
      pfit->SetParameter(1, adcmin);
      pfit->SetParName(1, "adcmin");
      pfit->SetParameter(2, slowoff);
      pfit->SetParName(2, "slowoff");
      pfit->SetParLimits(2, 0, 50);
      pfit->SetParameter(2, slowoff);
      pfit->SetParName(3, "slowfac");
      pfit->SetParameter(3, slowfac);
      pfit->SetParLimits(3, 0.01, 0.99);
      fitAdcMin = true;
      fitSlow = true;
    } else if ( sfit == "fgainMinTanh" ) {
      pfit = new TF1("fgainMinTanh", "x>([1]/[0])?[0]*tanh((x-[1]/[0])/[2])*x:[1]");
      adcmin = yf[nptf-2];
      pfit->SetParameter(1, adcmin);
      pfit->SetParName(1, "adcmin");
      pfit->SetParameter(2, adctanh);
      pfit->SetParName(2, "adctanh");
      pfit->SetParLimits(2, adctanhMin, 20);
      fitAdcMin = true;
      fitTanh = true;
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
      return res.setStatus(5);
    }
    // Fit.
    pfit->SetParName(0, "gain");
    pfit->SetParameter(0, gain);
    pfit->SetParLimits(0, 0.8*gain, 1.2*gain);
    pfit0 = pfit;
    Index npar = pfit->GetNpar();
    //pgf->Fit(pfit, "", "", xfmin, x[2]);
    pgfFit->Fit(pfit, "Q", "", xfmin, xfmax);
    pgf->GetListOfFunctions()->Add(pfit);
    gain = pfit->GetParameter(0);
    if ( fitAdcMin ) adcmin = pfit->GetParameter(pfit->GetParNumber("adcmin"));
    if ( fitAdcMax ) adcmax = pfit->GetParameter(pfit->GetParNumber("adcmax"));
    if ( fitKelOff ) keloff = pfit->GetParameter(pfit->GetParNumber("keloff"));
    float qmin = adcmin/gain - keloff;
    float qmax = adcmax/gain - keloff;
    // Refit without min saturation.
    // Exclude points close to saturation.
    // For area, we use the lower limit determined with heights.
    adcfloor = adcmin;
    if ( true ) {
      adcfloor = adcmin + 400;
      for ( Index ipt=0; ipt<nptf; ++ipt ) {
        if ( yf[ipt] < adcfloor ) {
          double xfminNew = xf[ipt] + 1.0;
          if ( xfminNew > xfmin ) xfmin = xfminNew;
        }
      }
      prefit = new TF1("fgain", "[0]*x");
      prefit->SetParName(0, "gain");
      prefit->SetParameter(0, gain);
      pgfFit->Fit(prefit, "Q", "", xfmin, xfmax);
      pgf->GetListOfFunctions()->Add(pfit);
      pfit = prefit;
      res.setFloat("refitXmin" + styp + signLabel, xfmin);
      res.setFloat("refitXmax" + styp + signLabel, xfmax);
    }
    // Record the gain.
    gain = pfit->GetParameter(0);
    double gainUnc = pfit->GetParError(0);
    string fpname = "fitGain" + styp + signLabel;
    string fename = fpname + "Unc";
    res.setFloat(fpname, gain);
    res.setFloat(fename, gainUnc);
    // Record the ADC saturation.
    if ( fitAdcMax ) {
      fpname = "fitSaturationMax" + styp + signLabel;
      res.setFloat(fpname, adcmax);
    }
    if ( fitAdcMin ) {
      fpname = "fitSaturationMin" + styp + signLabel;
      res.setFloat(fpname, adcmin);
    }
    if ( fitKelOff ) {
      fpname = "fitKelOff" + styp + signLabel;
      res.setFloat(fpname, keloff);
    }
    // If there is no calibration, record values that can be used to deduce the
    // the raw ADC level at which the signal saturates.
    //   adcminWithPed - fitted saturation level minus the pedestal for first point after
    //   lowSaturatedRawAdcs - raw ADC means for the saturated points
    //   lowSaturatedRawAdcMax - maximum raw ADC mean for the saturated points
    if ( isNoCalib() && fitAdcMin && !useArea ) {
      // Find the pedestal above and closest to adcmin.
      float dadcNearest = 1.e10;
      Index iptNearest = 0;
      vector<float> lowSaturatedRawAdcs;
      float lowSaturatedRawAdcMax = 0.0;
      for ( Index ipt=0; ipt<xf.size(); ++ipt ) {
        float q = xf[ipt];
        double a = pfit0->Eval(q);
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
  double difnSumSq = 0.0;
  for ( Index ipt=0; ipt<nptf; ++ipt ) {
    double xpt = xf[ipt];
    double ypt = yf[ipt];
    //double yunc = dyf[ipt];
    double yunc = dyFit[ipt];
    double yfit = pfit->Eval(xpt);
    double ydif = ypt - yfit;
    double difn = ydif/yunc;
    xres.push_back(xpt);
    yres.push_back(ydif/gain);
    dyres.push_back(yunc/gain);
    //if ( fitAdcMin && (yfit <= 0.9999*adcfloor) ) continue;
    //if ( fitAdcMax && (yfit >= 0.9999*adcmax) ) continue;
    if ( xpt < xfmin ) continue;
    if ( xpt > xfmax ) continue;
    difnSumSq += difn*difn;
    xrese.push_back(xpt);
    yrese.push_back(ydif/gain);
    dyrese.push_back(yunc/gain);
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
  string hnamBase = "hchaResp" + styp + signLabel;
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
  double linFitChiSquare = difnSumSq;
  int nlin = xrese.size() - 1;
  double linFitChiSquareDof = nlin > 0.0 ? difnSumSq/nlin : 0.0;
  res.setFloat("linFitChiSquare" + styp + signLabel, linFitChiSquare);
  res.setFloat("linFitChiSquareDof" + styp + signLabel, linFitChiSquareDof);
  return res;
}

//**********************************************************************

DataMap FembTestAnalyzer::getChannelDeviations(Index icha) {
  const string myname = "FembTestAnalyzer::getChannelDeviations: ";
  if ( ! doRoi() ) {
    cout << myname << "ROIs are required." << endl;
    return DataMap(1);
  }
  ostringstream sscha;
  sscha << icha;
  string scha = sscha.str();
  DataMap res;
  res.setInt("channel", icha);
  string hylab = "# entries";
  string hnamDev = "hdev";
  string httlDev = calibName(true) + " deviation; #DeltaQ [ke]; " + hylab;
  string hnamAdv = "hadv";
  string httlAdv = "Signal |deviation| channel " + scha + "; #DeltaQ [ke]; " + hylab;
  TH1* phDev = new TH1F(hnamDev.c_str(), httlDev.c_str(), 200, -20, 20);
  TH1* phAdv = new TH1F(hnamAdv.c_str(), httlAdv.c_str(), 50, 0, 5);
  vector<TH1*> hists = {phDev, phAdv};
  Index nevt = nEvent();
  Index devBulkCount = 0;
  Index devTailCount = 0;
  float devsum = 0.0;
  float dev2sum = 0.0;
  float devlim = 5.0;
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
        float adv = fabs(dev);
        if ( adv > devlim ) {
          ++devTailCount;
        } else {
          ++devBulkCount;
          devsum += dev;
          dev2sum += dev*dev;
        }
        phDev->Fill(dev);
        phAdv->Fill(adv);
      }
    }
  }
  for ( TH1* ph : hists ) {
    ph->SetLineWidth(2);
    ph->SetStats(0);
    res.setHist(ph, true);  
  }
  float devMean = devBulkCount > 0 ? devsum/devBulkCount : 0.0;
  float devRms = devBulkCount > 0 ? sqrt(dev2sum/devBulkCount) : 0.0;
  Index devCount = devBulkCount + devTailCount;
  float devTailFrac = float(devTailCount)/devCount;
  res.setInt("devCount", devCount);
  res.setFloat("devMean", devMean);
  res.setFloat("devRms", devRms);
  res.setFloat("devTailFrac", devTailFrac);
  return res;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::processChannel(Index icha) {
  const string myname = "FembTestAnalyzer::processChannel: ";
  DataMap& res = chanResults[icha];
  if ( res.haveInt("channel") ) return res;
  if ( ! haveTools() ) {
    cout << myname << "ADC processing tools are missing." << endl;
    return res.setStatus(3);
  }
  vector<SignOption> isgns;
  if ( true ) {
    isgns.push_back(OptBothSigns);
  } else {
    isgns.push_back(OptPositive);
    isgns.push_back(OptNegative);
  }
  vector<bool> useAreas;
  useAreas.push_back(false);
  if ( ! doTickModRoi() ) useAreas.push_back(true);
  if ( true ) {
    for ( SignOption isgn : isgns ) {
      for ( bool useArea : useAreas ) {
        res += getChannelResponse(icha, isgn, useArea);
      }
    }
    //if ( isCalib() && doPeakRoi() ) res += getChannelDeviations(icha);
    if ( isCalib() ) res += getChannelDeviations(icha);
  }
  if ( true ) {
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
  }
  return res;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::processAll(int a_tickPeriod) {
  const string myname = "FembTestAnalyzer::processAll: ";
  if ( allResult.haveInt("ncha") ) return allResult;
  if ( a_tickPeriod >= 0 ) setTickPeriod(a_tickPeriod);
  Index ncha = nChannel();
  allResult.setInt("ncha", ncha);
  // Process all channels and check for errors.
  for ( Index icha=0; icha<ncha; ++icha ) {
    cout << myname << "Channel " << icha << endl;
    const DataMap& resc = processChannel(icha);
    if ( resc ) {
      cout << myname << "Processing of channel " << icha << " returns error " << resc.status() << endl;
    }
  }
  map<string,TH1*> hists;
  // Pedestal histogram.
  string hnam = "hchaPed";
  ostringstream ssfemb;
  ssfemb << femb();
  string sfemb = ssfemb.str();
  string httl = "Pedestals; Channel; Pedestal [ADC]";
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
  if ( doRoi() ) {
    // Histograms for area/height and pos/neg combos.
    TH1* phsatMin = nullptr;
    TH1* phsatMax = nullptr;
    bool useBoth = true;
    vector<string> posValues;
    if ( useBoth ) {
      posValues.push_back("");
    } else {
      posValues.push_back("Neg");
      posValues.push_back("Pos");
    }
    // Loop over types (height and area)
    vector<bool> useAreas;
    useAreas.push_back(false);
    if ( doPeakRoi() ) useAreas.push_back(true);
    bool haveFitSatMin = false;
    bool haveFitSatMax = false;
    for ( bool useArea : useAreas ) {
      string sarea = useArea ? "Area" : "Height";
      string gunitLab = gainUnit().size() ? " [" + gainUnit() + "]" : "";
      // Loop over signs (pos, neg or both)
      for ( string spos : posValues ) {
        string hnam = "hgain" + sarea + spos;
        string httl = sarea + " " + spos + " Gain; Channel; Gain" + gunitLab;
        TH1* phg = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
        hists[hnam] = phg;
        string hnamd = "hgaindist" + sarea + spos;
        string httld = sarea + " " + spos + " Gain; Gain" + gunitLab + "; # channels";
        TH1* phgd = new TH1F(hnamd.c_str(), httld.c_str(), 100, 0, -1);
        hists[hnamd] = phgd;
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
          string fnam = "fitGain" + sarea + spos;
          string fenam = fnam + "Unc";
          const DataMap& resc = processChannel(icha);
          float gain = resc.getFloat(fnam);
          phg->SetBinContent(icha+1, gain);
          phg->SetBinError(icha+1, resc.getFloat(fenam));
          phgd->Fill(gain);
          if ( phsatMin != nullptr ) {
            fnam = "fitSaturationMinHeight" + spos;
            if ( resc.haveFloat(fnam) ) {
              phsatMin->SetBinContent(icha+1, resc.getFloat(fnam));
              haveFitSatMin = true;
            }
          }
          if ( phsatMax != nullptr ) {
            fnam = "fitSaturationMaxHeight" + spos;
            if ( resc.haveFloat(fnam) ) {
              phsatMax->SetBinContent(icha+1, resc.getFloat(fnam));
              haveFitSatMax = true;
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
    // Calibration underflow and overflow histograms.
    if ( isCalib() ) {
      hnam = "hchaCalibAdcMin";
      httl = "Calibration ADC min; Channel; ADC count";
      TH1* phmin = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
      hists[hnam] = phmin;
      hnam = "hchaCalibAdcMax";
      httl = "Calibration ADC max; Channel; ADC count";
      TH1* phmax = new TH1F(hnam.c_str(), httl.c_str(), ncha, 0, ncha);
      hists[hnam] = phmax;
      for ( Index icha=0; icha<ncha; ++icha ) {
        const DataMap& resc = processChannel(icha);
        phmin->SetBinContent(icha+1, resc.getInt("calibAdcMin", -999));
        phmax->SetBinContent(icha+1, resc.getInt("calibAdcMax", -999));
      }
    }
    // Fit underflow histogram.
    // This is the ADC value at which each channel saturates for negative signals.
    if ( haveFitSatMin ) {
      hnam = "hchaFitAdcMin";
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
      if ( phu != nullptr ) hists[hnam] = phu;
    }
    // Fit overflow histogram.
    // This is the ADC value at which each channel saturates for negative signals.
    if ( haveFitSatMax ) {
      hnam = "hchaFitAdcMax";
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
      if ( pho != nullptr ) hists[hnam] = pho;
    }
    // Fit chi-square distribution.
    if ( true ) {
      string sarea = "Height";
      string spos = "";
      string sposSpace = "";
      string hnamCsdd = "hcsdDist" + sarea + spos;;
      string httlCsdd = sarea + sposSpace + " linear fit quality; #chi^{2}/DOF; # channels";
      TH1* phcsdd= new TH1F(hnamCsdd.c_str(), httlCsdd.c_str(), 40, 0, 20);
      string hnamCsdc = "hcsdChan" + sarea + spos;
      string httlCsdc = sarea + sposSpace + " linear fit quality; Channel; #chi^{2}/DOF";
      TH1* phcsdc = new TH1F(hnamCsdc.c_str(), httlCsdc.c_str(), ncha, 0, ncha);
      for ( TH1* ph : {phcsdd, phcsdc} ) {
        ph->SetDirectory(0);
        ph->SetLineWidth(2);
        ph->SetStats(0);
      }
      for ( Index icha=0; icha<ncha; ++icha ) {
        const DataMap& resch = processChannel(icha);
        float csdh = resch.getFloat("linFitChiSquareDofHeight");
        phcsdd->Fill(csdh);
        phcsdc->SetBinContent(icha+1, csdh);
      }
      allResult.setHist(phcsdd);
      allResult.setHist(phcsdc);
    }
    // Signal deviation histograms.
    Index devChannelCount = 0;
    Index devBulkCount = 0;
    Index devTailCount = 0;
    float devsum = 0.0;
    float dev2sum = 0.0;
    if ( isCalib() ) {
      TH1* phdev = nullptr;
      TH1* phadv = nullptr;
      for ( Index icha=0; icha<ncha; ++icha ) {
        const DataMap& resevt = processChannel(icha);
        TH1* phdevCha = resevt.getHist("hdev");
        Index count = resevt.getInt("devCount");
        if ( count > 0 ) {
          ++devChannelCount;
          Index ntail = count*resevt.getFloat("devTailFrac") + 0.1;
          Index nbulk = count - ntail;
          devBulkCount += nbulk;
          devTailCount += ntail;
          devsum += nbulk*resevt.getFloat("devMean");
          float devrms = resevt.getFloat("devRms");
          dev2sum += nbulk*devrms*devrms;
        }
        if ( phdevCha == nullptr ) {
          cout << myname << "Result for channel " << icha << " does not have histogram hdev" << endl;
          continue;
        }
        TH1* phadvCha = resevt.getHist("hadv");
        bool doAbs = phadvCha != nullptr;
        if ( phdev == nullptr ) {
          phdev = dynamic_cast<TH1*>(phdevCha->Clone("hdev"));
          vector<TH1*> devhists;
          devhists.push_back(phdev);
          if ( doAbs ) {
            phadv = dynamic_cast<TH1*>(phadvCha->Clone("hadv"));
            devhists.push_back(phadv);
          }
          for ( TH1* ph : devhists ) {
            ph->SetDirectory(nullptr);
            ph->SetLineWidth(2);
            ph->Reset();
          }
        }
        string sttlDev = calibName(true) + " deviation";
        phdev->SetTitle(sttlDev.c_str());
        phdev->Add(phdevCha);
        if ( doAbs ) {
          string sttlAdv = calibName(true) + " deviation magnitude";
          phadv->SetTitle(sttlAdv.c_str());
          phadv->Add(phadvCha);
        }
      }
      allResult.setHist(phdev, true);
      if ( phadv != nullptr ) allResult.setHist(phadv, true);
      float devMean = devBulkCount > 0 ? devsum/devBulkCount : 0.0;
      float devRms = devBulkCount > 0 ? sqrt(dev2sum/devBulkCount) : 0.0;
      Index devCount = devBulkCount + devTailCount;
      float devTailFrac = float(devTailCount)/devCount;
      allResult.setInt("devChannelCount", devChannelCount);
      allResult.setInt("devCount", devCount);
      allResult.setFloat("devMean", devMean);
      allResult.setFloat("devRms", devRms);
      allResult.setFloat("devTailFrac", devTailFrac);
    }
  }  // end if doRoi()
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

bool FembTestAnalyzer::haveTools() const {
  return adcModifiers.size() && adcViewers.size();
}

//**********************************************************************

string FembTestAnalyzer::gainUnit() const {
  if ( signalUnit().substr(0,3) == "ADC" ) return "ADC/ke";
  if ( signalUnit() == "ke" ) return "";
  return signalUnit() + "/ke";
}

//**********************************************************************

FembTestPulseTree* FembTestAnalyzer::pulseTree(bool useAll) {
  const string myname = "FembTestAnalyzer::pulseTree: ";
  if ( m_ptreePulse != nullptr ) return m_ptreePulse.get();
  if ( ! isCalib() ) {
    cout << myname << "Must have calibration for pulse tree." << endl;
    return nullptr;
  }
  if ( ! haveTools() ) {
    cout << myname << "ADC processing tools are missing." << endl;
    return nullptr;
  }
  if ( useAll ) processAll();
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
    DataMap res0 = processChannelEvent(icha, 0);
    data.ped0 = res0.getFloat("pedestal");
    for ( Index ievt=0; ievt<nevt; ++ievt ) {
      DataMap res = processChannelEvent(icha, ievt);
      data.pede = res.getFloat("pedestal");
      float nele = res.getFloat("nElectron");
      if ( nele == 0.0 ) continue;
      data.qexp = -0.001*nele;
      data.sevt = -ievt;
      data.nsat = res.getInt("sigNUnderflowNeg");
      data.stk1 = res.getFloat("stickyFraction1Neg");
      data.stk2 = res.getFloat("stickyFraction2Neg");
      data.qcal = res.getFloatVector("roiSigCalNeg");
      data.cmea = res.getFloat("roiSigCalMeanNeg");
      data.crms = res.getFloat("roiSigCalRmsNeg");
      data.cdev = res.getFloatVector("roiSigDevPos");
      m_ptreePulse->fill(data);
      data.qexp = 0.001*nele;
      data.sevt = ievt;
      data.nsat = res.getInt("sigNOverflowPos");
      data.stk1 = res.getFloat("stickyFraction1Pos");
      data.stk2 = res.getFloat("stickyFraction2Pos");
      data.qcal = res.getFloatVector("roiSigCalPos");
      data.cmea = res.getFloat("roiSigCalMeanPos");
      data.crms = res.getFloat("roiSigCalRmsPos");
      data.cdev = res.getFloatVector("roiSigDevPos");
      m_ptreePulse->fill(data);
    }
  }
  m_ptreePulse->write();
  return m_ptreePulse.get();
}

//**********************************************************************

FembTestTickModTree* FembTestAnalyzer::tickModTree(bool useAll) {
  const string myname = "FembTestAnalyzer::tickModTree: ";
  if ( m_ptreeTickMod != nullptr ) return m_ptreeTickMod.get();
  string caltyp = isCalib() ? "calib" : "raw";
  if ( ! haveTools() ) {
    cout << myname << "ADC processing tools are missing." << endl;
    return nullptr;
  }
  if ( useAll ) processAll();
  ostringstream ssnam;
  ssnam << "femb_" << caltyp << "_tickmod_femb" << femb() << "_g" << gainIndex()
          << "_s" << shapingIndex()
          << "_p" << (extPulse() ? "ext" : "int");
  if ( ! extClock() ) ssnam << "_cint";
  string snam = ssnam.str() + ".root";
  m_ptreeTickMod.reset(new FembTestTickModTree(snam));
  return m_ptreeTickMod.get();
}

//**********************************************************************

int FembTestAnalyzer::writeCalibFcl() {
  const string myname = "FembTestAnalyzer::writeCalibFcl: ";
  if ( ! isNoCalib() ) {
    cout << myname << "Invalid calibration option: " << calibOptionName() << endl;
    return 1;
  }
  string sclass = "FembLinearCalibration";
  string baseName = "calibFromFemb";
  ostringstream ssnam;
  ssnam << baseName << "_" << "g" << gainIndex() << "s" << shapingIndex();
  if ( ! isCold() ) ssnam << "_warm";
  if ( ! extPulse() ) ssnam << "_intPulse";
  if ( ! extClock() ) ssnam << "_intClock";
  string dirName = ssnam.str();
  bool dirMissing = gSystem->AccessPathName(dirName.c_str());
  if ( dirMissing ) {
    if ( gSystem->mkdir(dirName.c_str()) ) {
      cout << myname << "Unable to create calibration fcl directory " << dirName << endl;
      return 2;
    }
    cout << myname << "Created directory " << dirName << endl;
  }
  ssnam << "_femb" << femb();
  string toolName = ssnam.str();
  string fclName = dirName + "/" + toolName + ".fcl";
  ofstream fout(fclName.c_str());
  ostringstream ssgain, ssamin;
  string gainName = "fitGainHeight";
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
    cout << myname <<   "     draw(\"raw\", ich, iev) - Raw ADC data for channel ich event iev" << endl;
    cout << myname <<   "    draw(\"prep\", ich, iev) - Prepared ADC data for channel ich event iev" << endl;
    cout << myname <<   "     draw(\"ped\", ich, iev) - Pedestal for channel ich event iev" << endl;
    cout << myname <<   "          draw(\"ped\", ich) - Pedestal vs. Qin for channel ich" << endl;
    cout << myname <<   "               draw(\"ped\") - Pedestal vs. channel" << endl;
    if ( doRoi() ) {
      cout << myname << "            draw(\"pedlim\") - Pedestal vs. channel with ADC ranges" << endl;
      cout << myname << "             draw(\"csddh\") - Height chi-square/DOF distribution." << endl;
      cout << myname << "             draw(\"csdch\") - Height chi-square/DOF vs. channel." << endl;
      cout << myname << "              draw(\"rmsh\") - Height deviations." << endl;
      cout << myname << "              draw(\"rmsa\") - Area deviations." << endl;
      cout << myname << "             draw(\"gainh\") - Height gains vs. channel" << endl;
      cout << myname << "             draw(\"gaina\") - Area gains vs. channel" << endl;
      cout << myname << "               draw(\"dev\") - ADC calibration deviations for the FEMB" << endl;
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
    }
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
  string slab = reader()->label().c_str();
  if      ( isHeightCalib() ) slab += " HeightCal";
  else if ( isAreaCalib()   ) slab += " AreaCal";
  else if ( isNoCalib()     ) slab += " NoCal";
  else                        slab += " UnknownCal";
  if      ( doPeakRoi() )    slab += " PeakROI";
  else if ( doTickModRoi() ) slab += " TickmodROI";
  else                       slab += " UknownROI";
  TLatex* plab = new TLatex(0.01, 0.015, slab.c_str());
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
      string hnam1;
      string hnam2;
      string hnamp = "hchaPed";
      string sttl;
      if ( isCalib() ) {
        hnam1 = "hchaCalibAdcMin";
        hnam2 = "hchaCalibAdcMax";
        sttl = "ADC pedestals and calibration ranges";
      } else if ( doResponseFit() ) {
        hnam1 = "hchaFitAdcMin";
        hnam2 = "hchaFitAdcMax";
        sttl = "ADC pedestals and fitted ranges";
      } else {
        cout << myname << "Plot " << sopt << " requires calibration or reponse fitting." << endl;
        return nullptr;
      }
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
      TH1* ph1 = processAll().getHist(hnam1);
      TH1* ph2 = processAll().getHist(hnam2);
      TH1* php = processAll().getHist(hnamp);
      if ( php == nullptr ) {
        cout << myname << "Unable to find histogram " << hnamp << " in this result:" << endl;
        processAll().print();
        return nullptr;
      } else if ( ph1 == nullptr && ph2 == nullptr ) {
        cout << myname << "Unable to find " << hnam1 << " or " << hnam2 << " in this result:" << endl;
        processAll().print();
        return nullptr;
      } else {
        //ph1->SetFillStyle(3001);
        if ( ph1 == nullptr ) {
          ph1 = dynamic_cast<TH1*>(php->Clone(hnam1.c_str()));
          ph1->SetDirectory(0);
          for ( int ibin=0; ibin<=ph1->GetNbinsX(); ++ibin ) ph1->SetBinContent(ibin, 0);
        }
        if ( ph2 == nullptr ) {
          ph2 = dynamic_cast<TH1*>(php->Clone(hnam2.c_str()));
          ph2->SetDirectory(0);
          for ( int ibin=0; ibin<=ph2->GetNbinsX(); ++ibin ) ph2->SetBinContent(ibin, 4095);
        }
        man.add(ph2, "H");
        man.add(ph1, "H same");
        man.add(php, "P same");
        TH1* ph1n = man.getHist(hnam1);
        TH1* ph2n = man.getHist(hnam2);
        TH1* phpn = man.getHist(hnamp);
        man.setTitle(sttl.c_str());
        man.hist()->GetYaxis()->SetTitle("ADC count");
        int icol = 17;    // Color for excluded regions.
        man.setFrameFillColor(icol);
        ph1n->SetLineColor(icol);
        ph1n->SetMarkerColor(icol);
        ph1n->SetFillColor(icol);
        ph1n->SetLineWidth(0);
        icol = 10;        // Color for included region
        ph2n->SetFillColor(icol);
        ph2n->SetLineColor(icol);
        ph2n->SetLineWidth(0);
        man.setRangeY(0, 4200);
        man.addVerticalModLines(16);
        return draw(&man);
      }
    // Gain vs. channel.
    } else if ( sopt == "gainh" || sopt == "gaina" ) {
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
      bool useArea = sopt == "gaina";
      string sarea = useArea ? "area" : "height";
      string hname = useArea ? "hgainArea" : "hgainHeight";
      TH1* ph = processAll().getHist(hname);
      if ( ph == nullptr ) {
        cout << myname << "Unable to find histogram " << hname << " in result:" << endl;
        processAll().print();
      } else if ( int rstat = man.add(ph, "H") ) {
        cout << myname << "Manipulator returned error " << rstat << endl;
      } else {
        string sttl = "ADC " + sarea + " gains";
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
    // Chi-square/DOF
    } else if ( sopt == "csddh" || sopt == "csdch" ) {
      string hnam = "NoSuchCsdHist";
      if ( sopt == "csddh" ) hnam = "hcsdDistHeight";
      if ( sopt == "csdch" ) hnam = "hcsdChanHeight";
      if ( ! processAll().haveHist(hnam) ) {
        cout << myname << "Unable to find histogram " << hnam << endl;
        return nullptr;
      }
      TH1* ph = processAll().getHist(hnam);
      bool isDist = hnam.find("Dist") != string::npos;
      man.add(ph);
      if ( isDist ) {
        man.showOverflow();
      } else {
        man.setRangeY(0, 20);
        man.addVerticalModLines(16);
      }
      return draw(&man);
    } else if ( sopt == "rmsh" || sopt == "rmsa" ) {
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
      string hnamdev = sopt == "rmsh" ? "hallHeightGms" : "hallAreaGms";
      string hnamrms = sopt == "rmsh" ? "hallHeightRms" : "hallAreaRms";
      string hnamars = sopt == "rmsh" ? "hallHeightArs" : "hallAreaArs";
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
      ssttl << " deviations";
      man.hist()->SetTitle(ssttl.str().c_str());
      return draw(&man);
    } else if ( sopt == "dev" || sopt == "adv" ) {
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
      if ( ! isCalib() ) {
        cout << myname << "Drawing " << sopt << " is only available for calibrated samples." << endl;
        return nullptr;
      }
      const DataMap& res = processAll();
      string hnam = "h" + sopt;
      TH1* ph = res.getHist(hnam);
      if ( ph == nullptr ) {
        cout << myname << "Unable to find histogram " << hnam << ". " << endl;
        return nullptr;
      }
      man.add(ph);
      if ( sopt == "dev" ) {
        man.setRangeX(-5.0, 5.0);
        vector<string> slabs;
        ostringstream ssline;
        ssline.str("");
        ssline << "   # chan: " << res.getInt("devChannelCount");
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "    Count: " << res.getInt("devCount");
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "     Mean: " << setprecision(0) << fixed << 1000.0*res.getFloat("devMean") << " e";
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "      RMS: " << setprecision(0) << fixed << 1000.0*res.getFloat("devRms") << " e";
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "Tail frac.: " << setprecision(3) << fixed << res.getFloat("devTailFrac");
        slabs.push_back(ssline.str());
        double xlab = 0.77;
        double ylab = 0.85;
        double dylab = 0.045;
        for ( string slab : slabs ) {
          TLatex* plab = new TLatex(xlab, ylab, slab.c_str());
          plab->SetNDC();
          plab->SetTextSize(0.035);
          plab->SetTextFont(42);
  	  man.add(plab, "");
          ylab -= dylab;
        }
      }
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
      if ( ! doRoi() ) {
        cout << myname << "Response plots require ROIs" << endl;
        return nullptr;
      }
      vector<string> gnams;
      double ymin = -1500;
      double ymax = 3000;
      if ( isHeightCalib() ) {
        ymin = -200;
        ymax = 400;
      }
      if ( sopt == "resph" ) gnams = {"gchaRespHeightFitPointsUnc", "gchaRespHeightFitPointsUnc"};
      if ( sopt == "respa" ) {
        gnams = {"gchaRespAreaFitPointsUnc", "gchaRespArea"};
        if ( isNoCalib() ) {
          ymin = -15000;
          ymax = 25000;
        } else {
          ymin = -1000;
          ymax = 2000;
        }
      }
      if ( sopt == "respresh" ) {
        gnams = {"gchaRespHeightRes"};
        ymin = -5;
        ymax = 5;
      }
      if ( sopt == "respresa" ) {
        gnams = {"gchaRespAreaRes"};
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
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
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
        vector<string> slabs;
        ostringstream ssline;
        ssline.str("");
        ssline << "    Count: " << res.getInt("devCount");
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "     Mean: " << setprecision(0) << fixed << 1000.0*res.getFloat("devMean") << " e";
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "      RMS: " << setprecision(0) << fixed << 1000.0*res.getFloat("devRms") << " e";
        slabs.push_back(ssline.str());
        ssline.str("");
        ssline << "Tail frac.: " << setprecision(3) << fixed << res.getFloat("devTailFrac");
        slabs.push_back(ssline.str());
        double xlab = 0.77;
        double ylab = 0.85;
        double dylab = 0.045;
        for ( string slab : slabs ) {
          TLatex* plab = new TLatex(xlab, ylab, slab.c_str());
          plab->SetNDC();
          plab->SetTextSize(0.035);
          plab->SetTextFont(42);
  	  man.add(plab, "");
          ylab -= dylab;
        }
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
        man.showUnderflow();
        man.showOverflow();
        return draw(&man);
      }
    } else if ( sopt == "resphp" || sopt == "resphn" ||
                sopt == "respap" || sopt == "respan" ) {
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
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
        man.showUnderflow();
        man.showOverflow();
        return draw(&man);
      }
    } else if ( sopt == "devp" || sopt == "devn" ) {
      if ( ! doRoi() ) {
        cout << myname << "Plot " << sopt << " requires ROIs." << endl;
        return nullptr;
      }
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
  if ( ievt >= 0 ) ssnam << "_" << ievt;
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
