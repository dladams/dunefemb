// moddiff.cxx

#ifndef moddiff_H
#define moddiff_H

// Returns the smallest difference i1-i2, modulus nmod.
int moddiff(unsigned int nmod, int i1, int i2) {
  if ( nmod == 0 ) return 0;
  int idif = i1 - i2;
  int dmax = nmod/2;
  while ( idif < -dmax ) idif += nmod;
  while ( idif >  dmax ) idif -= nmod;
  return idif;
}

#endif
