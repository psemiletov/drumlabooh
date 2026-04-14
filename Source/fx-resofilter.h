#ifndef FXRESOFILTER_H
#define FXRESOFILTER_H

/*
Peter Semiletov, 2023
*/

/*
this code based on Martin Finke's filter tutorial, https://www.martin-finke.de/articles/audio-plugins-013-filter/

where the filter itself based on the resonant filter 
by Paul Kellett http://www.musicdsp.org/showone.php?id=29
*/

#include <cstring>
#include <cmath>
#include <algorithm>

enum filter_mode
{
    FILTER_MODE_LOWPASS = 0,
    FILTER_MODE_HIGHPASS,
    FILTER_MODE_BANDPASS
};

class CResoFilter
{
public:
    float resonance;
    int mode;
    float feedback_amount;
    float cutoff;
    
    float bufl0;
    float bufl1;
    float bufl2;
    float bufl3;
    
    CResoFilter();
    
    void reset();
    float process (float sample);
    
    inline void set_cutoff (float v)
    {
        // Ограничиваем cutoff, чтобы избежать деления на ноль
        v = std::max (0.01f, std::min (0.99f, v));
        
        if (cutoff == v)
            return;
        
        cutoff = v;
        calc_feedback_amount();
    }
    
    inline void set_resonance (float v)
    {
        v = std::max (0.0f, std::min (0.99f, v));
        
        if (resonance == v)
            return;
        
        resonance = v;
        calc_feedback_amount();
    }
    
    inline void calc_feedback_amount()
    {
        float divisor = 1.0f - cutoff;
        if (divisor < 0.0001f)
            divisor = 0.0001f;
        
        feedback_amount = resonance + resonance / divisor;
        
        // Ограничиваем, чтобы избежать взрыва фильтра
        feedback_amount = std::min (feedback_amount, 1.5f);
    }
};

#endif // FXRESOFILTER_H