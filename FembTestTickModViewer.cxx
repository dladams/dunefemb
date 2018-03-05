// FembTestTickModViewer.cxx

#include "FembTestTickModViewer.h"
#include <iostream>
#include <sstream>
#include "TH1F.h"
#include "TLegend.h"
#include "TROOT.h"
#include "dune/DuneCommon/LineColors.h"

using std::string;
using std::cout;
using std::endl;
using std::ostringstream;
using std::istringstream;

namespace {
using Name = FembTestTickModViewer::Name;
using Index = FembTestTickModViewer::Index;
using IndexVector = FembTestTickModViewer::IndexVector;
using Selection = FembTestTickModViewer::Selection;
}

//**********************************************************************

FembTestTickModViewer::
FembTestTickModViewer(FembTestTickModTree& a_tmt, Index icha1, Index ncha)
: m_tmt(a_tmt) {
  for ( Index icha=icha1; icha<icha1+ncha; ++icha ) tmt().getTree(icha);
  for ( Index icha=0; icha<tmt().size(); ++icha ) {
    if ( tmt().haveTree(icha) ) m_chans.push_back(icha);
  }
}

//**********************************************************************

FembTestTickModViewer::
FembTestTickModViewer(string fname, Index icha1, Index ncha)
: m_ptmtOwned(new FembTestTickModTree(fname, "READ")),
  m_tmt(*m_ptmtOwned) {
  string lab = fname;
  if ( lab.substr(0, 18) == "femb_test_tickmod_" ) lab = lab.substr(18);
  Index len = lab.size();
  if ( lab.substr(len-5) == ".root" ) lab = lab.substr(0, len-5);
  m_label = lab;
  for ( Index icha=icha1; icha<icha1+ncha; ++icha ) tmt().getTree(icha);
  for ( Index icha=0; icha<tmt().size(); ++icha ) {
    if ( tmt().haveTree(icha) ) m_chans.push_back(icha);
  }
}

//**********************************************************************

void FembTestTickModViewer::help() const {
  cout << endl;
  cout << "FembTestTickModViewer provides standard selections and plots for viewing\n"
       << "tickmod trees. A plot may be displayed with viewer tmv using the syntax:\n"
       << "\n"
       << "  tmv.draw(myfig, mysel);\n"
       << "\n"
       << "where myfig is any of the following:\n"
       << "\n"
       << "  adc - ADC illumination with bin size of 64 ADC units\n"
       << "  adcw - ADC illumination with bin size of 8 ADC units\n"
       << "  qcal - Calibrated charge illumination with bin size of 10 ke (tickmod mean charge)\n"
       << "  qcalw - Calibrated charge illumination with bin size of 1 ke (tickmod mean charge)\n"
       << "  qexp - Peak charge illumination with bin size of 10 ke (pulse input charge)\n"
       << "  qexpw - Peak charge illumination with bin size of 1 ke (pulse input charge)\n"
       << "  qres - Charge residual (calibrated - mean)\n"
       << "  mod64 - ADC LSB (ADC%64) for each sample\n"
       << "  sfmx - Fraction of samples in peak of ADC LSB for each tickmod\n"
       << "  sf63 - Fraction of samples with ADC LSB = 63 for each tickmod\n"
       << "\n"
       << "The selection string mysel is any combination of the following separated\n"
       << "with underscores:\n"
       << "\n"
       << "  chanXXX - only channel XXX\n"
       << "     adcX - only ADC X\n"
       << "     evtX - only event (subrun) X\n"
       << "    nosat - exclude tickmods with any saturated ADC samples\n"
       << "      ped - pedestal region: |qcal| < " << pedLimit << " ke\n"
       << "      sig - signal region: |qcal| >= " << pedLimit << " ke\n"
       << "      mip - MIP region: " << pedLimit << " <= |qcal| <= " << mipLimit << " ke\n"
       << "      hvy - Heavy ionization region: |qcal| >= " << mipLimit << "50 ke\n"
       << "\n"
       << "The call to draw returns a TPadManipulator pointer.\n"
       << "It may be replaced with a call to pad to return the pointer without updating\n"
       << "the screen or with a call to hist to return the primary histogram."
       << endl;
}

//**********************************************************************

