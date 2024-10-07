#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#include <mutex>
#include <vector>


class Histogram {
private:
	std::vector<int> hist;
	int nbins;
	double start, end;

	std::mutex lck;

public:
    Histogram (int _nbins, double _start, double _end);
	~Histogram ();

	void update (double value);
    int size ();

	std::vector<double> get_range ();
    const std::vector<int>& get_hist ();
};

#endif
