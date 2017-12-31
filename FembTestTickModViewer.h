// FembTestTickModViewer.h

// David Adams
// December 2017
//
// View a FEMB test pulse tree.

#ifndef FembTestTickModViewer_H
#define FembTestTickModViewer_H

#include "FembTestTickModTree.h"
#include "dune/DuneCommon/TPadManipulator.h"
#include <memory>

class FembTestTickModViewer {

public:

  using ManMap = std::map<std::string, TPadManipulator>;
  using Index = unsigned int;

  FembTestTickModViewer(FembTestTickModTree& a_tmt); 
  FembTestTickModViewer(std::string fname); 

  // Getters.
  FembTestTickModTree& tmt() { return m_tmt; }
  const FembTestTickModTree& tmt() const { return m_tmt; }
  Index size() const { return tmt().size(); }
  const FembTestTickModData* read(Index ient) { return tmt().read(ient); }
  bool doDraw() const { return m_doDraw; }
  bool doDraw(bool val) { return m_doDraw = val; }
  
  TPadManipulator* draw(std::string sopt ="");

private:

  std::unique_ptr<FembTestTickModTree> m_ptmtOwned;
  FembTestTickModTree& m_tmt;
  ManMap m_mans;
  bool m_doDraw =true;

  TPadManipulator* draw(TPadManipulator* pman) const;
};

#endif
