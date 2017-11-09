// DuneFembFinder.cxx

#include "DuneFembFinder.h"
#include "DuneFembReader.h"
#include "dunesupport/FileDirectory.h"
#include <iostream>
#include <fstream>
#include <sstream>

using std::string;
using std::getline;
using std::cout;
using std::endl;
using std::ifstream;
using std::ostringstream;

namespace {
using Index = DuneFembFinder::Index;
using IndexVector = std::vector<Index>;
using NameVector = DuneFembFinder::NameVector;
using FembMap = DuneFembFinder::FembMap;
using RdrPtr = DuneFembFinder::RdrPtr;
using FileMap = FileDirectory::FileMap;
}

//**********************************************************************

DuneFembFinder::DuneFembFinder(string topdir)
: m_topdir(topdir) {
  const string myname = "DuneFembFinder::ctor: ";
  string ifname = topdir + "/" + "fembjson.dat";
  ifstream fin(ifname);
  if ( ! fin ) {
    cout << myname << "Unable to find FEMB run map at" << endl;
    cout << myname << ifname << endl;
    return;
  }
  string line;
  getline(fin, line);
  // Find the header positions.
  IndexVector poss(1, 0);
  NameVector headers;
  Index ipos = 0;
  Index npos = line.size();
  while ( ipos < npos ) {
    while ( line[ipos] == ' ' ) ++ipos;
    string slab;
    while ( ipos < npos && line[ipos] != ' ' ) slab += line[ipos++];
    headers.push_back(slab);
    poss.push_back(ipos);
  }
  Index nfld = headers.size();
  // Find the field positions for the FEMB ID and timestamp.
  Index iposFemb = 0;
  Index jposFemb = 0;
  Index iposTs   = 0;
  Index jposTs   = 0;
  Index iposTemp = 0;
  Index jposTemp = 0;
  for ( Index ifld=0; ifld<nfld; ++ifld ) {
    string slab = headers[ifld];
    Index ipos = poss[ifld];
    Index jpos = poss[ifld+1];
    if ( slab == "TS" ) {
      iposTs = ipos;
      jposTs = jpos;
    } else if ( slab == "FEMB" ) {
      iposFemb = ipos;
      jposFemb = jpos;
    } else if ( slab == "TEMP" ) {
      iposTemp = ipos;
      jposTemp = jpos;
    }
  }
  if ( jposTs <= iposTs ) {
    cout << myname << "Label TS not found in header line:" << endl;
    cout << line << endl;
    return;
  }
  if ( jposFemb <= iposFemb ) {
    cout << myname << "Label FEMB not found in header line:" << endl;
    cout << line << endl;
    return;
  }
  if ( jposTemp <= iposTemp ) {
    cout << myname << "Label TEMP not found in header line:" << endl;
    cout << line << endl;
    return;
  }
  // Loop over lines and fill the maps.
  while ( fin && ! fin.eof() ) {
    getline(fin, line);
    if ( line.size() == 0 ) continue;
    string ts;
    istringstream ssts(line.substr(iposTs, jposTs));
    ssts >> ts;
    Index fembId = 999999;
    istringstream ssfemb(line.substr(iposFemb, jposFemb));
    ssfemb >> fembId;
    string stemp;
    istringstream sstemp(line.substr(iposTemp, jposTemp));
    sstemp >> stemp;
    if ( stemp == "cold" ) {
      m_coldFembMap[fembId].push_back(ts);
    } else if ( stemp == "warm" ) {
      m_warmFembMap[fembId].push_back(ts);
    } else {
      cout << myname << "Skipping line with invalid temperature:" << endl;
      cout << line << endl;
      string fline;
      for ( Index ipos=0; ipos<iposTemp; ++ipos ) fline += ' ';
      for ( Index ipos=iposTemp; ipos<jposTemp; ++ipos ) fline += '^';
      cout << fline << endl;
    }
  }
}

//**********************************************************************

RdrPtr DuneFembFinder::find(string dir, string fpat) {
  const string myname = "DuneFembFinder::find: ";
  string dsdir = m_topdir + "/" + dir;
  FileDirectory ftopdir(dsdir);
  FileMap dsfiles = ftopdir.find(fpat);
  if ( dsfiles.size() == 0 ) {
    cout << myname << "Unable to find file pattern " << fpat << " in " << dsdir << endl;
    return nullptr;
  }
  if ( dsfiles.size() > 1 ) {
    cout << myname << "Found multiple matches:" << endl;
    for ( auto ent : dsfiles ) {
      cout << "  " << dsdir << "/" << ent.first << endl;
    }
    return nullptr;
  }
  string dsfile = dsdir + "/" + dsfiles.begin()->first;
  return std::move(RdrPtr(new DuneFembReader(dsfile, 123, 456)));
}

//**********************************************************************

