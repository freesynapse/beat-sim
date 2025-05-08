#ifndef __FILTER_H
#define __FILTER_H

#include "types.h"
#include <synapse/Debug>

class Filter
{
public:
    //
    Filter() {}
    //
    Filter(size_t _signal_len, size_t _signal_freq=50) :
        m_n(_signal_len), m_signalFreq(_signal_freq)
    {
        m_yhat  = init_array_<float>();
        m_z     = init_array_<float>();

        m_tsSec = 1.0f / (float)_signal_freq;
    }
    //
    ~Filter()
    {
        delete[] m_yhat;
        delete[] m_z;
    }

    // Applies the filter to a signal
    void applyLPFilter(Array<float> *_signal);
    
    // computes the slope sum function -- _w is the window length
    void computeSSF(size_t _w);

    // determine beat onset
    void detectBeats();

    // Accessors
    float *y()                      { return m_yhat;                        }
    float *z()                      { return m_z;                           }
    std::vector<int> &beatOnsets()  { return m_beatOnsets;                         }
    bool beatsDetected()            { return m_beatsDetected;               }
    float meanSSF()                 { return m_meanSSF;                     }
    size_t n()                      { return m_n - 2;                       }
    size_t timeSteps()              { return m_timeSteps;                   }
    float timeSec()                 { return (float)m_timeSteps * m_tsSec;  }

private:
    template<typename T>
    T *init_array_()
    {
        T *a = new T[m_n];
        for (size_t i = 0; i < m_n; i++)
            a[i] = 0;
        return a;
    }
private:
    size_t m_n = 0;                     // buffer size
    size_t m_signalFreq = 0;            // frequency of signal
    float *m_yhat = NULL;               // fitlered signal
    float *m_z = NULL;                  // SSF output
    float m_meanSSF = 0.0f;             // mean of the SSF the last 3 seconds
    std::vector<int> m_beatOnsets;      // beat onsets: indices into m_z
    float m_beatDetectThresh = 10.0f;
    bool m_beatsDetected;               // flag set if any beats are detected

    size_t m_timeSteps = 0;             // increases every time applyLPFilter() is called
    float m_tsSec = 0.0f;               // number of seconds per time step

    // derived using scipy.signal in ~/0res/0_misc/IABP_circuit_exp/Filter.py
    float b[3] = { 0.01364439f,  0.02728878f, 0.01364439f };
    float a[2] = { 1.64345381f, -0.69803137f };

};



#endif // __FILTER_H

