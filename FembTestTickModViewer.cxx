// FembTestTickModViewer.cxx

#include "FembTestTickModViewer.h"
#include <iostream>
#include "TH1F.h"

using std::string;
using std::cout;
using std::endl;

namespace {
using Index = FembTestTickModViewer::Index;
}

//**********************************************************************

FembTestTickModViewer::
FembTestTickModViewer(FembTestTickModTree& a_tmt)
: m_tmt(a_tmt) { }

//**********************************************************************

FembTestTickModViewer::
FembTestTickModViewer(string fname)
: m_ptmtOwned(new FembTestTickModTree(fname, "READ")),
  m_tmt(*m_ptmtOwned) { }

//**********************************************************************

TPadManipulator* FembTestTickModViewer::draw(string sopt) {
  const string myname = "FembTestTickModViewer::draw: ";
  if ( sopt == "" ) {
    cout << myname << "   draw(\"q\") - Charge distribution" << endl;
    return nullptr;
  }
  string mnam = sopt;
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  Index nent = size();
  TPadManipulator& man = m_mans[mnam];
  if ( mnam == "q" ) {
    TH1* ph = new TH1F("hq", "Tickmod charge; Q [ke]; # Entries", 50, -150, 350);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient=0; ient<nent; ++ient ) {
      ph->Fill(read(ient)->cmea);
    }
    man.add(ph);
    man.setLogRangeY(0.5, 1.e6);
    man.setLogY();
    return draw(&man);
  }
  cout << myname << "Invalid plot name: " << sopt << endl;
  m_mans.erase(mnam);
  return nullptr;
}

//**********************************************************************

TPadManipulator* FembTestTickModViewer::draw(TPadManipulator* pman) const {
  if ( doDraw() ) pman->draw();
  return pman;
}

//**********************************************************************
