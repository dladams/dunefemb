// FembTestTickModViewer.cxx

#include "FembTestTickModViewer.h"
#include <iostream>
#include <sstream>
#include "TH1F.h"
#include "TLegend.h"

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
    // If any channel selection is requested, construct all channel selections.
    // First make the prefix for all selection names.
    string selprefix;
    if ( basename != "all" ) selprefix = basename + "_";
    selprefix += "chan";
    // If channel selections for this base are already present and this is not in
    // that set, it has invalid format.
    if ( m_sels.find(selprefix + "000") != m_sels.end() ) {
      cout << myname << "ERROR: Invalid format for channel selection: " << remname << endl;
      return m_sels["empty"];
    }
    // First make selection names for all channels and
    // a selection list and label for each name.
    vector<vector<string>> chselnames(128);
    for ( Index icha=0; icha<128; ++icha ) {
      ostringstream sscha;
      sscha << icha;
      string scha = sscha.str();
      chselnames[icha].push_back(selprefix + scha);
      if ( icha < 100 ) chselnames[icha].push_back(selprefix + "0" + scha);
      if ( icha <  10 ) chselnames[icha].push_back(selprefix + "00" + scha);
      string slab = baseLabel;
      if ( slab.size() ) slab += " ";
      slab += "channel " + scha;
      for ( string chselname : chselnames[icha] ) {
        m_sellabs[chselname] = slab;
      }
    }
    // Loop over events and add them to the appropriate selection vectors.
    for ( Index ient : selbase ) {
      Index icha = read(ient)->chan;
      for ( string chselname : chselnames[icha] ) {
        m_sels[chselname].push_back(ient);
      }
    }
    // Return the requested selection.
    return selection(selname);
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

