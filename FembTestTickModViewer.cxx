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
  Index nent = size();
  SelMap::const_iterator ient = m_sels.find(selname);
  // Return the selection if it already exists.
  if ( ient != m_sels.end() ) return ient->second;
  // Handle special selections all and empty.
  if ( selname == "empty" ) {
    m_sellabs[selname] = "empty";
    return m_sels[selname];
  }
  if ( selname == "all" ) {
    m_sellabs[selname] = "";
    IndexVector& sel = m_sels.emplace(selname, IndexVector(size())).first->second;
    for ( Index ient=0; ient<nent; ++ient ) sel[ient] = ient;
    return sel;
  }
  // Otherwise, build this selection.
  // if this is a combined selection, split into base and last subselection.
  // Otherwise the base is all.
  string::size_type ipos = selname.rfind("_");
  string basename;
  string remname;
  string baseLabel;
  if ( ipos == string::npos ) {
    basename = "all";
    remname  = selname;
  } else {
    basename = selname.substr(0, ipos);
    remname  = selname.substr(ipos + 1);
    baseLabel = selectionLabel(basename);
  }
  const IndexVector& selbase = selection(basename);
  string remLabel;
  float sigThresh = 5.0;
  if ( remname.substr(0,4) == "chan" ) {
    string scha = remname.substr(4);
    while ( scha.size() > 1 && scha[0] == '0' ) scha=scha.substr(1);
    istringstream sscha(scha);
    Index icha;
    sscha >> icha;
    IndexVector& sel = m_sels[selname];
    for ( Index ient : selbase ) {
      if ( read(ient)->chan == icha ) sel.push_back(ient);
    }
    ostringstream sslab;
    sslab << "channel " << icha;
    remLabel = sslab.str();
  } else if ( remname.substr(0,3) == "adc" ) {
    string sadc = remname.substr(3);
    while ( sadc.size() > 1 && sadc[0] == '0' ) sadc=sadc.substr(1);
    istringstream ssadc(sadc);
    Index iadc;
    ssadc >> iadc;
    IndexVector& sel = m_sels[selname];
    for ( Index ient : selbase ) {
      if ( read(ient)->chan/16 == iadc ) sel.push_back(ient);
    }
    ostringstream sslab;
    sslab << "ADC " << iadc;
    remLabel = sslab.str();
  } else if ( remname == "nosat" ) {
    IndexVector& sel = m_sels[selname];
    for ( Index ient : selbase ) {
      if ( read(ient)->nsat == 0 ) sel.push_back(ient);
    }
    remLabel = remname;
  } else if ( remname == "pedestal" ) {
    IndexVector& sel = m_sels[selname];
    for ( Index ient : selbase ) {
      if ( fabs(read(ient)->cmea) <= sigThresh ) sel.push_back(ient);
    }
    remLabel = remname;
  } else if ( remname == "signal" ) {
    IndexVector& sel = m_sels[selname];
    for ( Index ient : selbase ) {
      if ( fabs(read(ient)->cmea) > sigThresh ) sel.push_back(ient);
    }
    remLabel = remname;
  } else {
    cout << myname << "Invalid selection word: " << remname << endl;
    return selection("empty");
  }
  // Check the selection was properly defined.
  if ( m_sels.find(selname) == m_sels.end() ) {
    cout << myname << "ERROR: selection was not inserted for " << selname << endl;
    return selection("empty");
  }
  if ( remLabel.size() == 0 ) {
    cout << myname << "ERROR: selection label was not created for subselection " << remname << endl;
    return selection("empty");
  }
  // Record a label for this selection: "baseLabel remLabel";
  string slab = baseLabel;
  if (  slab.size() ) slab += " ";
  slab += remLabel;
  m_sellabs[selname] = slab;
  return m_sels[selname];
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
  ssttlsuf << "FEMB " << read(0)->femb;
  string sellab = selectionLabel(selname);
  if ( sellab.size() ) ssttlsuf << " " << sellab;
  string ttlsuf = ssttlsuf.str();
  // Create and add plot to the manipulator.
  if ( sopt == "q" || sopt == "qw" ) {
    double xmin = qmin();
    double xmax = qmax();
    int nbin = xmax - xmin + 0.1;
    string syunit;
    if ( sopt == "qw" ) {
      man.setCanvasSize(1400, 500);
      syunit = "/ke";
    } else {
      nbin = nbin/10;
      syunit = "/(10 ke)";
    }
    string ttl = "Charge illumination " + ttlsuf + "; Q [ke]; # samples [" + syunit + "]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      ph->Fill(read(ient)->cmea, read(ient)->qcal.size());
    }
    man.add(ph, "hist");
    man.setLogRangeY(0.5, 1.e6);
    man.setLogY();
    man.setGridY(true);
    return draw(&man);
  } else if ( sopt == "adc" || sopt == "adcw" ) {
    int xmin = 0;
    int xmax = 4096;
    int nbin = xmax - xmin;
    string syunit;
    if ( sopt == "adcw" ) {
      man.setCanvasSize(1400, 500);
      nbin = nbin/8;
      syunit = "/(8 ADC)";
    } else {
      nbin = nbin/64;
      syunit = "/(64 ADC)";
    }
    string ttl = "ADC illumination " + ttlsuf + "; ADC count; # samples [" + syunit + "]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      for ( short iadc : read(ient)->radc ) ph->Fill(iadc);
    }
    man.add(ph, "hist");
    man.setLogRangeY(0.5, 1.e6);
    man.setLogY();
    man.addVerticalModLines(64);
    man.setGridY(true);
    return draw(&man);
  } else if ( sopt == "qres" ) {
    double xmin = -3.0;
    double xmax =  3.0;
    int nbin = 60;
    string syunit = "(0.1 ke)";
    string ttl = "Charge residual " + ttlsuf + "; Q - <Q> [ke]; # samples [" + syunit + "]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      float cmea = read(ient)->cmea;
      for ( float qcal : read()->qcal ) {
        ph->Fill(qcal - cmea);
      }
    }
    man.add(ph);
    man.showUnderflow();
    man.showOverflow();
    return draw(&man);
  } else if ( sopt == "qrms" ) {
    double xmin =  0.0;
    double xmax =  5.0;
    int nbin = 50;
    string syunit = "(0.1 ke)";
    string ttl = "Local charge RMS " + ttlsuf + "; RMS(Q - <Q>) [ke]; # ticksets" + syunit + "]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      ph->Fill(read(ient)->crms);
    }
    man.add(ph);
    man.showUnderflow();
    man.showOverflow();
    return draw(&man);
  } else if ( sopt == "stk1" || sopt == "stk2" ) {
    double xmin =  0.0;
    double xmax =  1.0;
    int nbin = 40;
    string svar = sopt == "stk1" ? "S_{1}" : "S_{2}";
    string ttl = "Local charge RMS " + ttlsuf + "; " + svar + "; # ticksets";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      float s = ( sopt == "stk1" ) ? read(ient)->stk1 : read(ient)->stk2;
      if ( s == 1.0 ) s = 0.9999;
      ph->Fill(s);
    }
    man.add(ph);
    man.showUnderflow();
    man.showOverflow();
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
