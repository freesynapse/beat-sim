
#include "filter.h"


//----------------------------------------------------------------------------------------
void Filter::applyLPFilter(Array<float> *_signal)
{
    int n = _signal->size;
    static int order = sizeof(a) / sizeof(a[0]);
    float *y = _signal->data;

    // filter signal
    for (size_t i = 2; i < n; i++)
    {
        m_yhat[i] = b[0]*y[i] + b[1]*y[i-1] + b[2]*y[i-2] + \
                                a[0]*m_yhat[i-1] + a[1]*m_yhat[i-2];
    }

    // shift by two steps -- it is correct that the filter is 2 steps ahead, but we 
    // shouldn't correct that here. Rather, the location of the beat is shifted 2 steps
    // back.
    // memmove(m_yhat, m_yhat+2, sizeof(float) * m_n);

    m_timeSteps++;

}

//----------------------------------------------------------------------------------------
void Filter::computeSSF(size_t _w)
{
    assert(_w > 0);    // detects beats
    


    // '/home/iomanip/0res/0_misc/IABP_circuit_exp/ABP-peak_Zong03a.pdf'
    //
    //  z_i = sum^{i}_{k=i-w}[ delta_u_k  ]
    //      delta_u_k:  delta_y_k if delta_y_k > 0 else 0
    //      delta_y_k:  y[k] - y[k-1]
    //      w:          approximate the duration of the upstroke in the waveform
    //
    for (size_t i = _w; i < m_n; i++)
    {
        m_z[i] = 0.0f;
        for (size_t k = i - _w; k < i; k++)
        {
            float dy = m_yhat[k] - m_yhat[k-1];
            float delta_u_k = (dy > 0 ? dy : 0.0f);
            m_z[i] += delta_u_k;
        }

    }

    // update mean SSF during the last 3 seconds for determining the threshold
    // for beat onset detection
    static size_t period = (size_t)(m_signalFreq * 3.0f);
    static size_t period_start = m_n - period;

    m_meanSSF = 0.0f;
    if (m_timeSteps > period)
    {
        for (size_t i = m_n - 1; i > period_start; i--)
        {
            m_meanSSF += m_z[i];
        }
        m_meanSSF /= (float)period_start;
    }

}

//----------------------------------------------------------------------------------------
void Filter::detectBeats()
{
    // any new beats this period?
    m_beatsDetected = false;

    // threshold
    float thresh = 0.6 * m_meanSSF;

    // calculate number of timesteps corresponding to t ms
    static float t = 150.0f;
    static int search_radius_ts = (int)(t * (float)m_signalFreq) / 1000.0f;

    // clear beats vector
    m_beatOnsets = std::vector<int>();

    int n = m_n - 1;
    while (n > 0)
    {
        // 1. detect where SSF signal crosses threshold
        int thresh_cross_idx = -1;
        // for (size_t i = m_n - 1; i >= 0; i--)
        for (int i = n; i >= 0; i--)
        {
            if (m_z[i] - m_meanSSF > 0.0f)
            {
                thresh_cross_idx = (int)i;
                break;
            }
        }
        if (thresh_cross_idx <= 0)
            // the m_beatsDetected flag will be false if no beats was detected
            break;
        
        // 2. search neighbourhood for min and max SSF values
        float ssf_min =  1000.0f;
        float ssf_max = -1000.0f;
        //
        int i0 = Syn::max(0, thresh_cross_idx - search_radius_ts);
        int i1 = Syn::min(thresh_cross_idx + search_radius_ts, (int)m_n);

        //
        for (int i = i0; i < i1; i++)
        {
            if (m_z[i] < ssf_min) ssf_min = m_z[i];
            if (m_z[i] > ssf_max) ssf_max = m_z[i];
        }

        // 3. test SSF max-min difference for pulse
        if ((ssf_max - ssf_min) < m_beatDetectThresh)
            // the m_beatsDetected flag will be false if no beats was accepted
            break;

        // 4. search backwards; beat onset is defined as < 1.0% of SSF max-min difference
        float diff_1percent = 0.01f * (ssf_max - ssf_min);
        for (int i = thresh_cross_idx; i >= 0; i--)
        {
            if (m_z[i] < diff_1percent)
            {
                m_beatOnsets.push_back(i);
                n = i - 1;
                break;
            }
        }

    }
    
    //
    m_beatsDetected = (m_beatOnsets.size() > 0);

}


