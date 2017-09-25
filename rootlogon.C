// rootlogon.C
//
// David Adams
// August 2017
//
// Root startup file for dunefemb.

{
  const char* cdtinc = gSystem->Getenv("DUNETPC_INC");
  if ( cdtinc == nullptr ) {
    cout << "rootlogon: ERROR: dunetpc must be set up." << endl;
  } else {
    gROOT->ProcessLine(".X rootlogon_load.C");
    gROOT->ProcessLine(".X rootlogon_setup.C");
    cout << "Done" << endl;
  }
}
