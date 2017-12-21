// FembTestPulseData.h

// David Adams
// December 2017
//
// Data for the FEMB test pulse tree.

#ifndef FembTestPulseData_H
#define FembTestPulseData_H

class FembTestPulseData {

public:

  using Index = unsigned int;

  // Run data.
  Index femb;   // FEMB ID
  Index gain;   // Gain index
  Index shap;   // Shaping Index
  bool  extp;   // True for external pulser.

  // Channel data.
  Index chan;   // Channel
  float ped0;   // ADC pedestal with no pulse

  // Event/sign data
  float qexp;   // Expected charge [ke]
  int sevt;     // Signed event index.

  // Channel/Event data
  float pede;   // ADC pedestal with pulses

  // Channel/Event/sign data
  float cmea;   // Calibrated signal mean
  float crms;   // Calibrated signal RMS;
  float stk1;   // sticky 1
  float stk2;   // sticky 1
  int   nsat;   // # saturated pulses

  // Pulse data
  std::vector<float> qcal;   // Measured charge with calibration.

};

#endif
