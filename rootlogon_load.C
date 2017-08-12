// rootlogon.C
//
// David Adams
// August 2017
//
// Root startup file fo dunefemb.

{
  cout << "Welcome to dunefemb." << endl;
  cout << endl;
  gSystem->SetBuildDir(".aclic");
  string dtinc = gSystem->Getenv("DUNETPC_INC");
  if ( dtinc.size() ) {
    cout << "Dunetpc include dir: " << dtinc << endl;
    string sarg = "-I" + dtinc;
    gSystem->AddIncludePath(sarg.c_str());
  } else {
    cout << "ERROR: Dunetpc is not set up." << endl;
    return 1;
  }
  // Load the dunetpc and supporting libraries.
  vector<string> libs;
  libs.push_back("$FHICLCPP_LIB/libfhiclcpp");
  libs.push_back("$DUNETPC_LIB/libdune_ArtSupport");
  string libext = "so";
  string arch = gSystem->GetBuildArch();
  if ( arch.substr(0,3) == "mac" ) libext = "dylib";
  for ( string lib : libs ) {
    string libpath = lib + "." + libext;
    string libres = gSystem->ExpandPathName(libpath.c_str());
    if ( 1 ) cout << "AddLinkedLibs: " << libres << endl;
    gSystem->AddLinkedLibs(libres.c_str());
  }
  // Load the classes we would like available on the command line.
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/ArtSupport/DuneToolManager.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/AdcChannelData.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/AdcDataViewer.h+");
  // Build local classes.
  gROOT->ProcessLine(".L DuneFembReader.cxx+");
}

