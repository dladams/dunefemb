void draw16(int femb, string tspat="", bool isCold=true, int evt=0, int adc1 =0, int adc2 =8) {
  //DuneFembReader rdr("/home/dladams/data/dune/femb/test/fembTest_gainenc_test_g3_s3_extpulse/gainMeasurement_femb_1-parseBinaryFile.root");
  DuneFembFinder ff;
  auto prdr = ff.find(femb, isCold, tspat, 3, 3, 1, 1);
  if ( prdr == 0 ) return;
  DuneFembReader& rdr = *prdr;
  vector<AdcChannelTool*> vwrs;
  //vwrs.push_back(ptm->getShared<AdcChannelTool>("adcPlotRaw"));
  //vwrs.push_back(ptm->getShared<AdcChannelTool>("adcPlotRawDist"));
  vwrs.push_back(ptm->getShared<AdcChannelTool>("adcPedestalFit"));
  double wy =1000;
  if ( gROOT->IsBatch() ) wy = 2000;
  double wx = 1.4 * wy;
  AdcChannelData acd;
  for ( int iadc=adc1; iadc<adc2; ++iadc ) {
    ostringstream ssnam;
    ssnam << "cadc" << iadc;
    string cnam = ssnam.str();
    TCanvas* pcan = new TCanvas(cnam.c_str(), cnam.c_str(), 2800, 2000);
    pcan->Divide(4,4);
    int chan = 16*iadc;
    for ( int ach=0; ach<16; (++ach, ++chan) ) {
      rdr.read(evt, chan, &acd);
      pcan->cd(ach+1);
      for ( auto pvwr : vwrs ) {
        pvwr->view(acd);
        cout << "> ";
        cout.flush();
      }
    }
    ssnam.str("");
    ssnam << "cadc_femb" << femb << "_" ;
    if ( tspat.size() ) ssnam << tspat << "_";
    ssnam << ( isCold ? "cold" : "warm")
          << "_ev" << evt << "_adc" << iadc << ".png";
    string ofnam = ssnam.str();
    pcan->Print(ofnam.c_str());
  }
}