RdrPtr DuneFembFinder::
find(string ts, Index gain, Index shap, bool a_extPulse, bool a_extClock) {
  const string myname = "DuneFembFinder::find: ";
  FileDirectory ftopdir(m_topdir);
  int ndir = ftopdir.select("wib");
  if ( ndir == 0 ) {
    cout << myname << "No wib directories found at " << m_topdir << endl;
    return nullptr;
  }
  vector<string> tsdirs;
  for ( auto& ent : ftopdir.files ) {
    string path = m_topdir + "/" + ent.first;
    FileDirectory fdir(path);
    fdir.select(ts);
    for ( auto tsent : fdir.files ) {
      tsdirs.push_back(fdir.dirname + "/" + tsent.first);
    }
  }
  if ( tsdirs.size() == 0 ) {
    cout << myname << "No match found for timestamp " << ts << endl;
    return nullptr;
  }
  if ( tsdirs.size() > 1 ) {
    cout << myname << "Multiple matches found for timestamp " << ts << ":" << endl;
    for ( string tsdir : tsdirs ) {
      cout << myname << "  " << tsdir << endl;
    }
    return nullptr;
  }
  FileDirectory tsdir(tsdirs[0]);
  ostringstream sspat;
  sspat << "fembTest_gainenc_test_g" << gain << "_s" << shap
        << "_" << ( a_extPulse ? "extpulse" : "intpulse" )
        << ( a_extClock ? "" : "_intclock" );
  string spat = sspat.str();
  FileMap dsdirs = tsdir.find(spat);
  // External clock files are distinguished by not having "intclock" in their file names.
  if ( a_extClock ) {
    vector<string> dropKeys;
    for ( const FileMap::value_type& ent : dsdirs ) {
      string key = ent.first;
      if ( key.find("_intclock") != string::npos ) dropKeys.push_back(key);
    }
    for ( string key : dropKeys ) dsdirs.erase(key);
  }
  if ( dsdirs.size() == 0 ) {
    cout << myname << "No match found for param pattern " << spat << endl;
    tsdir.print();
    return nullptr;
  }
  if ( dsdirs.size() > 1 ) {
    cout << myname << "Multiple matches (" << dsdirs.size() << ") found for param pattern "
         << spat << ":" << endl;
    for ( auto ent : dsdirs ) {
      cout << myname << "  " << ent.first << endl;
    }
    return nullptr;
  }
  FileDirectory dsdir(tsdir.dirname + "/" + dsdirs.begin()->first);
  FileMap dsfiles = dsdir.find("parseBinaryFile.root");
  if ( dsfiles.size() == 0 ) {
    cout << myname << "Binary file not found in " << tsdir.dirname << endl;
    return nullptr;
  }
  if ( dsfiles.size() > 1 ) {
    cout << myname << "Multiple binaries found in " << tsdir.dirname << ":" << endl;
    for ( auto ent : dsfiles ) {
      cout << myname << "  " << ent.first << endl;
    }
  }
  string dsfile = dsdir.dirname + "/" + dsfiles.begin()->first;
  RdrPtr prdr(new DuneFembReader(dsfile, 123, 456));
  prdr->setMetadata(gain, shap, a_extPulse, a_extClock);
  return std::move(prdr);
}

//**********************************************************************

RdrPtr DuneFembFinder::
find(Index fembId, bool isCold, string tspat,
Index gain, Index shap, bool extPulse, bool extClock) {
  const string myname = "DuneFembFinder::find: ";
  const FembMap& fembMap = isCold ? m_coldFembMap : m_warmFembMap;
  string stemp = isCold ? "cold" : "warm";
  FembMap::const_iterator ient = fembMap.find(fembId);
  if ( ient == fembMap.end() ) {
    cout << myname << "FEMB " << fembId << " is not in " << stemp << " map." << endl;
    return nullptr;
  }
  NameVector matchedTss;
  NameVector candidateTss  = ient->second;
  for ( string ts : candidateTss ) {
    if ( ts.find(tspat) != string::npos ) matchedTss.push_back(ts);
  }
  if ( matchedTss.size() == 0 ) {
    cout << myname << "No match found for FEMB " << fembId << " " << stemp
         << " with timestamp pattern " << tspat << endl;
    cout << myname << "Candidates are:" << endl;
    for ( string ts : candidateTss ) cout << myname << "  " << ts << endl;
    return nullptr;
  }
  if ( matchedTss.size() > 1 ) {
    cout << myname << "Multiple matches found for FEMB " << fembId << " " << stemp
         << " with timestamp pattern " << tspat << ":" << endl;
    for ( string ts : matchedTss ) cout << myname << "  " << ts << endl;
    return nullptr;
  }
  string myts = matchedTss.front();
  RdrPtr prdr = std::move(find(myts, gain, shap, extPulse, extClock));
  if ( prdr != nullptr ) {
    ostringstream sslab;
    sslab << "FEMB " << fembId << " g" << gain << " s" << shap
          << (tspat.size() ? " " : "") << tspat
          << " " << (isCold ? "cold" : "warm")
          << " " << (extPulse ? "ext" : "int") << "Pulse"
          << " " << (extClock ? "ext" : "int") << "Clock";
    prdr->setLabel(sslab.str());
  }
  return prdr;
}

//**********************************************************************
