#include "fx-resofilter.h"

/*
Peter Semiletov, 2023
*/


/*
this code based on Martin Finke's filter tutorial,
where the filter itself based on the resonant filter 
by Paul Kellett http://www.musicdsp.org/showone.php?id=29
*/


float CResoFilter::process (float sample)
{
  bufl0 += cutoff * (sample - bufl0 + feedback_amount * (bufl0 - bufl1)); //add reso
  bufl1 += cutoff * (bufl0 - bufl1);
  bufl2 += cutoff * (bufl1 - bufl2);
  bufl3 += cutoff * (bufl2 - bufl3);

  switch (mode)
         {
          case FILTER_MODE_LOWPASS:
               return bufl1;

          case FILTER_MODE_HIGHPASS:
               return sample - bufl0;

          case FILTER_MODE_BANDPASS:
               return bufl0 - bufl3;

          default:
                  return 0.0f;
         }

  return 0.0f;
}


CResoFilter::CResoFilter()
{
  cutoff = 0.99f;
  resonance = 0.0f;
  mode = FILTER_MODE_LOWPASS;
  
  bufl0 = 0.0f;
  bufl1 = 0.0f;
  bufl2 = 0.0f;
  bufl3 = 0.0f;
  bufr0 = 0.0f;
  bufr1 = 0.0f;
  bufr2 = 0.0f;
  bufr3 = 0.0f;
  
  calc_feedback_amount();
}


void CResoFilter::reset()
{
  bufl0 = 0.0f;
  bufl1 = 0.0f;
  bufl2 = 0.0f;
  bufl3 = 0.0f;
  bufr0 = 0.0f;
  bufr1 = 0.0f;
  bufr2 = 0.0f;
  bufr3 = 0.0f;
  
  calc_feedback_amount();
} 