TPadManipulator* FembTestTickModViewer::pad(string sopt, Name selname) {
  const string myname = "FembTestTickModViewer::pad: ";
  if ( sopt == "" ) {
    cout << myname << "     draw(\"q\") - Charge illumination" << endl;
    cout << myname << "    draw(\"qw\") - Charge illumination (wide)" << endl;
    cout << myname << "   draw(\"adc\") - ADC illumination" << endl;
    cout << myname << "  draw(\"adcw\") - ADC illumination (wide)" << endl;
    cout << myname << "  draw(\"qres\") - Charge residual" << endl;
    cout << myname << "  draw(\"qrms\") - Charge RMS" << endl;
    cout << myname << "  draw(\"sfmx\") - Bin peak stuck code fraction" << endl;
    cout << myname << "  draw(\"sf63\") - Mod63 stuck code fraction" << endl;
    cout << myname << "  draw(\"sf63\") - Mod63 stuck code fraction" << endl;
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
    return &man;
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
    return &man;
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
    return &man;
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
    return &man;
  } else if ( sopt == "sfmx" || sopt == "sf63" ) {
    double xmin =  0.0;
    double xmax =  1.0;
    int nbin = 40;
    string ttl;
    if ( sopt == "sfmx" ) ttl = "Peak";
    else ttl = "Mod63";
    ttl += " ADC bin fraction " + ttlsuf + "; Fraction; # ticksets";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      float frac = -1.0;
      if ( sopt == "sfmx" ) frac = read(ient)->sfmx;
      if ( sopt == "sf63" ) frac = read(ient)->sf63;
      if ( frac == 1.0 ) frac = 0.9999;
      ph->Fill(frac);
    }
    man.add(ph);
    man.showUnderflow();
    man.showOverflow();
    return &man;
  } else if ( sopt == "mod64") {
    double xmin =  0.0;
    double xmax = 64.0;
    int nbin = 64;
    string ttl = "ADC LSB " + ttlsuf + "; ADC%64; # ticks";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index ient : sel ) {
      for ( Index adc : read(ient)->radc ) {
        ph->Fill(adc%64);
      }
    }
    man.add(ph);
    man.showUnderflow();
    man.showOverflow();
    return &man;
  } else if ( sopt == "fmodsch" ) {
    TH1* ph63 = hist("fmod63ch", selname);
    TH1* ph00 = hist("fmod00ch", selname);
    TH1* ph01 = hist("fmod01ch", selname);
    string dopt = "hist";
    man.add(ph63, dopt);
    TH1* ph63new = man.hist();
    dopt += " same";
    man.add(ph00, dopt);
    TH1* ph00new = dynamic_cast<TH1*>(man.objects().back().get());
    man.add(ph01, dopt);
    TH1* ph01new = dynamic_cast<TH1*>(man.objects().back().get());
    ph00new->SetLineStyle(2);
    ph01new->SetLineStyle(3);
    TLegend* pleg = man.addLegend(0.60, 0.73, 0.93, 0.88);
    pleg->AddEntry(ph63new, "ADC%64 == 63", "l");
    pleg->AddEntry(ph00new, "ADC%64 == 0", "l");
    pleg->AddEntry(ph01new, "ADC%64 == 1", "l");
    man.setRangeY(0,1);
    return &man;
  // fmod00, fmod01, fmod64 for dists
  // Plus suffix "ch" for vs. channel.
  } else if ( sopt.substr(0,4) == "fmod" ) {
    static vector<string> sufs = {"00", "01", "63"};
    static vector<string> labmods = {"mod00", "mod01", "mod63"};
    static vector<Index> imods = {0, 1, 63};
    bool isDist = sopt.size() == 6;
    bool isChan = sopt.size() == 8 && sopt.substr(6,2) == "ch";
    string suf;
    Index ityp = sufs.size();
    if ( isDist || isChan ) {
      string insuf = sopt.substr(4,2);
      for ( ityp=0; ityp<sufs.size(); ++ityp ) if ( sufs[ityp] == insuf ) break;
    }
    if ( ityp == sufs.size() ) {
      cout << myname << "ERROR: Invalid fmod plot specifier: " << sopt << endl;
      return nullptr;
    }
    string labmod = labmods[ityp];
    Index imod = imods[ityp];
    Index ncha = 128;
    double xmin =   0;
    double xmax = ncha;
    int nbin = ncha;
    TH1* ph = nullptr;
    string sopt = "";
    if ( isDist ) {
      string ttl = "ADC " + labmod + " fraction " + ttlsuf + "; Fraction; # channels [/0.01]";
      ph = new TH1F(mnam.c_str(), ttl.c_str(), 50, 0.0, 0.5);
      man.showUnderflow();
      man.showOverflow();
    } else {
      string ttl = "ADC " + labmod + " fraction " + ttlsuf + "; Channel; Fraction";
      ph = new TH1F(mnam.c_str(), ttl.c_str(), ncha, 0, ncha);
      man.addVerticalModLines(16);
      man.setRangeY(0,1);
      sopt = "hist";
    }
    ph->SetStats(0);
    ph->SetLineWidth(2);
    for ( Index icha=0; icha<ncha; ++icha ) {
      ostringstream ssout;
      ssout << "chan";
      if ( icha < 10 ) ssout << "0";
      if ( icha < 100 ) ssout << "0";
      ssout << icha;
      if ( selname.size() ) ssout << "_" << selname;
      string chselname = ssout.str();
      TH1* phm = hist("mod64", chselname);
      if ( phm == nullptr ) {
        cout << myname << "ERROR: Unable to fetch mod64 hist for fmod64. Selection is "
             << selname << endl;
        if ( isDist ) ph->Fill(-1.0);
      } else {
        double nmod = phm->GetBinContent(1+imod);
        double ntot = phm->Integral();
        double frac = ntot > 0 ? nmod/ntot : 0.0;
        if ( isDist ) ph->Fill(frac);
        else          ph->Fill(icha, frac);
      }
    }
    man.add(ph, sopt);
    return &man;
  }
  cout << myname << "Invalid plot name: " << sopt << endl;
  m_mans.erase(mnam);
  return nullptr;
}

//**********************************************************************

TPadManipulator* FembTestTickModViewer::draw(string sopt, Name selname) {
  TPadManipulator* pman = pad(sopt, selname);
  if ( pman != nullptr && doDraw() ) pman->draw();
  return pman;
}

//**********************************************************************

TH1* FembTestTickModViewer::hist(string sopt, Name selname) {
  TPadManipulator* pman = pad(sopt, selname);
  if ( pman != nullptr ) return pman->hist();
  return nullptr;
}

//**********************************************************************