const Selection& FembTestTickModViewer::selection(Name selname) {
  const string myname = "FembTestTickModViewer::sel: ";
  SelMap::const_iterator ient = m_sels.find(selname);
  // Return the selection if it already exists.
  if ( ient != m_sels.end() ) return ient->second;
  // Handle special selections all and empty.
  if ( selname == "empty" ) {
    m_sellabs[selname] = "empty";
    m_selcols[selname] = LineColors::blue();
    return m_sels[selname];
  }
  if ( selname == "all" ) {
    m_sellabs[selname] = "";
    m_selcols[selname] = LineColors::blue();
    auto sstat = m_sels.emplace(selname, Selection());
    if ( sstat.second ) {
      auto itsel = sstat.first;
      for ( Index icha : channels() ) {
        Index nidx = tmt().size(icha);
        Selection& sel = itsel->second;
        if ( sel.container().find(icha) != sel.container().end() ) {
          cout << myname << "Duplicate channel " << icha << " in selection all!" << endl;
        } else {
          Selection::C2& idxs = itsel->second.container()[icha];
          idxs.resize(nidx);
          for ( Index iidx=0; iidx<idxs.size(); ++iidx ) idxs[iidx] = iidx;
        }
      }
    }
    return m_sels[selname];
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
  const Selection& selbase = selection(basename);
  string badLabel = "SelectionNotFound";
  string remLabel = badLabel;;
  Index selcol = m_selcols[basename];
  if ( remname.substr(0,3) == "all" ) {
    m_sels[selname] = selbase;
    remLabel = "";
  } else if ( remname.substr(0,4) == "chan" ) {
    Selection& sel = m_sels[selname];
    const Index badidx = 99999999;
    Index icha = badidx;
    istringstream sscha(remname.substr(4));
    sscha >> icha;
    if ( icha != badidx ) {
      const Selection::C1& selmap = selbase.container();
      const Selection::C1::const_iterator ient = selmap.find(icha);
      if ( ient != selmap.end() ) sel.container()[icha] = ient->second;
      ostringstream sslab;
      sslab << "channel " << icha;
      remLabel = sslab.str();
    }
  } else if ( remname.substr(0,3) == "adc" ) {
    string sadc = remname.substr(3);
    while ( sadc.size() > 1 && sadc[0] == '0' ) sadc=sadc.substr(1);
    Selection& sel = m_sels[selname];
    istringstream ssadc(sadc);
    Index iadc;
    ssadc >> iadc;
    Index icha1 = 16*iadc;
    Index icha2 = icha1 + 16;
    const Selection::C1& selmap = selbase.container();
    for ( Index icha=icha1; icha<icha2; ++icha ) {
      const Selection::C1::const_iterator ient = selmap.find(icha);
      if ( ient != selmap.end() ) sel.container()[icha] = ient->second;
    }
    ostringstream sslab;
    sslab << "ADC " << iadc;
    remLabel = sslab.str();
  } else if ( remname.substr(0,3) == "evt" ) {
    Selection& sel = m_sels[selname];
    const Index badevt = 99999999;
    Index ievt = badevt;
    istringstream ssevt(remname.substr(3));
    ssevt >> ievt;
    if ( ievt != badevt ) {
      for ( Selection::const_iterator isel=selbase.begin(); isel!=selbase.end(); ++isel ) {
        Index icha = isel.key1();
        Index iidx = isel.value();
        if ( tmt().read(icha, iidx)->ievt == ievt ) sel.container()[icha].push_back(iidx);
      }
      ostringstream sslab;
      sslab << "event " << ievt;
      remLabel = sslab.str();
    }
  } else if ( remname.substr(0,5) == "nosat" ) {
    Selection& sel = m_sels[selname];
    for ( Selection::const_iterator isel=selbase.begin(); isel!=selbase.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      if ( tmt().read(icha, iidx)->nsat == 0 ) sel.container()[icha].push_back(iidx);
    }
    remLabel = remname;
  } else if ( remname == "ped" ) {
    Selection& sel = m_sels[selname];
    for ( Selection::const_iterator isel=selbase.begin(); isel!=selbase.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      if ( fabs(tmt().read(icha, iidx)->cmea) < pedLimit ) sel.container()[icha].push_back(iidx);
    }
    remLabel = remname;
    selcol = LineColors::brown();
  } else if ( remname == "sig" ) {
    Selection& sel = m_sels[selname];
    for ( Selection::const_iterator isel=selbase.begin(); isel!=selbase.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      if ( fabs(tmt().read(icha, iidx)->cmea) >= pedLimit ) sel.container()[icha].push_back(iidx);
    }
    remLabel = remname;
    selcol = LineColors::violet();
  } else if ( remname == "mip" ) {
    Selection& sel = m_sels[selname];
    for ( Selection::const_iterator isel=selbase.begin(); isel!=selbase.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      float aq = fabs(tmt().read(icha, iidx)->cmea);
      if ( aq >= pedLimit && aq < mipLimit) sel.container()[icha].push_back(iidx);
    }
    remLabel = remname;
    selcol = LineColors::green();
  } else if ( remname == "hvy" ) {
    Selection& sel = m_sels[selname];
    for ( Selection::const_iterator isel=selbase.begin(); isel!=selbase.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      if ( fabs(tmt().read(icha, iidx)->cmea) >= mipLimit ) sel.container()[icha].push_back(iidx);
    }
    remLabel = remname;
    selcol = LineColors::red();
  } else {
    cout << myname << "Invalid selection word: " << remname << endl;
    return selection("empty");
  }
  // Check the selection was properly defined.
  if ( m_sels.find(selname) == m_sels.end() ) {
    cout << myname << "ERROR: selection was not inserted for " << selname << endl;
    return selection("empty");
  }
  if ( remLabel == badLabel ) {
    cout << myname << "ERROR: selection label was not created for subselection " << remname << endl;
    return selection("empty");
  }
  // Record a label for this selection: "baseLabel remLabel";
  string slab = baseLabel;
  if ( slab.size() && remLabel.size() ) slab += " ";
  slab += remLabel;
  m_sellabs[selname] = slab;
  m_selcols[selname] = selcol;
  return m_sels[selname];
}

//**********************************************************************

Name FembTestTickModViewer::selectionLabel(Name selname) {
  selection(selname);  // Make sure name is defined.
  return m_sellabs[selname];
}

//**********************************************************************

TPadManipulator* FembTestTickModViewer::pad(string sopt, Name selname, Name selovls) {
  const string myname = "FembTestTickModViewer::pad: ";
  if ( sopt == "" ) {
    help();
    return nullptr;
  }
  Index ncha = channels().size();
  if ( ncha == 0 ) {
    cout << myname << "ERROR: There are no channel trees." << endl;
    return nullptr;
  }
  // If manipulator already exists, return it.
  string mnam = sopt + "_" + selname;
  if ( selovls.size() ) mnam += "-" + selovls;
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  // Create a new pad manipulator.
  TPadManipulator& man = m_mans[mnam];
  // Handle multiple selections.
  if ( selovls.size() ) {
    string::size_type ipos = 0;
    string::size_type jpos = 0;
    vector<string> ovlsels;
    while ( ipos != string::npos && ipos<selovls.size() ) {
      jpos = selovls.find(":", ipos);
      if ( jpos == string::npos ) jpos = selovls.size();
      string selovl = selovls.substr(ipos, jpos-ipos);
      ovlsels.push_back(selovl);
      ipos = jpos == string::npos ? string::npos : jpos+1;
    }
    bool first = true;
    int newSty = 1;
    int newWid = 2;
    for ( string selovl : ovlsels ) {
      string fulsel = selname + "_" + selovl;
      if ( first ) {
        const TPadManipulator* ppad = pad(sopt, fulsel);
        if ( ppad == nullptr ) {
          cout << myname << "Unable to find base manipulator for selection " << fulsel << endl;
          return nullptr;
        }
        man = *ppad;
        first = false;
        // Use the title with no extra selection.
        const TPadManipulator* ppadt = pad(sopt, selname);
        man.setTitle(ppadt->getTitle());
      } else {
        man.add(hist(sopt, fulsel), "same hist");
        TH1* ph = dynamic_cast<TH1*>(man.objects().back().get());
        if ( ph != nullptr ) {
          ph->SetLineStyle(++newSty);
          ph->SetLineWidth(++newWid);
        }
      }
    }
    return &man;
  }  
  const Selection& sel = selection(selname);
  if ( sel.size() == 0 ) {
    cout << myname << "WARNING: No entries found for selection \""
         << selname << "\"." << endl;
  }
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
  Selection::const_iterator isel = sel.begin();
  Index icha0 = isel.key1();
  ssttlsuf << "FEMB " << read(icha0,0)->femb;    // We are guaranteed 1st selection vector is not empty.
  string sellab = selectionLabel(selname);
  if ( sellab.size() ) ssttlsuf << " " << sellab;
  string ttlsuf = ssttlsuf.str();
  const SelectionContainer& selcon = sel.container();  // STL map
  // Create and add plot to the manipulator.
  Index selcol = m_selcols[selname];
  if ( sopt == "chan" ) {
    int nbin = 128;
    double xmin = 0.0;
    double xmax = nbin;
    string ttl = "Channel occupancy " + ttlsuf + "; Channel; # entries [/channel]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    ph->SetMinimum(0.0);
    ph->SetLineColor(selcol);
    for ( const SelectionContainer::value_type selval : selcon ) {
      ph->Fill(selval.first, selval.second.size());
    }
    man.add(ph, "hist");
    man.addVerticalModLines(16);
    return &man;
  } else if ( sopt == "qcal" || sopt == "qcalw" || sopt == "qexp" || sopt == "qexpw" ) {
    double xmin = qmin();
    double xmax = qmax();
    int nbin = xmax - xmin + 0.1;
    string syunit;
    bool usecal = sopt == "qcal" || sopt == "qcalw";
    bool usew = sopt == "qcalw" || sopt == "qexpw";
    if ( usew ) {
      man.setCanvasSize(1400, 500);
      syunit = "/ke";
    } else {
      nbin = nbin/10;
      syunit = "/(10 ke)";
    }
    string ttl = usecal ? "Calibrated" : "Expected peak";
    string sq = usecal ? "<Q>_{tm}" : "Q_{exp}";
    ttl += " charge illumination " + ttlsuf + "; " + sq + " [ke]; # samples [" + syunit + "]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    ph->SetLineColor(selcol);
    for ( Selection::const_iterator isel=sel.begin(); isel!=sel.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      if ( usecal ) ph->Fill(read(icha, iidx)->cmea, read(icha, iidx)->qcal.size());
      else ph->Fill(read(icha, iidx)->qexp, read(icha, iidx)->qcal.size());
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
    ph->SetLineColor(selcol);
    for ( Selection::const_iterator isel=sel.begin(); isel!=sel.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      for ( short iadc : read(icha, iidx)->radc ) ph->Fill(iadc);
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
    ph->SetLineColor(selcol);
    for ( Selection::const_iterator isel=sel.begin(); isel!=sel.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      float cmea = read(icha, iidx)->cmea;
      for ( float qcal : read()->qcal ) {
        ph->Fill(qcal - cmea);
      }
    }
    man.add(ph);
    man.showUnderflow();
    man.showOverflow();
    return &man;
  } else if ( sopt == "qrms" || sopt == "qrmsn" ) {
    double xmin =  0.0;
    double xmax =  5.0;
    int nbin = 50;
    string syunit = "(0.1 ke)";
    string sytype = sopt == "qrmsn" ? "Fraction of" : "#";
    string ttl = "Local charge RMS " + ttlsuf + "; RMS(Q - <Q>) [ke]; " + sytype + " ticksets [/" + syunit + "]";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    ph->SetLineColor(selcol);
    for ( Selection::const_iterator isel=sel.begin(); isel!=sel.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      ph->Fill(read(icha, iidx)->crms);
    }
    ph->SetMinimum(0.0);
    if ( sopt == "qrmsn" ) {
      if( sel.size() ) ph->Scale(1.0/sel.size());
      ph->SetMaximum(0.5);
    }
    man.add(ph, "hist");
    man.showUnderflow();
    man.showOverflow();
    //man.addVerticalModLines(1.0);
    man.addVerticalLine(0.3, 1.0, 3);
    man.addVerticalLine(1.0, 1.0, 3);
    return &man;
  } else if ( sopt == "sfmx" || sopt == "sfmxn" || sopt == "sf63" || sopt == "sf63n" ) {
    double xmin =  0.0;
    double xmax =  1.0;
    int nbin = 40;
    bool isfrac = sopt == "sfmxn" || sopt == "sf63n";
    bool is63 = sopt == "sf63" || sopt == "sf63n";
    string ttl;
    if ( is63 ) ttl = "Mod63";
    else ttl = "Peak";
    string sytype = isfrac ? "Fraction of" : "#";
    ttl += " ADC bin fraction " + ttlsuf + "; Fraction; " + sytype + " ticksets";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    ph->SetLineColor(selcol);
    for ( Selection::const_iterator isel=sel.begin(); isel!=sel.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      float frac = -1.0;
      if ( is63 ) frac = read(icha, iidx)->sf63;
      else frac = read(icha, iidx)->sfmx;
      if ( frac == 1.0 ) frac = 0.9999;
      ph->Fill(frac);
    }
    if ( isfrac ) {
      if ( sel.size() ) ph->Scale(1.0/sel.size());
      ph->SetMinimum(0.0);
      ph->SetMaximum(0.3);
    }
    man.add(ph, "hist");
    man.showUnderflow();
    man.showOverflow();
    man.addVerticalLine(0.30, 1.0, 3);
    if ( ! is63 ) man.addVerticalLine(0.10, 1.0, 3);
    return &man;
  } else if ( sopt == "mod64" || sopt == "mod64n" ) {
    double xmin =  0.0;
    double xmax = 64.0;
    int nbin = 64;
    string ttl = "ADC LSB " + ttlsuf + "; ADC%64; # ticks";
    TH1* ph = new TH1F(mnam.c_str(), ttl.c_str(), nbin, xmin, xmax);
    ph->SetStats(0);
    ph->SetLineWidth(2);
    ph->SetLineColor(selcol);
    ph->SetMinimum(0.0);
    for ( Selection::const_iterator isel=sel.begin(); isel!=sel.end(); ++isel ) {
      Index icha = isel.key1();
      Index iidx = isel.value();
      for ( Index adc : read(icha, iidx)->radc ) {
        ph->Fill(adc%64);
      }
    }
    double yexp = 1.0/64;
    double nent = ph->GetEntries();
    if ( sopt == "mod64n" ) {
      if ( nent > 0.0 ) ph->Scale(1.0/nent);
      ph->SetMaximum(0.3);
    } else {
      yexp *= nent;
    }
    man.add(ph, "hist");
    man.showUnderflow();
    man.showOverflow();
    man.addHorizontalLine(yexp, 1.0, 2);
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
    man.addVerticalModLines(16);
    string ttl = pad("fmod63ch", selname)->getTitle();
    string::size_type ipos = ttl.find("mod63 fraction");
    ttl.replace(ipos, 14, "mod fractions");
    man.setTitle(ttl);
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
    ph->SetLineColor(selcol);
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

TPadManipulator* FembTestTickModViewer::padAdc(Index iadc, string sopt, Name selname, Name selovls) {
  const string myname = "FembTestTickModViewer::pad: ";
  ostringstream ssnam;
  if ( iadc > 7 ) return nullptr;
  ssnam << "adc" << iadc << "_" << sopt << "_" << selname << "_" << selovls;
  string mnam = ssnam.str();
  ManMap::iterator iman = m_mans.find(mnam);
  if ( iman != m_mans.end() ) return &iman->second;
  TPadManipulator& man = m_mans[mnam];
  int wy = gROOT->IsBatch() ? 2000 : 1000;
  man.setCanvasSize(1.4*wy, wy);
  man.split(4);
  bool doDrawSave = doDraw();
  for ( Index kcha=0; kcha<16; ++kcha ) {
    Index icha = 16*iadc + kcha;
    ostringstream ssel;
    if ( selname.size() ) ssel << selname << "_";
    ssel << "chan";
    if ( icha < 100 ) ssel << "0";
    if ( icha < 10 ) ssel << "0";
    ssel << icha;
    TPadManipulator* pman = pad(sopt, ssel.str(), selovls);
    if ( pman == nullptr ) {
      cout << myname << "Unable to find plot " << sopt << " for selection " << ssel.str() << endl;
      m_mans.erase(mnam);
      return nullptr;
    }
    *man.man(kcha) = *pman;
  }
  return &man;
}

//**********************************************************************

TPadManipulator* FembTestTickModViewer::draw(string sopt, Name selname, Name selovls) {
  TPadManipulator* pman = pad(sopt, selname, selovls);
  if ( pman != nullptr && doDraw() ) pman->draw();
  return pman;
}

//**********************************************************************

TPadManipulator* FembTestTickModViewer::drawAdc(Index iadc, string sopt, Name selname, Name selovls) {
  TPadManipulator* pman = padAdc(iadc, sopt, selname, selovls);
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
