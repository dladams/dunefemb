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
//   svwrs - Names of the ADC viewer tools seprated by spaces
//           E.g. "adcPlotRaw adcPlotRawDist adcPedestalFit"
DataMap draw(int femb, string tspat, bool isCold, int evt, int igain, int ishap,
             int chan, string svwrs ="",
             double xmin= 0.0, double xmax =0.0,
             double ymin= 0.0, double ymax =0.0,
             bool save =false) {
  //DuneFembReader rdr("/home/dladams/data/dune/femb/test/fembTest_gainenc_test_g3_s3_extpulse/gainMeasurement_femb_1-parseBinaryFile.root");
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
  while ( ipos < svwrs.size() ) {
   string::size_type jpos = svwrs.find(" ", ipos+1);
   if ( jpos == npos ) jpos = svwrs.size();
   string svwr = svwrs.substr(ipos, jpos - ipos);
   vwrs.push_back(ptm->getShared<AdcChannelViewer>(svwr));
   ipos = jpos;
   while ( ipos < svwrs.size() && svwrs[ipos] == ' ' ) ++ipos;
  }
  AdcChannelData acd;
  rdr.read(evt, chan, &acd);
  if ( acd.raw.size() == 0 ) {
    cout << "No data found for event " << evt << " channel " << chan << endl;
    return DataMap(102);
  }
  DataMap resOut;
  TLatex* plab = new TLatex(0.01, 0.01, rdr.label().c_str());
  plab->SetNDC();
  plab->SetTextFont(42);
  plab->SetTextSize(0.03);
  for ( auto pvwr : vwrs ) {
    TCanvas* pcan = nullptr;
    DataMap res = pvwr->view(acd);
    res.print();
    resOut += res;
    for ( TH1* ph : res.getHists() ) {
      TCanvas* pcan = new TCanvas;
      if ( xmax > xmin ) ph->GetXaxis()->SetRangeUser(xmin, xmax);
      if ( ymax > ymin ) ph->GetYaxis()->SetRangeUser(ymin, ymax);
      ph->DrawCopy();
      for ( TObject* pf : *(ph->GetListOfFunctions()) ) pf->Draw("same");
    }
  }
  return resOut;
}
