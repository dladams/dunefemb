// StickyCodeMetrics.cxx

#include "StickyCodeMetrics.h"
#include <map>

using std::map;

namespace {
using Index = unsigned int;
}

//**********************************************************************

StickyCodeMetrics::StickyCodeMetrics(const AdcCountVector& adcs) {
  map<AdcCount, Index> counts;
  int maxadc = -1;
  Index maxCount = 0;
  Index nmod0 = 0;
  Index nmod1 = 0;
  Index nmod63 = 0;
  Index adcsum = 0;
  for ( AdcCount adc : adcs ) {
    if ( counts.find(adc) == counts.end() ) counts[adc] = 0;
    ++counts[adc];
    if ( counts[adc] > maxCount ) {
      maxCount = counts[adc];
      maxadc = adc;
    }
    AdcCount adcmod = adc%64;
    if ( adcmod ==  0 ) ++nmod0;
    if ( adcmod ==  1 ) ++nmod1;
    if ( adcmod == 63 ) ++nmod63;
    adcsum += adc;
  }
  Index adcCount2 = 0;
  Index adcsum2 = 0;
  for ( auto ent : counts ) {
    AdcCount adc = ent.first;
    Index count = ent.second;
    if ( adc != maxadc ) {
      adcCount2 += count;
      adcsum2 += count*adc;
    }
  }
  double count = adcs.size();
  double count2 = adcCount2;
  m_maxAdc = maxadc;
  m_meanAdc = count>0 ? adcsum/count : -1.0;
  m_meanAdc2 = count2>0 ? adcsum2/count2 : -1.0;
  m_maxFraction = maxCount/count;
  m_zeroFraction = nmod0/count;
  m_oneFraction = nmod1/count;
  m_highFraction = nmod63/count;
}

//**********************************************************************
