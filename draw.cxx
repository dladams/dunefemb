// draw.C
//
// David Adams
// October 2017
//
// Example script that draws a histogram created with a dunetpc tool.
//
//   femb - ID of the FEMB
//   tspat - Timestamp pattern used to select the FEMB sample
//   isCold - True to use data taken cold
//   igain - Preamp gain setting (0-3)
//   ishap - Shaping time setting (0-3 for 0.5, 1, 2, 3 us)
//   evt - Event number in file, typically 0 for no DAC, 1, ... for DAC = 0, ...
//   svwrs - Names of the ADC viewer tools separated by spaces
//           E.g. "adcPlotRaw adcPlotRawDist adcPedestalFit"

#include "DuneFembReader.h"
#include "DuneFembFinder.h"
#include "dune/ArtSupport/DuneToolManager.h"
#include "dune/DuneInterface/Data/DataMap.h"
#include "dune/DuneInterface/Tool/AdcChannelViewer.h"
#include "dune/DuneInterface/Tool/AdcChannelDataModifier.h"
#include "dune/DuneCommon/TPadManipulator.h"
#include "TLatex.h"
#include "TCanvas.h"
#include "TROOT.h"

DataMap draw(int femb, string tspat, bool isCold,
             int igain, int ishap,
             int evt,
             int chan, int nchan,
             string svwrs ="",
             double xmin= 0.0, double xmax =0.0,
             double ymin= 0.0, double ymax =0.0,
             bool save =false) {
  const string myname = "draw: ";
  //DuneFembReader rdr("/home/dladams/data/dune/femb/test/fembTest_gainenc_test_g3_s3_extpulse/gainMeasurement_femb_1-parseBinaryFile.root");
  static vector<TPadManipulator> mans;
  DuneToolManager* ptm = DuneToolManager::instance("dunefemb.fcl");
  DuneFembFinder ff;
  cout << "         FEMB: " << femb << endl;
  cout << "   TS pattern: " << tspat << endl;
  cout << "Shaping index: " << ishap << endl;
  auto prdr = ff.find(femb, isCold, tspat, igain, ishap, 1, 1);
  if ( prdr == 0 ) return DataMap(101);
  DuneFembReader& rdr = *prdr;
  cout << "Sample is " << rdr.label() << endl;
  if ( svwrs == "" ) svwrs = "adcPedestalFit";
  string::size_type ipos = 0;
  string::size_type npos = string::npos;
  vector<AdcChannelViewer*> vwrs;
  vector<string> vwrnames;
  while ( ipos < svwrs.size() ) {
   string::size_type jpos = svwrs.find(" ", ipos+1);
   if ( jpos == npos ) jpos = svwrs.size();
   string svwr = svwrs.substr(ipos, jpos - ipos);
   vwrnames.push_back(svwr);
   vwrs.push_back(ptm->getShared<AdcChannelViewer>(svwr));
   ipos = jpos;
   while ( ipos < svwrs.size() && svwrs[ipos] == ' ' ) ++ipos;
  }
  TLatex* plab = new TLatex(0.01, 0.01, rdr.label().c_str());
  plab->SetNDC();
  plab->SetTextFont(42);
  plab->SetTextSize(0.03);
  vector<TCanvas*> cans(vwrnames.size(), nullptr);
  int wwx = 700;
  int wwy = 500;
  int npad = 1;
  if ( nchan > 4 ) npad = 4;
  else if ( nchan > 1 ) npad = 2;
  int wx = npad*wwx;
  int wy = npad*wwy;
  if ( ! gROOT->IsBatch() && wy > 1000 ) { wx=1400; wy = 1000; }
  DataMap resOut;  // Save this to keep histograms.
  int ipad = 0;
  mans.clear();
  vector<string> modnames;
  modnames.push_back("adcPedestalFit");
  modnames.push_back("adcSampleFiller");
  modnames.push_back("adcThresholdSignalFinder");
  vector<std::unique_ptr<AdcChannelDataModifier>> mods;
  map<string, const AdcChannelDataModifier*> modmap;
  for ( string modname : modnames ) {
    auto pmod = ptm->getPrivate<AdcChannelDataModifier>(modname);
    if ( ! pmod ) {
      cout << myname << "Unable to find modifier " << modname << endl;
      return DataMap(101);
    }
    modmap[modname] = pmod.get();
    mods.push_back(std::move(pmod));
  }
  for ( int icha=chan; icha<chan+nchan; ++icha ) {
    cout << myname << "Processing channel " << icha << endl;
    if ( npad > 1 ) ++ipad;
    AdcChannelData acd;
    rdr.read(evt, icha, &acd);
    if ( acd.raw.size() == 0 ) {
      cout << "No data found for event " << evt << " channel " << chan << endl;
      return DataMap(102);
    }
    for ( string modname : modnames ) {
      cout << myname << "Applying modifier " << modname << endl;
      DataMap res = modmap[modname]->update(acd);
      res.print();
    }
    cout << myname << "Pedestal: " << acd.pedestal << endl;
    unsigned int nroi = acd.rois.size();
    cout << myname << "# ROI: " << nroi << endl;
    if ( nroi ) {
      if ( nroi > 10 ) nroi = 10;
      for ( unsigned int iroi=0; iroi<nroi; ++ iroi ) {
        cout << myname << "  " << iroi << ": [" << acd.rois[iroi].first << ", "
             << acd.rois[iroi].second << "]" << endl;
      }
    }
    for ( unsigned int ivwr=0; ivwr<vwrs.size(); ++ivwr ) {
      AdcChannelViewer* pvwr = vwrs[ivwr];
      DataMap res = pvwr->view(acd);
      res.print();
      resOut += res;
      for ( TH1* ph : res.getHists() ) {
        TCanvas* pcan = cans[ivwr];
        if ( pcan == nullptr ) {
          string vname = vwrnames[ivwr];
          pcan = new TCanvas(vname.c_str(), vname.c_str(), wx, wy);
          if ( npad > 1 ) pcan->Divide(npad, npad);
          cans[ivwr] = pcan;
        }
        pcan->cd(ipad);
        if ( xmax > xmin ) ph->GetXaxis()->SetRangeUser(xmin, xmax);
        if ( ymax > ymin ) ph->GetYaxis()->SetRangeUser(ymin, ymax);
        //ph->DrawCopy("HIST");
        ph->DrawCopy();
        mans.emplace_back();
        TPadManipulator& man = mans.back();
        man.addAxis();
        if ( ph->GetListOfFunctions()->GetEntries() ) ph->GetListOfFunctions()->At(0)->Draw("same");
        //for ( TObject* pf : *(ph->GetListOfFunctions()) ) pf->Draw("same");
        //man.drawVerticalModLines();
        if ( vwrnames[ivwr] == "adcPedestalFit" ) {
          man.setRangeY(0, 4000);
          //man.addHistFun(1);   // This is the starting function for the fit.
          man.addHistFun();
          man.showUnderflow();
          man.showOverflow();
          man.addVerticalModLines(64, 0, 1.03);
        }
      }
    }  // End loop over viewers
  }  // End loop over channels
  return resOut;
}
