// FembTestTickModViewer.cxx

#include "FembTestTickModViewer.h"
#include <iostream>
#include <sstream>
#include "TH1F.h"

using std::string;
using std::cout;
using std::endl;
using std::ostringstream;
using std::istringstream;

namespace {
using Name = FembTestTickModViewer::Name;
using Index = FembTestTickModViewer::Index;
using IndexVector = FembTestTickModViewer::IndexVector;
}

//**********************************************************************

FembTestTickModViewer::
FembTestTickModViewer(FembTestTickModTree& a_tmt)
: m_tmt(a_tmt) { }

//**********************************************************************

FembTestTickModViewer::
FembTestTickModViewer(string fname)
: m_ptmtOwned(new FembTestTickModTree(fname, "READ")),
  m_tmt(*m_ptmtOwned) {
  string lab = fname;
  if ( lab.substr(0, 18) == "femb_test_tickmod_" ) lab = lab.substr(18);
  Index len = lab.size();
  if ( lab.substr(len-5) == ".root" ) lab = lab.substr(0, len-5);
  m_label = lab;
}

//**********************************************************************

const IndexVector& FembTestTickModViewer::selection(Name selname) {
  const string myname = "FembTestTickModViewer::sel: ";
  Name ssel = selname;
  Index nent = size();
  SelMap::const_iterator ient = m_sels.find(selname);
  if ( ient != m_sels.end() ) return ient->second;
  if ( ssel == "empty" ) {
    m_sellabs[ssel] = "empty";
    return m_sels[ssel];
  }
  if ( ssel == "all" ) {
    m_sellabs[ssel] = "";
    IndexVector& sel = m_sels.emplace(ssel, IndexVector(size())).first->second;
    for ( Index ient=0; ient<nent; ++ient ) sel[ient] = ient;
    return sel;
  }
  IndexVector& sel = m_sels[ssel];
  if ( ssel.substr(0,4) == "chan" ) {
    string scha = ssel.substr(4);
    while ( scha.size() > 1 && scha[0] == '0' ) scha=scha.substr(1);
    istringstream sscha(scha);
    Index icha;
    sscha >> icha;
    for ( Index ient=0; ient<nent; ++ient ) {
      if ( read(ient)->chan == icha ) sel.push_back(ient);
    }
    ostringstream sslab;
    sslab << "channel " << icha;
    m_sellabs[ssel] = sslab.str();
  }
  return sel;
}

//**********************************************************************

Name FembTestTickModViewer::selectionLabel(Name selname) {
  selection(selname);  // Make sure name is defined.
  return m_sellabs[selname];
}

//**********************************************************************

TPadManipulator* FembTestTickModViewer::draw(string sopt, Name selname) {
  const string myname = "FembTestTickModViewer::draw: ";
  if ( sopt == "" ) {
    cout << myname << "   draw(\"q\") - Charge distribution" << endl;
    return nullptr;
  }
  if ( size() == 0 ) {
    cout << myname << "ERROR: Tree has no entries." << endl;
    return nullptr;
  }
  const IndexVector sel = selection(selname);
  if ( sel.size() == 0 ) {
    cout << myname << "WARNING: No entries found for selection \""
         << selname << "\"." << endl;
  }
  string mnam = sopt + "_" + selname;
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  Index nent = size();
  // Create a new pad manipulator.
  TPadManipulator& man = m_mans[mnam];
  // Create and add sample label.
  TLatex* plab = new TLatex(0.01, 0.015, label().c_str());
  plab->SetNDC();
  plab->SetTextSize(0.035);
  plab->SetTextFont(42);
  man.add(plab, "");
  // Add top and right axis.
  man.addAxis();
  // Create suffix for the plot title.
  ostringstream ssttlsuf;
  ssttlsuf << " FEMB " << read(0)->femb;
  string sellab = selectionLabel(selname);
  if ( sellab.size() ) ssttlsuf << " " << sellab;
  string ttlsuf = ssttlsuf.str();
  // Create and add plot to the manipulator.
  if ( sopt == "q" ) {
    string ttl = "Tickmod charge " + ttlsuf + "; Q [ke]; # Entries";
    TH1* ph = new TH1F("hq", ttl.c_str(), 50, -150, 350);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      ph->Fill(read(ient)->cmea, read(ient)->qcal.size());
    }
    man.add(ph, "hist");
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
