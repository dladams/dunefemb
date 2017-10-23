// FembTestAnalyzer.cxx

#include "FembTestAnalyzer.h"
#include <iostream>
#include "dune/DuneInterface/AdcChannelData.h"
#include "dune/ArtSupport/DuneToolManager.h"
#include "DuneFembFinder.h"
#include "TGraphErrors.h"

using std::string;
using std::cout;
using std::endl;

//**********************************************************************

FembTestAnalyzer::
FembTestAnalyzer(int a_femb, int a_gain, int a_shap, 
                 std::string a_tspat, bool a_isCold,
                 bool a_extPulse, bool a_extClock) :
FembTestAnalyzer(a_femb, a_tspat, a_isCold) {
  find(a_gain, a_shap, a_extPulse, a_extClock);
}

//**********************************************************************

FembTestAnalyzer::FembTestAnalyzer(int a_femb, string a_tspat, bool a_isCold )
: m_femb(a_femb), m_tspat(a_tspat), m_isCold(a_isCold),
  m_gain(99), m_shap(99), m_extPulse(false), m_extClock(false) {
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
find(int a_gain, int a_shap, bool a_extPulse, bool a_extClock) {
  const string myname = "FembTestAnalyzer::find: ";
  DuneFembFinder fdr;
  results.clear();
  cout << myname << "Fetching reader." << endl;
  m_reader = std::move(fdr.find(femb(), isCold(), tspat(), a_gain, a_shap, a_extPulse, a_extClock));
  cout << myname << "Done fetching reader." << endl;
  if ( m_reader == nullptr ) {
    m_gain = -1;
    m_shap = -1;
    m_extPulse = false;
    m_extClock = false;
    return 1;
  }
  m_gain = a_gain;
  m_shap = a_shap;
  m_extPulse = a_extPulse;
  m_extPulse = a_extClock;
  results.resize(nChannel(), vector<DataMap>(nEvent()));
  return 0;
}

//**********************************************************************

const DataMap& FembTestAnalyzer::
processChannelEvent(Index icha, Index ievt) {
  const string myname = "FembTestAnalyzer::processChannelEvent: ";
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
  DataMap& res = results[icha][ievt];
  if ( res.haveInt("channel") ) return res;
  res.setInt("channel", icha);
  res.setInt("event", ievt);
  double pulseQfC = 0.0;
  if ( ievt > 1 ) pulseQfC = (ievt-1)*fembCfF()*0.001*fembVmV();
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
  for ( const std::unique_ptr<AdcChannelDataModifier>& pmod : adcModifiers ) {
    resmod += pmod->update(acd);
  }
  for ( const std::unique_ptr<AdcChannelViewer>& pvwr : adcViewers ) {
    resmod += pvwr->view(acd);
  }
  if ( dbg >= 2 ) resmod.print();
  res.setFloat("pedestal", acd.pedestal);
  if ( resmod.haveInt("roiCount") ) {
    vector<float> sigAreas[2];
    vector<float> sigHeights[2];
    Index nroi = resmod.getInt("roiCount");
    float areaMin[2] = {1.e9, 1.e9};
    float areaMax[2] = {0.0, 0.0};
    float heightMin[2] = {1.e9, 1.e9};
    float heightMax[2] = {0.0, 0.0};
    for ( Index iroi=0; iroi<nroi; ++iroi ) {
      float area = resmod.getFloatVector("roiSigAreas")[iroi];
      float sigmin = resmod.getFloatVector("roiSigMins")[iroi];
      float sigmax = resmod.getFloatVector("roiSigMaxs")[iroi];
      Index isPos = area >= 0.0;
      float sign = isPos ? 1.0 : -1.0;
      area *= sign;
      float height = isPos ? sigmax : -sigmin;
      sigAreas[isPos].push_back(area);
      sigHeights[isPos].push_back(height);
      if ( area < areaMin[isPos] ) areaMin[isPos] = area;
      if ( area > areaMax[isPos] ) areaMax[isPos] = area;
      if ( height < heightMin[isPos] ) heightMin[isPos] = height;
      if ( height > heightMax[isPos] ) heightMax[isPos] = height;
    }
    for ( Index isgn=0; isgn<2; ++isgn ) {
      string namsuf = isgn ? "Pos" : "Neg";
      string cntname = "sigCount" + namsuf;
      res.setInt(cntname, sigAreas[isgn].size());
      // Area
      {
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
      }
      // Height
      {
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
      }
    }
  } else {
    cout << myname << "ERROR: roiCount does not appear in output." << endl;
  }
  return res;
}

//**********************************************************************

DataMap FembTestAnalyzer::getChannelResponse(Index icha, bool usePos, bool useArea) {
  DataMap res;
  Index nevt = nEvent();
  Index nbin = nevt - 1;
  Index npt = nbin;
  string ssgn = usePos ? "Pos" : "Neg";
  string styp = useArea ? "Area" : "Height";
  string hnam = "hchaResp" + styp + ssgn;
  string gttl = "Response " + styp + " " + ssgn;
  string httl = gttl + " ; charge factor; Mean ADC " + styp;
  vector<float> x(nbin, 0.0);
  vector<float> dx(nbin, 0.0);
  vector<float> y(nbin, 0.0);
  vector<float> dy(nbin, 0.0);
  double ymax = 1000.0;
  vector<float> peds(nevt, 0.0);
  for ( Index ievt=2; ievt<nevt; ++ievt ) {
    Index ibin = ievt;
    const DataMap& resevt = processChannelEvent(icha, ievt);
    peds[ievt] = resevt.getFloat("pedestal");
    resevt.print();
    string meaName = "sig" + styp + "Mean" + ssgn;
    string rmsName = "sig" + styp + "Rms" + ssgn;
    float sigmean = resevt.getFloat(meaName);
    float sigrms  = resevt.getFloat(rmsName);
    float nele = resevt.getFloat("nElectron");
    Index ipt = ievt - 1;
    x[ipt] = 0.001*nele;
    y[ipt] = sigmean;
    dy[ipt] = sigrms;
    double ylim = y[ipt] + dy[ipt];
    if ( ylim > ymax ) ymax = ylim;
  }
  res.setFloatVector("peds", peds);
  httl = "Response " + styp + " " + ssgn + "; Charge [ke]; Mean ADC " + styp;
  string gnam = "gchaResp" + styp + ssgn;
  TGraph* pg = new TGraphErrors(nbin, &x[0], &y[0], &dx[0], &dy[0]);
  pg->SetName(gnam.c_str());
  pg->SetTitle(gttl.c_str());
  pg->SetLineWidth(2);
  //pg->SetMarkerStyle(2);
  pg->GetXaxis()->SetTitle("Input charge [ke]");
  pg->GetYaxis()->SetTitle("Output signal [ADC counts]");
  pg->Fit("pol1");
  res.setGraph(gnam, pg);
  return res;
}

//**********************************************************************

DataMap FembTestAnalyzer::processChannel(Index icha) {
  DataMap res;
  res += getChannelResponse(icha,  true,  true);
  res += getChannelResponse(icha, false,  true);
  res += getChannelResponse(icha,  true, false);
  res += getChannelResponse(icha, false, false);
  return res;
}

//**********************************************************************
