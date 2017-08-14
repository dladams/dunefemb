// test_DuneFembReader.cxx

#include "DuneFembReader.h"
#include "dune/DuneInterface/AdcChannelData.h"
#include <string>
#include <iostream>

using std::string;
using std::cout;
using std::endl;

//**********************************************************************

namespace {

template<typename T1, typename T2>
int check(T1 t1, T2 t2, string msg ="") {
  if ( t1 != t2 ) {
    cout << "Failed";
    if ( msg.size() ) cout << ": " << msg;
    cout << ": " << t1 << " != " << t2;
    cout << endl;
    return 1;
  } 
  if ( true ) {
    cout << "Passed";
    if ( msg.size() ) cout << ": " << msg;
    cout << endl;
  }
  return 0;
}

}  // end unnamed namespace
    
//**********************************************************************

int test_DuneFembReader() {
  const string myname = "test_DuneFembReader: ";

  using Index = DuneFembReader::Index;
  using Entry = DuneFembReader::Entry;
  using Waveform = DuneFembReader::Waveform;

  string fname = "/home/dladams/data/dune/femb/test/fembTest_gainenc_test_g3_s3_extpulse/gainMeasurement_femb_1-parseBinaryFile.root";
  DuneFembReader rdr(fname);
  std::vector<Index> subruns = {4,   4,   4};
  std::vector<Index> chans =  {25,  26,  25};
  std::vector<Index> ents =  {537, 538, 537};
  int nerr = 0;
  Index ntst = subruns.size();
  const Waveform* pwf = nullptr;
  for ( Index itst=0; itst<ntst; ++itst ) {
    cout << myname << "Test " << itst << endl;
    Index subrun = subruns[itst];
    Index chan = chans[itst];
    Entry ient = ents[itst];
    Waveform::size_type nsam = 19499;
    nerr += check(rdr.waveform(), pwf, "before find waveform");
    pwf = nullptr;
    nerr += check(rdr.find(subrun, chan), ient, "find");
    nerr += check(rdr.entry(), ient, "ntry");
    nerr += check(rdr.subrun(), subrun, "subrun");
    nerr += check(rdr.channel(), chan, "chan");
    nerr += check(rdr.waveform(), pwf, "after find waveform");
    rdr.find(11, 111);
    AdcChannelData acd;
    nerr += check(rdr.read(subrun, chan, &acd), 0, "read");
    nerr += check(rdr.entry(), ient, "after read entry");
    nerr += check(rdr.subrun(), subrun, "after read subrun");
    nerr += check(rdr.channel(), chan, "after read chan");
    pwf = rdr.waveform();
    nerr += check(pwf->size(), nsam, "after read waveform");
  }
  cout << myname << "Error count: " << nerr << endl;
  return nerr;
}

//**********************************************************************
