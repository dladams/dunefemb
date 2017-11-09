// FembTestAnalyzer.cxx

#include "FembTestAnalyzer.h"
#include <iostream>
#include <map>
#include "dune/DuneInterface/AdcChannelData.h"
#include "dune/ArtSupport/DuneToolManager.h"
#include "DuneFembFinder.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TPad.h"
#include "TLatex.h"

using std::string;
using std::cout;
using std::endl;
using std::map;

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int a_femb, int a_gain, int a_shap, 
                 std::string a_tspat, bool a_isCold,
                 bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(a_femb, a_tspat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock);
}

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(string dir, string fpat,
                 int a_femb, int a_gain, int a_shap, 
                 bool a_isCold, bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(a_femb, fpat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock, dir);
}

//**********************************************************************

FembTestAnalyzer::FembTestAnalyzer(int a_femb, string a_tspat, bool a_isCold )
: m_femb(a_femb), m_tspat(a_tspat), m_isCold(a_isCold) {
  const string myname = "FembTestAnalyzer::ctor: ";
  DuneToolManager* ptm = DuneToolManager::instance("dunefemb.fcl");
  if ( ptm == nullptr ) {
    cout << myname << "Unable to retrieve tool manager." << endl;
    return;
  }
  vector<string> acdModifierNames = {"adcPedestalFit", "adcSampleFiller", "adcThresholdSignalFinder"};
  for ( string modname : acdModifierNames ) {
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
  // Read raw data.
  reader()->read(ievt, icha, &acd);
  // Find pedestal.
  DataMap resmod;
  if ( dbg > 2 ) cout << myname << "Applying modifiers." << endl;
  for ( const std::unique_ptr<AdcChannelDataModifier>& pmod : adcModifiers ) {
    resmod += pmod->update(acd);
  }
  if ( dbg > 3 ) resmod.print();
  if ( dbg > 2 ) cout << myname << "Applying viewers." << endl;
  for ( const std::unique_ptr<AdcChannelViewer>& pvwr : adcViewers ) {
    resmod += pvwr->view(acd);
  }
  if ( dbg >= 2 ) {
    cout << myname << "Begin display of result. ---------------------------" << endl;
    resmod.print();
    cout << myname << "End display of result. -----------------------------" << endl;
  }
  res += resmod;
  res.setFloat("pedestal", acd.pedestal);
  bool haverois = resmod.haveInt("roiCount");
  Index nroi = haverois ? resmod.getInt("roiCount") : 0;
  if ( dbg > 2 ) cout << myname << "ROI count is " << nroi << endl;
  if ( nroi ) {
    vector<float> sigAreas[2];
    vector<float> sigHeights[2];
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
    for ( Index iroi=iroi1; iroi<iroi2; ++iroi ) {
      float area = resmod.getFloatVector("roiSigAreas")[iroi];
      float sigmin = resmod.getFloatVector("roiSigMins")[iroi];
      float sigmax = resmod.getFloatVector("roiSigMaxs")[iroi];
      int nundr = resmod.getIntVector("roiNUnderflows")[iroi];
      int nover = resmod.getIntVector("roiNOverflows")[iroi];
      Index isPos = area >= 0.0;
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
    }
    // For each sign...
    for ( Index isgn=0; isgn<2; ++isgn ) {
      string namsuf = isgn ? "Pos" : "Neg";
      string cntname = "sigCount" + namsuf;
      res.setInt(cntname, sigAreas[isgn].size());
      // Build area histogram. Record it and its mean and RMS.
      if ( sigAreas[isgn].size() ) {
        string hnam = "hsigArea" + namsuf;
        string httl = "FEMB test signal area; ADC counts; # signals";
        int binfac = 10;
        float xmin = binfac*int(areaMin[isgn]/binfac - 2);
        float xmax = binfac*int(areaMax[isgn]/binfac + 3);
        TH1* ph = new TH1F(hnam.c_str(), httl.c_str(), (xmax-xmin)/binfac, xmin, xmax);
        ph->SetStats(0);
        ph->SetLineWidth(2);
        for ( float val : sigAreas[isgn] ) ph->Fill(val);
        res.setHist(hnam, ph, true);
        string meanName = "sigAreaMean" + namsuf;
        string rmsName = "sigAreaRms" + namsuf;
        res.setFloat(meanName, ph->GetMean());
        res.setFloat(rmsName, ph->GetRMS());
      } else {
        cout << myname << "No " + namsuf + " Area ROIS " + " for channel " << icha
             << " event " << ievt << endl;
      }
      // Build height histogram. Record it and its mean and RMS.
      if ( sigHeights[isgn].size() ) {
        string hnam = "hsigHeight" + namsuf;
        string httl = "FEMB test signal height; ADC counts; # signals";
        int binfac = 10;
        float xmin = binfac*int(heightMin[isgn]/binfac - 2);
        float xmax = binfac*int(heightMax[isgn]/binfac + 3);
        TH1* ph = new TH1F(hnam.c_str(), httl.c_str(), (xmax-xmin)/binfac, xmin, xmax);
        ph->SetStats(0);
        ph->SetLineWidth(2);
        for ( float val : sigHeights[isgn] ) ph->Fill(val);
        res.setHist(hnam, ph, true);
        string meanName = "sigHeightMean" + namsuf;
        string rmsName = "sigHeightRms" + namsuf;
        res.setFloat(meanName, ph->GetMean());
        res.setFloat(rmsName, ph->GetRMS());
      } else {
        cout << myname << "No " + namsuf + " Area ROIS " + " for channel " << icha
             << " event " << ievt << endl;
      }
      // Record the # of ROIS with underflow and overflow bins.
      string unam = "sigNUnderflow" + namsuf;
      string onam = "sigNOverflow" + namsuf;
      res.setInt(unam, sigNundr[isgn]);
      res.setInt(onam, sigNover[isgn]);
    }
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
  vector<bool> fitkeep(npt, true);
  double ymax = 1000.0;
  vector<float> peds(nevt, 0.0);
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
    if ( ievt > ievt0 ) {
      for ( bool usePos : usePosValues ) {
        string ssgn = usePos ? "Pos" : "Neg";
        string meaName = "sig" + styp + "Mean" + ssgn;
        string rmsName = "sig" + styp + "Rms" + ssgn;
        string ovrName = "sigNOverflow" + ssgn;
        string udrName = "sigNUnderflow" + ssgn;
        Index ipt = x.size();
        int nudr = resevt.getInt(udrName);
        int novr = resevt.getInt(ovrName);   // # ROI with ADC overflows
        if ( resevt.haveFloat(meaName) ) {
          float sigmean = resevt.getFloat(meaName);
          float sigrms  = resevt.getFloat(rmsName);
          float nele = resevt.getFloat("nElectron");
          bool isUnderOver = nudr>0 || novr>0;
          if ( isUnderOver ) {
            if ( usePos ) ++nptPosBad;
            else          ++nptNegBad;
          } else {
            if ( usePos ) iptPosGood = ipt;
            else          iptNegGood = ipt;
          }
          float dsigmin = 0.5;  // Intrinsic ADC uncertainty.
          if ( isUnderOver ) dsigmin = 100.0;  // Big uncertainty for uder/overflows.
          float sigsign = 1.0;
          if ( useBoth && !usePos ) sigsign = -1.0;
          float dsig = sigrms;
          if ( dsig < dsigmin ) dsig = dsigmin;
          x.push_back(sigsign*0.001*nele);
          dx.push_back(0.0);
          y.push_back(sigsign*sigmean);
          dy.push_back(dsig);
          bool fitpt = true;
          if ( isUnderOver && !useBoth ) fitpt = false;
          if ( !pulseIsExternal && ievt < 2 ) fitpt = false;
          fitkeep.push_back(fitpt);
          double ylim = y[ipt] + dy[ipt];
          if ( ylim > ymax ) ymax = ylim;
        } else {
          cout << myname << styp << " " << ssgn << " mean not found for channel "
               << icha << " event " << ievt << ". ROI count is " << roiCount
               << ". Point skipped." << endl;
        }
      }
    }
  }
  npt = x.size();
  vector<float> xf;
  vector<float> dxf;
  vector<float> yf;
  vector<float> dyf;
  for ( Index ipt=0; ipt<npt; ++ipt ) {
    if ( fitkeep[ipt] ) {
      xf.push_back(x[ipt]);
      dxf.push_back(dx[ipt]);
      yf.push_back(y[ipt]);
      dyf.push_back(dy[ipt]);
    }
  }
  Index nptf = xf.size();
  res.setInt("channel", icha);
  res.setFloatVector("peds", peds);
  res.setFloatVector("nkes", x);
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
  pg->GetYaxis()->SetTitle("Output signal [ADC counts]");
  // Create graph with fitted points.
  string gnamf = gnam + "Fit";
  TGraph* pgf = new TGraphErrors(nptf, &xf[0], &yf[0], &dxf[0], &dyf[0]);
  pgf->SetName(gnamf.c_str());
  pgf->SetTitle(gttl.c_str());
  pgf->SetLineWidth(2);
  pgf->SetMarkerStyle(4);
  pgf->SetMarkerColor(2);
  pgf->GetXaxis()->SetTitle("Input charge [ke]");
  pgf->GetXaxis()->SetLimits(-xmax, xmax);
  pgf->GetYaxis()->SetTitle("Output signal [ADC counts]");
  // Set starting values for fit.
  Index iptGain = nptf - 1;
  if ( iptPosGood ) iptGain = iptPosGood;
  else if ( iptNegGood ) iptGain = iptNegGood;
  float gain = y[iptGain]/x[iptGain];
  float ped = 0.0;
  float adcmax = 0.0;
  float adcmin = 0.0;
  float keloff = 10;
  bool fitAdcMin = false;
  bool fitAdcMax = false;
  bool fitKelOff = false;
  // Choose fit function.
  string sfit;
  if      ( usePosOpt == "Pos" )  sfit = "fgain";
  else if ( usePosOpt == "Neg" )  sfit = "fgainMax";
  else if ( useBoth ) {
    if ( prdr->extPulse() ) {
      if ( nptPosBad ) sfit = "fgainMinMax";
      else sfit = "fgainMin";
    } else if ( prdr->intPulse() ) {
      if ( nptPosBad ) sfit = "fgainOffMinMax";
      else sfit = "fgainOffMin";
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
      pfit->SetParameter(2, keloff);
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
      pfit->SetParameter(3, keloff);
      pfit->SetParLimits(3, 1, 50);
      fitAdcMin = true;
      fitAdcMax = true;
      fitKelOff = true;
    } else {
      cout << myname << "Invalid value for sfit: " << sfit << endl;
      return DataMap(4);
    }
    pfit->SetParName(0, "gain");
    pfit->SetParameter(0, gain);
    pfit->SetParLimits(0, 0.8*gain, 1.2*gain);
    Index npar = pfit->GetNpar();
    //pgf->Fit(pfit, "", "", xfmin, x[2]);
    pgf->Fit(pfit, "Q");
    gain = pfit->GetParameter(0);
    ped = npar > 1 ? pfit->GetParameter(1) : 0.0;
    // Record the gain.
    string fpname = "fitGain" + styp + usePosOpt;
    res.setFloat(fpname, gain);
    // Record the ADC saturation.
    if ( fitAdcMax ) {
      adcmax = pfit->GetParameter("adcmax");
      fpname = "fitSaturationMax" + styp + usePosOpt;
      res.setFloat(fpname, adcmax);
    }
    if ( fitAdcMin ) {
      adcmin = pfit->GetParameter("adcmin");
      fpname = "fitSaturationMin" + styp + usePosOpt;
      res.setFloat(fpname, adcmin);
    }
    if ( fitKelOff ) {
      keloff = pfit->GetParameter("keloff");
      fpname = "fitKelOff" + styp + usePosOpt;
      res.setFloat(fpname, keloff);
    }
  }
  // Copy function to original graph.
  //TF1* pfitCopy = (TF1*) pfit->Clone();
  TF1* pfitCopy = new TF1;
  pfit->Copy(*pfitCopy);
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
    dyres.push_back(dy[ipt]/gain);
    if ( fitAdcMin && (yfit <= 0.9999*adcmin) ) continue;
    if ( fitAdcMax && (yfit >= 0.9999*adcmax) ) continue;
    xrese.push_back(xpt);
    yrese.push_back(ydif/gain);
    dyrese.push_back(dy[ipt]/gain);
  }
  string gnamr = gnam + "Res";
  TGraph* pgr = new TGraphErrors(xrese.size(), &xrese[0], &yrese[0], &dx[0], &dyrese[0]);
  pgr->SetName(gnamr.c_str());
  pgr->SetTitle(gttlr.c_str());
  pgr->SetLineWidth(2);
  pgr->GetXaxis()->SetTitle("Input charge [ke]");
  pgr->GetXaxis()->SetLimits(-xmax, xmax);
  pgr->GetYaxis()->SetTitle("Output signal residual [ke]");
  res.setGraph(gnam, pg);
  res.setGraph(gnamf, pgf);
  res.setGraph(gnamr, pgr);
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
  // Create pedestal graph.
  const DataMap::FloatVector& nkes = res.getFloatVector("nkes");
  const DataMap::FloatVector& peds = res.getFloatVector("peds");
  Index npt = nkes.size();
  string gnamp = "gchaPeds";
  ostringstream ssttl;
  ssttl << "Pedestal for channel " << icha;
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
  Index nch = nChannel();
  allResult.setInt("ncha", nch);
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
  for ( bool useArea : {false, true} ) {
    string sarea = useArea ? "Area" : "Height";
    for ( string spos : posValues ) {
      string hnam = "hgain" + sarea + spos;
      string httl = sarea + " " + spos + " Gain; Channel; Gain [ADC/ke]";
      TH1* phg = new TH1F(hnam.c_str(), httl.c_str(), nch, 0, nch);
      hists[hnam] = phg;
      // Histogram saturation for negative pulse height.
      if ( !useArea ) {
        hnam = "hsatMin" + sarea + spos;
        httl = sarea + " " + spos + " Lower saturation level; Channel; Saturation level [ADC]";
        phsatMin = new TH1F(hnam.c_str(), httl.c_str(), nch, 0, nch);
        hists[hnam] = phsatMin;
        hnam = "hsatMax" + sarea + spos;
        httl = sarea + " " + spos + " Upper saturation level; Channel; Saturation level [ADC]";
        phsatMax = new TH1F(hnam.c_str(), httl.c_str(), nch, 0, nch);
        hists[hnam] = phsatMax;
      }
      for ( Index ich=0; ich<nch; ++ich ) {
cout << myname << "Channel " << ich << endl;
        string fnam = "fitGain" + sarea + spos;
        const DataMap& resc = processChannel(ich);
        phg->SetBinContent(ich+1, resc.getFloat(fnam));
        if ( phsatMin != nullptr ) {
          fnam = "fitSaturationMinHeight" + spos;
          if ( resc.haveFloat(fnam) ) {
            phsatMin->SetBinContent(ich+1, resc.getFloat(fnam));
          }
        }
        if ( phsatMax != nullptr ) {
          fnam = "fitSaturationMaxHeight" + spos;
          if ( resc.haveFloat(fnam) ) {
            phsatMax->SetBinContent(ich+1, resc.getFloat(fnam));
          }
        }
      }
    }
  }
  // Pedestal histogram.
  string hnam = "hchaPed";
  ostringstream ssfemb;
  ssfemb << femb();
  string sfemb = ssfemb.str();
  string httl = "Pedestals for FEMB " + sfemb + "; Channel; Pedestal [ADC]";
  TH1* php = new TH1F(hnam.c_str(), httl.c_str(), nch, 0, nch);
  for ( Index ich=0; ich<nch; ++ich ) {
    const DataMap& resc = processChannel(ich);
    double ymin = resc.getFloat("pedMin");
    double ymax = resc.getFloat("pedMax");
    double y = 0.5*(ymin + ymax);
    double dy = 0.5*(ymax - ymin);
    php->SetBinContent(ich+1, y);
    php->SetBinError(ich+1, dy);
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
    phu = new TH1F(hnam.c_str(), httl.c_str(), nch, 0, nch);
    double addfac = useBoth ? 1.0 : -1.0;
    for ( Index ich=0; ich<nch; ++ich ) {
      double val = 0.0;
      double dval = 0.0;
      double satmin = phsatMin->GetBinContent(ich+1);
      if ( satmin ) {
        double ped = php->GetBinContent(ich+1);
        double dped = php->GetBinError(ich+1);
        double satval = ped + addfac*satmin;
        if ( satval > val ) {
          val = satval;
          dval = dped;
        }
      }
      phu->SetBinContent(ich+1, val);
      phu->SetBinError(ich+1, dval);
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
    pho = new TH1F(hnam.c_str(), httl.c_str(), nch, 0, nch);
    for ( Index ich=0; ich<nch; ++ich ) {
      double val = 4096;
      double dval = 0.0;
      double satmax = phsatMax->GetBinContent(ich+1);
      if ( satmax ) {
        double ped = php->GetBinContent(ich+1);
        double dped = php->GetBinError(ich+1);
        double satval = ped + satmax;
        if ( satval < val ) {
          val = satval;
          dval = dped;
        }
      }
      pho->SetBinContent(ich+1, val);
      pho->SetBinError(ich+1, dval);
    }
  }
  if ( pho != nullptr ) {
    int icol = 17;
    pho->SetLineColor(icol);
    pho->SetMarkerColor(icol);
    pho->SetFillColor(icol);
    hists[hnam] = pho;
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

TPadManipulator* FembTestAnalyzer::draw(string sopt, int icha) {
  const string myname = "FembTestAnalyzer::draw: ";
  if ( sopt == "help" ) {
    cout << myname << "           pedch - ADC pedestal for each channel" << endl;
    cout << myname << "        pedlimch - ADC pedestal and limits for each channel" << endl;
    cout << myname << "         gainsch - Area and height gains for each channel" << endl;
    cout << myname << "     resph, icha - Height response for channel icha" << endl;
    cout << myname << "     respa, icha - Area response for channel in icha" << endl;
    cout << myname << "  respresh, icha - Height response residual for channel icha" << endl;
    cout << myname << "  respresa, icha - Area response residual for channel icha" << endl;
    return nullptr;
  }
  const int wx = 700;
  const int wy = 500;
  string mnam = sopt;
  if ( icha >= 0 ) {
    ostringstream ssopt;
    ssopt << sopt << "-" << icha;
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
  // Pedestal vs. channel
  if ( sopt == "pedch" ) {
    TPadManipulator& man = m_mans[mnam];
    TH1* ph = processAll().getHist("hchaPed");
    if ( ph == nullptr ) {
      cout << myname << "Unable to find pedestal histogram hchaPed in this result:" << endl;
      processAll().print();
      return nullptr;
    }
    man.add(ph, "P");
    man.setRangeY(0, 4100);
    man.addVerticalModLines(16);
    return &man;
  // Pedestal vs. channel with ADC ranges
  } else if ( sopt == "pedlimch" ) {
    TPadManipulator& man = m_mans[mnam];
    TH1* ph1 = processAll().getHist("hchaAdcMin");
    TH1* ph2 = processAll().getHist("hchaAdcMax");
    TH1* php = processAll().getHist("hchaPed");
    if ( php == nullptr || ph1 == nullptr || ph2 == nullptr ) {
      cout << myname << "Unable to find hchaAdcMin, hchaAdcMax or hchaPed in this result:" << endl;
      processAll().print();
      return nullptr;
    }
    if ( int rstat = man.add(ph2, "H") ) {
      cout << myname << "Manipulator returned error " << rstat << endl;
      return nullptr;
    }
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
    man.addAxis();
    man.add(plab, "");
    return &man;
  // Gain vs. channel.
  } else if ( sopt == "gainsch" ) {
    TPadManipulator& man = m_mans[mnam];
    TH1* ph1 = processAll().getHist("hgainHeightBoth");
    TH1* ph2 = processAll().getHist("hgainAreaBoth");
    if ( ph1 == nullptr || ph2 == nullptr ) {
      cout << myname << "Unable to find hgainHeightBoth or hgainAreaBoth in process result:" << endl;
      processAll().print();
      return nullptr;
    }
    if ( int rstat = man.add(ph2, "H") ) {
      cout << myname << "Manipulator returned error " << rstat << endl;
      return nullptr;
    }
    //ph1->SetFillStyle(3001);
    man.add(ph1, "same");
    string sttl = "ADC gains for FEMB " + sfemb;
    man.hist()->SetTitle(sttl.c_str());
    //man.hist()->GetYaxis()->SetTitle("ADC count");
    man.addVerticalModLines(16);
    man.addAxis();
    man.add(plab);
    return &man;
  } else if ( sopt == "ped" ) {
    Index ievt = 0;
    const DataMap& res = processChannelEvent(icha, ievt);
    TH1* ph = res.getHist("pedestal");
    if ( ph == nullptr ) return nullptr;
    TPadManipulator& man = m_mans[mnam];
    man.add(ph, "");
    man.addHistFun(1);
    man.addHistFun(0);
    man.addVerticalModLines(64);
    man.addAxis();
    man.add(plab);
    man.update();
    return &man;
  } else if ( sopt == "resph" || 
              sopt == "respa" ||
              sopt == "respresh" ||
              sopt == "respresa" ) {
    vector<string> gnams;
    double ymin = -1500;
    double ymax = 3000;
    if ( sopt == "resph" ) gnams = {"gchaRespHeightBothFit", "gchaRespHeightBoth"};
    if ( sopt == "respa" ) {
      gnams = {"gchaRespAreaBothFit", "gchaRespAreaBoth"};
      ymin = -15000;
      ymax = 25000;
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
/*
    Index iadc = iopt;
    if ( iadc > 8 ) return nullptr;
    TPadManipulator& topman = m_mans[mnam];
    topman.split(4);
    Index ich0 = 16*iadc;
    Index ievt = 0;
    for ( Index ich=0; ich<16; ++ich ) {
      const DataMap& res = processChannel(ich0+ich);
    }
    topman.update();
    return &topman;
  }
*/
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
      return nullptr;
    }
    TPadManipulator& man = m_mans[mnam];
    man.add(pgs[0], "AP");
    for ( Index igrf=1; igrf<ngrf; ++igrf ) man.add(pgs[igrf], "P");
    man.setRangeY(ymin, ymax);
    man.setGrid();
    //man.addHistFun(1);
    //man.addHistFun(0);
    //man.addVerticalModLines(64);
    man.addHorizontalLine();
    man.addVerticalLine();
    man.add(plab);
    man.addAxis();
    man.update();
    return &man;
  }
  cout << myname << "Unknown plot name: " << sopt << endl;
  return nullptr;
}

//**********************************************************************

TPadManipulator* FembTestAnalyzer::drawAdc(string sopt, int iadc) {
  const string myname = "FembTestAnalyzer::drawAdc: ";
  ostringstream ssnam;
  if ( iadc < 0 || iadc > 7 ) return nullptr;
  ssnam << "adc" << iadc << "_" << sopt;
  string mnam = ssnam.str();
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  TPadManipulator& man = m_mans[mnam];
  int wy = gROOT->IsBatch() ? 2000 : 1000;
  man.setCanvasSize(wx, 1.4*wx);
  man.split(4);
  for ( Index kcha=0; kcha<16; ++kcha ) {
    Index icha = 16*iadc + kcha;
    TPadManipulator* pman = draw(sopt, icha);
    if ( pman == nullptr ) {
      cout << myname << "Unable to find plot " << sopt << " for channel " << icha << endl;
      m_mans.erase(mnam);
      return nullptr;
    }
    *man.man(kcha) = *pman;
  }
  return &man;
}

//**********************************************************************
