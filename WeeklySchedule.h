//Weekly schedule class definitions

#pragma once
#include "Route.h"
#include "PSVRP_Instance.h"
#include "stdafx.h"
#include "VisitVariation.h"

class WeeklySchedule
{
	PSVRP_Instance* Instance; //Instance object
	int numberOfRoutes; //number of routes performed during the week
	int numberOfVessels; //number of vessels used during the week
	vector < Route > Routes;
	vector < SupVes > SchedVessels;
	vector < OffshoreInst > SchedInstals;
	vector < VisitVariation > VisVariations;
	int departuresPerDay;

public:
	WeeklySchedule(void); 
	WeeklySchedule(PSVRP_Instance* Inst); //construct initial solution here

	void setDepPerDay(int num) { departuresPerDay  = num;}
	int getDepPerDay () {return departuresPerDay;}
	
	int computeLB_numRoutes ();
	double computeSchedCost();
	bool schedFeasible();

	vector <SupVes> getSchedVessels() {return SchedVessels;}

	Route * getRoute(int routeNum);
	
	static const int NUMBER_OF_DAYS = 7;

	vector < vector < bool > > VesAvail; // vessel availability throughout the week
	vector < vector < int > > VisDayAssign; // assignment of visit days 2d-vector

	void addVessel(SupVes ves){SchedVessels.push_back(ves);}
	void addInstallation (OffshoreInst inst) {SchedInstals.push_back(inst);}
	void addVisitVariation (VisitVariation visVar) {VisVariations.push_back(visVar);}
	//A procedure to come close to LB in terms of number of routes

	void reassignVesselsToRoutes(); //reducing number of vessels used
	void reduceNumberOfRoutes();
	void relocateVisits();
	bool assignVessels(); //initial vessel assignment

	bool isDepartureSpreadEven(Route* routeFrom, Route* routeTo, Visit *visit);
	void updateVisDayComb(Route *routeFrom, Route *routeTo, Visit *visit);
	
	void printWeeklySchedule();
	void saveSchedule (WeeklySchedule &sched);
	void writeScheduleToFile();

	double computeTotalSlack();
	void minimizeTotalSlack();
	void reduceRouteDurTotDays();

};