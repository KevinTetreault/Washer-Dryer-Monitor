// StatsRun - class which performs short run statistics on data points
// author: Kevin Tetreault
// version: 1.0
// date: Nov 5, 2021

#ifndef STATSRUN_H
#define	STATSRUN_H

class StatsRun{
	private:
		int16_t min, max, avg, rsltMin, rsltMax, rsltAvg, length, cnt;
		int32_t sum;
    bool rsltsChanged;

	public:
		StatsRun();
		StatsRun(int16_t _length);
		void getResults( int16_t* min, int16_t* max, int16_t* avg);
		void init();
		void setLength(int16_t _length);
		void update(int16_t value);
    bool resultsChanged();
};

#endif	// STATSRUN_H