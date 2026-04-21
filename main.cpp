// PSVRP_Heuristic.cpp : main project file.

#include "stdafx.h"
//#include "OffshoreInst.h"
//#include "Route.h"
#include "WeeklySchedule.h"
#include "PSVRP_Instance.h"
#include "Timer.h"


int main(array<System::String ^> ^args)
{
	cout << "Hello" << endl;

	//Read and display supply vessel data
	int i;
	string SupBaseName("FMO");
	string NumNodes("10TW");

	PSVRP_Instance instance;
	bool isRead = instance.ReadInstance(SupBaseName, NumNodes);
	//instance.PrintInstance();

	double schedCost;
	Timer timer;

	const char* fileName;
	fileName = "C:/Aliaksandr Shyshou/Supply Vessels 2008-2009/Heuristics for PSVRP/MongstadInst_PSVRP_Heuristic/output.txt";
	ofstream solFile(fileName, ios::app);
	
	cout << "Instance read" << endl;
	cout << "maxInst = " << instance.getMaxInst() << endl;

	double bestSchedCost = numeric_limits<double>::max();
	double costBefore;
	bool improvement;
	//instance.PrintInstance();

	timer.startTimer();

	

	for ( i = 0; i < 100; i++ )
	{
		improvement = true;
		WeeklySchedule Sched(& instance);

		while (improvement)
		{
			costBefore = Sched.computeSchedCost();

			Sched.minimizeTotalSlack();
			Sched.reduceNumberOfRoutes();
			Sched.reassignVesselsToRoutes();

			Sched.reduceRouteDurTotDays();
			Sched.reassignVesselsToRoutes();

			Sched.relocateVisits();
			Sched.reassignVesselsToRoutes();

			if (Sched.computeSchedCost() + 0.0001 < costBefore)
				improvement = true;
			else
				improvement = false;

		}

		schedCost = Sched.computeSchedCost();

		if (i%50 == 0)
			cout << "ITERATION " << i << endl;

		//cout << "Schedule cost at iteration " << i <<" = " << schedCost << " NOK" << endl;
		if (schedCost < bestSchedCost)
		{
			//Sched.saveSchedule(bestSched);
			bestSchedCost = schedCost;
			cout << "Best cost = " << bestSchedCost << endl;
			cout << "At iteration " << i << endl;
			Sched.printWeeklySchedule();

			solFile << "At iteration " << i << endl;
			solFile << "Schedule cost = " << bestSchedCost << endl << endl;
			Sched.writeScheduleToFile();
			solFile << endl;
			//break;
		}
	
		//Sched.printWeeklySchedule();
	}

	cout << "The best cost = " << bestSchedCost << endl;
	cout << "Time = " << timer.getElapsedSeconds() << endl;
	//cout << "The best cost = " << bestSched.computeSchedCost() << endl;
	solFile << "Time = " << timer.getElapsedSeconds() << endl;
	
	string _str;
	cin >> _str; 
	
	return 0;
}
