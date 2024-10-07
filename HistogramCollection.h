#ifndef _HISTOGRAMCOLLECTION_H_
#define _HISTOGRAMCOLLECTION_H_

#include <vector>
#include "Histogram.h"

class HistogramCollection{
private:
    // collection of histograms
    std::vector<Histogram*> hists;

public:
    HistogramCollection ();
    ~HistogramCollection ();
    
    void add (Histogram* hist);
    void update (int pno, double val);
    
    void print ();
};

#endif
