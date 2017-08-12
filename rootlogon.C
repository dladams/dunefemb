// rootlogon.C
//
// David Adams
// August 2017
//
// Root startup file fo dunefemb.

{
  gROOT->ProcessLine(".X rootlogon_load.C");
  gROOT->ProcessLine(".X rootlogon_setup.C");
  cout << "Done" << endl;
}
