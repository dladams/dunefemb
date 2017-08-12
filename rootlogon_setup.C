// rootlogon_setup.C
//
// David Adams
// August 2017
//
// Root startup file fo dunefemb.

// Declare global tool manager.
DuneToolManager* ptm = nullptr;

int rootlogon_setup() {
  ptm = DuneToolManager::instance("dunefemb.fcl");
  return 0;
}
