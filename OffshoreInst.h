//Offshore installation class

#pragma once

#include "stdafx.h"

class OffshoreInst
{
private:
	string instName;
	double WeeklyDemand;
	int VisitFreq;
	double LayTime;
	int ClusterSize;
	int OpeningTime;
	int ClosingTime;
	int seqNumb;
	int numVisDayComb;

public:
	OffshoreInst(void);
	OffshoreInst (string theName, int OpenTime, int CloseTime, double WeekDem, 
		int VisFreq, double LayDur, int ClustSize);

	double getWeeklyDemand() const {return WeeklyDemand;}
	int getVisitFreq() const {return VisitFreq;}
	double getLayTime() const {return LayTime;}
	int getClusterSize() const {return ClusterSize;}
	int getOpeningTime() const {return OpeningTime;}
	int getClosingTime() const {return ClosingTime;}
	string getInstName() const {return instName;}
	int getSeqNumb() const {return seqNumb;}
	int getNumVisDayComb() const {return numVisDayComb;}

	//vectors made public for easier access
	vector < vector <int> > VisitDayCombs; 
	vector <double> Dist; //a vector of distances to other installations
	vector <int> CurVisitDayComb;

	void setSeqNumb(int num)
	{
	if (num >= 0)
		seqNumb = num;
	else
		cout << "Err SeqNumb" << endl;
	}

	void setNumVisDayComb(int numb) 
	{
		if (numb >= 0)
			numVisDayComb = numb;
		else
			cout << "Err numVisDayComb" << endl;
	}
			
};
