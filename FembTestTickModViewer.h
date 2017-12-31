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
  using Name = std::string;
  using IndexVector = std::vector<Index>;
  using SelMap = std::map<Name, IndexVector>;
  using NameMap = std::map<Name, Name>;

  FembTestTickModViewer(FembTestTickModTree& a_tmt); 
  FembTestTickModViewer(std::string fname); 

  // Getters.
  FembTestTickModTree& tmt() { return m_tmt; }
  const FembTestTickModTree& tmt() const { return m_tmt; }
  Index size() const { return tmt().size(); }
  const FembTestTickModData* read(Index ient) { return tmt().read(ient); }
  bool doDraw() const { return m_doDraw; }
  Name label() const { return m_label; }
  const IndexVector& selection(Name selname);
  Name selectionLabel(Name selname);

  // Setters.
  bool doDraw(bool val) { return m_doDraw = val; }
  void setLabel(Name a_label) { m_label = a_label; }
  
  // Draw plot sopt for selection selname.
  // Use sopt="" for help.
  TPadManipulator* draw(std::string sopt ="", Name selname ="all");

private:

  std::unique_ptr<FembTestTickModTree> m_ptmtOwned;
  FembTestTickModTree& m_tmt;
  ManMap m_mans;
  bool m_doDraw =true;
  Name m_label;
  SelMap m_sels;     // Selection vectors indexed by name
  NameMap m_sellabs;  // Selection labels indexed by name

  TPadManipulator* draw(TPadManipulator* pman) const;
};

#endif
