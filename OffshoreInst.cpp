#include "stdafx.h"
#include "OffshoreInst.h"

OffshoreInst::OffshoreInst(void)
{
}

OffshoreInst::OffshoreInst(string theName, int OpenTime, int CloseTime, double WeekDem, 
		int VisFreq, double LayDur, int ClustSize) {
	WeeklyDemand = WeekDem;
	VisitFreq = VisFreq;
	LayTime = LayDur;
	ClusterSize = ClustSize;
	OpeningTime = OpenTime;
	ClosingTime = CloseTime;
	instName = theName;
}

		