void draw(int femb, string tspat, bool isCold, int evt, int chan, string svwr ="", int gset =2, bool save =false) {
  //DuneFembReader rdr("/home/dladams/data/dune/femb/test/fembTest_gainenc_test_g3_s3_extpulse/gainMeasurement_femb_1-parseBinaryFile.root");
  DuneFembFinder ff;
  cout << "Gain setting: " << gset << endl;
  auto prdr = ff.find(femb, isCold, tspat, gset, 3, 1, 1);
  if ( prdr == 0 ) return;
  DuneFembReader& rdr = *prdr;
  if ( svwr == "" ) svwr = "adcPedestalFit";
  vector<AdcChannelViewer*> vwrs;
  //vwrs.push_back(ptm->getShared<AdcChannelViewer>("adcPlotRaw"));
  //vwrs.push_back(ptm->getShared<AdcChannelViewer>("adcPlotRawDist"));
  vwrs.push_back(ptm->getShared<AdcChannelViewer>(svwr));
  AdcChannelData acd;
  rdr.read(evt, chan, &acd);
  if ( acd.raw.size() == 0 ) {
    cout << "No data found for event " << evt << " channel " << chan << endl;
    return;
  }
  for ( auto pvwr : vwrs ) {
    TCanvas* pcan = new TCanvas;
    pvwr->view(acd);
    if ( svwr == "adcPedestalFit" ) cout << "Pedestal: " << acd.pedestal << endl;
    if ( save ) {
      ostringstream ssnam;
      ssnam << "cadc_femb" << femb << "_" ;
      if ( tspat.size() ) ssnam << tspat << "_";
      ssnam << ( isCold ? "cold" : "warm")
            << "_ev" << evt << "_adc" << chan/16 << "_chan" << chan%16 << ".png";
      string ofnam = ssnam.str();
      pcan->Print(ofnam.c_str());
    }
  }
}
