// rootlogon.C
//
// David Adams
// August 2017
//
// Root startup file for dunefemb.

{
  cout << "Welcome to dunefemb." << endl;
  cout << endl;
  gSystem->SetBuildDir(".aclic");
  string dtinc = gSystem->Getenv("DUNETPC_INC");
  cout << "Dunetpc include dir: " << dtinc << endl;
  string sarg = "-I" + dtinc;
  gSystem->AddIncludePath(sarg.c_str());
  // Load the dunetpc and supporting libraries.
  vector<string> libs;
  libs.push_back("$FHICLCPP_LIB/libfhiclcpp");
  libs.push_back("$DUNETPC_LIB/libdune_ArtSupport");
  libs.push_back("$DUNETPC_LIB/libdune_DuneServiceAccess");
  libs.push_back("$DUNETPC_LIB/libdune_DuneCommon");
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
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneCommon/TPadManipulator.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/AdcChannelData.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/AdcChannelViewer.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/AdcDataViewer.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/HistogramManager.h+");
  gROOT->ProcessLine(".L $DUNETPC_LIB/libdune_DuneCommon.so");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/ArtSupport/ArtServiceHelper.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneServiceAccess/DuneServiceAccess.h+");
  // Build local classes.
  gROOT->ProcessLine(".L DuneFembReader.cxx+");
  gROOT->ProcessLine(".L dunesupport/FileDirectory.cxx+");
  gROOT->ProcessLine(".L DuneFembFinder.cxx+");
}