// FembTestTickModViewer.h

// David Adams
// December 2017
//
// View a FEMB test pulse tree.

#ifndef FembTestTickModViewer_H
#define FembTestTickModViewer_H

#include "FembTestTickModTree.h"
#include "dunesupport/NestedContainer.h"
#include "dune/DuneCommon/TPadManipulator.h"
#include <memory>

class FembTestTickModViewer {

public:

  using ManMap = std::map<std::string, TPadManipulator>;
  using Index = unsigned int;
  using Name = std::string;
  using IndexVector = std::vector<Index>;
  using SelectionContainer = std::map<Index, IndexVector>;  // Index vectors indexed by channel
  using Selection = NestedContainer<SelectionContainer>;
  //using Selection = std::map<Index, IndexVector>;    // Index vectors indexed by channel
  using SelMap = std::map<Name, Selection>;
  using NameMap = std::map<Name, Name>;
  using IndexMap = std::map<Name, Index>;

  // Ctor from tree and file holding tree.
  // Tree data is read for channels [icha1, icha1+ncha).
  FembTestTickModViewer(FembTestTickModTree& a_tmt, Index icha1 =0, Index ncha =1); 
  FembTestTickModViewer(std::string fname, Index icha1 =0, Index ncha =1); 

  // Display help message.
  void help() const;

  // Getters.
  FembTestTickModTree& tmt() { return m_tmt; }
  const FembTestTickModTree& tmt() const { return m_tmt; }

  // Channels with trees.
  const IndexVector& channels() const { return m_chans; }
  const FembTestTickModData* read(Index icha, Index ient) { return tmt().read(icha, ient); }
  const FembTestTickModData* read() { return tmt().read(); }
  bool doDraw() const { return m_doDraw; }
  Name label() const { return m_label; }
  const Selection& selection(Name selname);
  Name selectionLabel(Name selname);
  double qmin() const { return -130.0; }
  double qmax() const { return  220.0; }
  const ManMap& pads() const { return m_mans; }

  // Setters.
  bool doDraw(bool val) { return m_doDraw = val; }
  void setLabel(Name a_label) { m_label = a_label; }
  
  // Get plot sopt for selection selname.
  // Use sopt="" for help.
  // Draw also puts plot on a canvas.
  // hist returns the histogram associated with a pad.
  TPadManipulator* pad(std::string sopt ="", Name selname ="all", Name selovls ="");
  TPadManipulator* draw(std::string sopt ="", Name selname ="all", Name selovls ="");
  TH1* hist(std::string sopt ="", Name selname ="all");

  // Create a pad with a plot for channel in an ADC.
  TPadManipulator* padAdc(Index iadc, std::string sopt ="", Name selname ="all", Name selovls ="");
  TPadManipulator* drawAdc(Index iadc, std::string sopt ="", Name selname ="all", Name selovls ="");

private:

  std::unique_ptr<FembTestTickModTree> m_ptmtOwned;
  FembTestTickModTree& m_tmt;
  ManMap m_mans;
  bool m_doDraw =true;
  Name m_label;
  IndexVector m_chans;  // Channels with a tree.
  SelMap m_sels;        // Selection vectors indexed by name
  NameMap m_sellabs;    // Selection labels indexed by name
  IndexMap m_selcols;   // Selection colors indexed by name
  float pedLimit =  5.0;
  float mipLimit = 50.0;

};

#endif
