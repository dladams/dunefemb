// rootlogon_setup.C
//
// David Adams
// August 2017
//
// Root startup file fo dunefemb.

// Define service helper.
ArtServiceHelper& ash = ArtServiceHelper::load("standard_reco_dune35tdata.fcl");

// Declare global tool manager.
DuneToolManager* ptm = nullptr;
HistogramManager* phm = nullptr;

int rootlogon_setup() {
  ptm = DuneToolManager::instance("dunefemb.fcl");
  phm = ptm->getShared<HistogramManager>("adcHists");
  return 0;
}
