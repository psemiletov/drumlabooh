#include "fx-resofilter.h"

/*
Peter Semiletov, 2023
*/

/*
this code based on Martin Finke's filter tutorial,
where the filter itself based on the resonant filter 
by Paul Kellett http://www.musicdsp.org/showone.php?id=29
*/

CResoFilter::CResoFilter()
    : resonance (0.0f)
    , mode (FILTER_MODE_LOWPASS)
    , feedback_amount (0.0f)
    , cutoff (0.99f)
    , bufl0 (0.0f)
    , bufl1 (0.0f)
    , bufl2 (0.0f)
    , bufl3 (0.0f)
{
    calc_feedback_amount();
}

void CResoFilter::reset()
{
    bufl0 = 0.0f;
    bufl1 = 0.0f;
    bufl2 = 0.0f;
    bufl3 = 0.0f;
    
    calc_feedback_amount();
}

float CResoFilter::process (float sample)
{
    // 4-полюсный резонансный фильтр
    bufl0 += cutoff * (sample - bufl0 + feedback_amount * (bufl0 - bufl1));
    bufl1 += cutoff * (bufl0 - bufl1);
    bufl2 += cutoff * (bufl1 - bufl2);
    bufl3 += cutoff * (bufl2 - bufl3);
    
    switch (mode)
    {
        case FILTER_MODE_LOWPASS:
            return bufl1;  // 2-полюсный (как в оригинале)
        case FILTER_MODE_HIGHPASS:
            return sample - bufl0;
        case FILTER_MODE_BANDPASS:
            return bufl0 - bufl3;
        default:
            return sample;
    }
}