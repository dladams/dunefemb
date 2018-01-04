// rootlogon.C
//
// David Adams
// August 2017
//
// Root startup file for dunefemb.

{
  cout << "Welcome to dunefemb." << endl;
  string dtver;
  try {
    const char* cdtver = gSystem->Getenv("DUNETPC_VERSION");
    if ( cdtver != nullptr ) dtver = cdtver;
  } catch(...) {
    cout << "Must first set up dunetpc." << endl;
    exit(1);
  }
/*  cout << endl;
*/
  if ( dtver.size() == 0 ) {
    cout << "Must first set up dunetpc." << endl;
    exit(1);
  }
  string buildDir = gSystem->Getenv("ACLICDIR");
  if ( buildDir.size() == 0 ) {
    buildDir = ".aclic_" + dtver;
  }
  cout << "ACLiC dir: " << buildDir << endl;
  gSystem->SetBuildDir(buildDir.c_str());
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
  cout << "Loading dunetpc classes." << endl;
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/ArtSupport/DuneToolManager.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneCommon/TPadManipulator.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/AdcChannelData.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/AdcChannelViewer.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/AdcChannelDataModifier.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/AdcDataViewer.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneInterface/Tool/HistogramManager.h+");
  string sline = ".L $DUNETPC_LIB/libdune_DuneCommon." + libext;
  gROOT->ProcessLine(sline.c_str());
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/ArtSupport/ArtServiceHelper.h+");
  gROOT->ProcessLine(".L $DUNETPC_INC/dune/DuneServiceAccess/DuneServiceAccess.h+");
  // Build local classes.
  cout << "Loading local classes." << endl;
  gROOT->ProcessLine(".L moddiff.h+");
  gROOT->ProcessLine(".L StickyCodeMetrics.cxx+");
  gROOT->ProcessLine(".L DuneFembReader.cxx+");
  gROOT->ProcessLine(".L dunesupport/FileDirectory.cxx+");
  gROOT->ProcessLine(".L DuneFembFinder.cxx+");
  gROOT->ProcessLine(".L FembTestPulseTree.cxx+");
  gROOT->ProcessLine(".L FembTestTickModTree.cxx+");
  gROOT->ProcessLine(".L FembTestTickModViewer.cxx+");
  gROOT->ProcessLine(".L FembTestAnalyzer.cxx+");
  gROOT->ProcessLine(".L FembDatasetAnalyzer.cxx+");
  gROOT->ProcessLine(".L draw.cxx+");
  cout << "Finished loading." << endl;
}
