// rootlogon_setup.C
//
// David Adams
// August 2017
//
// Root startup file fo dunefemb.

#define ADDSERVICES 0

// Define service helper.
#if ADDSERVICES
ArtServiceHelper& ash = ArtServiceHelper::load("standard_reco_dune35tdata.fcl");
#endif

// Declare global tool manager.
DuneToolManager* ptm = nullptr;
HistogramManager* phm = nullptr;

int rootlogon_setup() {
  cout << "Fetching tool manager." << endl;
  ptm = DuneToolManager::instance("dunefemb.fcl");
  cout << "Fetching histogram manager." << endl;
  phm = ptm->getShared<HistogramManager>("adcHists");
#if ADDSERVICES
  cout << "     Service helper: ash" << endl;
#endif
  cout << "       Tool manager: ptm" << endl;
  cout << "  Histogram manager: phm" << endl;
  return 0;
}
