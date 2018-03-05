// FembTestTickModData.h

// David Adams
// December 2017
//
// Data for the FEMB test pulse tree.

#ifndef FembTestTickModData_H
#define FembTestTickModData_H

class FembTestTickModData {

public:

  using Index = unsigned int;

  // Run data.
  Index femb;   // FEMB ID
  Index gain;   // Gain index
  Index shap;   // Shaping Index
  bool  extp;   // True for external pulser.
  int   ntmd;   // Tick period = # ticks in each tickset

  // Channel data.
  Index chan;   // Channel
  float ped0;   // ADC pedestal with no pulse

  // Event data
  float qexp;   // Expected charge for the signal peak (not the tickmod) [ke]
  Index ievt;   // Event index.
  Index itmx;   // Tickmod with maximum mean charge
  Index itmn;   // Tickmod with minimum mean charge
  float effq;   // Fraction of cmea>5 samples within 2.5 ke of cmea.
  float effp;   // Fraction of cmea<=5 samples within 2.5 ke of cmea.

  // Channel/Event data
  float pede;   // ADC pedestal with pulses

  // Tickmod data
  int   itmd;               // Tickmod (0 <= itmd < ntmd)
  float cmea;               // Calibrated signal mean
  float crms;               // Calibrated signal RMS
  float sadc;               // Most common ADC code
  float adcm;               // ADC mean
  float adcn;               // ADC mean without most common code
  float efft;               // Fraction of samples within 2.5 ke of mean.
  float sfmx;               // sticky max fraction
  float sf00;               // sticky mod0 fraction1
  float sf01;               // sticky mod1 fraction1
  float sf63;               // sticky mod63 fraction
  int   nsat;               // # saturated pulses
  std::vector<short> radc;  // Raw ADC for each tickset.
  std::vector<float> qcal;  // Measured charge for each tickset sample.

};

#endif
