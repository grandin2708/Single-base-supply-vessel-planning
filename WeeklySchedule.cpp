#include "StdAfx.h"
#include "WeeklySchedule.h"
#include "PSVRP_Instance.h"
#include "Route.h"
#include "Visit.h"

WeeklySchedule::WeeklySchedule(void)
{
}


WeeklySchedule::WeeklySchedule(PSVRP_Instance* Inst)
{
	Instance = Inst;
	//Randomly assign visit day combinations
	int i, j, k, l, m;
	vector < OffshoreInst > Platfs = Instance->getPlatforms();
	vector < OffshoreInst >::iterator it;

	vector < SupVes > Ships = Instance->getVessels();
	vector < SupVes >::iterator iter;

	bool routesShortEnough;
	vector <int> routesPerDay;

	for (iter = Ships.begin(); iter != Ships.end(); ++iter)
		addVessel(*iter);

	for (it = Platfs.begin(); it != Platfs.end(); ++it)
		addInstallation(*it);

	//Set maximum number of departures per day
	setDepPerDay(3);
	
	vector <SupVes >::iterator vesPointIter;

	vector <int> visComb;
	vector <int> numInstDay;
	int maxNumInstDay;
	int numTries = 0;
	double routeDur;
	int middle;

	bool isSchedFeasible = false;

	//START INITIALIZING THE SCHEDUlE
	while (!isSchedFeasible)
	{
		numTries++;
		//reinitialize all vectors
		VisDayAssign.clear();
		numInstDay.clear();

		VisDayAssign.resize(NUMBER_OF_DAYS); //# of rows equals # of days + 1
		numInstDay.resize(NUMBER_OF_DAYS); //# of rows equals # of days + 1
		routesPerDay.resize(NUMBER_OF_DAYS);

		maxNumInstDay = 0;

		for (it = SchedInstals.begin(); it != SchedInstals.end(); ++it)
		{
			//Skip the base
			if (it->getSeqNumb() > 0)
			{
				visComb.clear();
				visComb.resize(it->getVisitFreq());
				it->CurVisitDayComb.resize(it->getVisitFreq());
				
				//Randomly choose visit day combination
				int idx = rand() % (it->getNumVisDayComb());
				//cout << "idx = " << idx << endl;

				copy(it->VisitDayCombs[idx].begin(), it->VisitDayCombs[idx].end(),
				visComb.begin());
				
				it->CurVisitDayComb = visComb;

				for (i = 0; i < it->getVisitFreq(); i++)
					VisDayAssign[visComb[i]].push_back(it->getSeqNumb());
			}
		}

		
		for (j = 0; j < NUMBER_OF_DAYS; j++)
		{
			if (VisDayAssign[j].size() > maxNumInstDay)
				maxNumInstDay = VisDayAssign[j].size();

			routesPerDay[j] = ceil( (double)VisDayAssign[j].size()/ (Inst->getMaxInst()) );
		}

		if (maxNumInstDay > Inst->getMaxInst()*getDepPerDay())
		{
			isSchedFeasible = false;
			//cout << "Too many installations per day!!" << endl;
			continue; // we are continuing in outer loop -> ok
		}


		j = 0;
		
		Routes.clear();
		routesShortEnough = true;
		//Initialize route objects for each day
		for (i = 1; i < NUMBER_OF_DAYS; i++)
		{
			middle = ceil( (double)VisDayAssign[i].size()/routesPerDay[i]);
			for (l = 0; l < routesPerDay[i]; l++)
			{
				//Initialize Route objects
				Route route(j, i*24 - 8);
				j++;

				//start at the base
				route.addRouteInstal( &SchedInstals[0] );
				route.addVisit( Visit( &SchedInstals[0] ));

				m = 0;
				//Populate route installations vector
				for (k = l*middle; k < VisDayAssign[i].size(); k++)
				{
					if (m == middle)
						break;

					route.addRouteInstal( &SchedInstals[VisDayAssign[i][k]] );
					route.addVisit( Visit(&SchedInstals[VisDayAssign[i][k]]) );
					m++;
					

					//cout << Platfs[VisDayAssign[i][k]].getInstName() << " ";
				}
				//cout << endl;

				//end route at the base
				route.addRouteInstal( &SchedInstals[0] );
				route.addVisit( Visit( &SchedInstals[0] ));
			
				//Set route parameters
				route.setRouteAcceptanceTime(Inst->getAcceptTime());
				route.setRouteMinSlack(Inst->getMinSlack());
				route.setRouteLoadFactor(Inst->getLoadFactor());
				route.setMaxVisits(Inst->getMaxInst());
				route.setMinVisits(Inst->getMinInst());	

				route.updateVisitVector();
				//cout << "Route duration = " << route.computeRouteDuration() << endl;
				//route.printRoute();
				
				//route.cheapInsertSeq();
				//route.synchrRouteInstalVisits();
				//route.updateVisitVector();
				route.intelligentReorder();
				routeDur = route.computeRouteDuration();

				if ( (routeDur + route.getRouteAcceptanceTime() >
					route.MAX_ROUTE_DUR) || (routeDur + route.getRouteAcceptanceTime() 
					< route.MIN_ROUTE_DUR) )
				{
					routesShortEnough = false;
					isSchedFeasible = false;
					//cout << "Route too short/long!!!" << endl;
					break;
				}
				//cout << "After intelligentReorder"<< endl;
				//cout << "Route duration = " << route.computeRouteDuration() << endl;
				//route.printRoute();
			
				Routes.push_back(route);

			}// end for l < routesPerDay[i]
				
		}// end for i < NUMBER_OF_DAYS

		if (!routesShortEnough) //one of the routes is too long/too short
			continue; //try again

		//Assign vessels to routes
		VesAvail.resize(SchedVessels.size());
		int planHorizon = NUMBER_OF_DAYS + (int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);

		//Make all vessels available
		for (k = 0; k < VesAvail.size(); k++)
		{
			VesAvail[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				VesAvail[k][i] = true;
		}

		//Clear vessel data
		for (vesPointIter = SchedVessels.begin(); vesPointIter != SchedVessels.end();
					++vesPointIter)
		{
			vesPointIter->setIsVesselUsed(false);
			vesPointIter->clearVesselRoutes();
		}

		vector <Route>::iterator routeIter;
		bool vesAssigned = false;
		int routeEndIdx, routeStartIdx;
		bool endOfWeekOverlap = false;
		double rouDur;


		for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
		{
			routeStartIdx = (int) ceil(routeIter->getRouteStartTime()/24);
			routeEndIdx = (int) ceil((routeIter->getRouteEndTime() - 
							routeIter->getInstAtPos(0)->getLayTime() +
							routeIter->getRouteMinSlack() )/24);
			rouDur = routeIter->getRouteEndTime() - routeIter->getRouteStartTime();
			while (!vesAssigned) 
			{		
				//Take a vessel. See if it fits by demand and is available
				for (vesPointIter = SchedVessels.begin(); vesPointIter != SchedVessels.end();
					++vesPointIter)
				{
					//Additional check to see if the route overlaps 
					//with the first route next week
					if (routeEndIdx >= (NUMBER_OF_DAYS+1))
						for (i = 0; i <= routeEndIdx % (NUMBER_OF_DAYS+1); i++)
							if(!VesAvail[vesPointIter->getID()][i+1])
							{
								endOfWeekOverlap = true;
								break;
							}

					if ((vesPointIter->getCapacity() >= routeIter->computeRouteDemand()) &&
						(VesAvail[vesPointIter->getID()][(int) ceil(routeIter->getRouteStartTime()/24)] ) 
						&& (!endOfWeekOverlap) && 
						(rouDur < routeIter->MAX_ROUTE_DUR + routeIter->getRouteAcceptanceTime()) )
					{
							//Assign vessel to the route
							routeIter->setRouteVessel(& (*vesPointIter) );
							vesPointIter->setIsVesselUsed(true);
							vesPointIter->addVesselRoute(&(*routeIter)); //add route to "vesselRoutes" vector

							//Update vessel availability vector
							for (i = routeStartIdx; i <= routeEndIdx; i++)
								VesAvail[vesPointIter->getID()][i] = false;

							vesAssigned = true;
							break;
					}//end if 
					endOfWeekOverlap = false;
				}//end for
				
				/*if (!vesAssigned) //no assignment was made
				{
					for (vesPointIter = SchedVessels.begin(); vesPointIter != SchedVessels.end();
					++vesPointIter)
					{
						//Additional check to see if the route overlaps 
						//with the first route next week
						if (routeEndIdx >= NUMBER_OF_DAYS)
							for (i = 0; i <= routeEndIdx % NUMBER_OF_DAYS; i++)
								if(!VesAvail[vesPointIter->getID()][i+1])
								{
									endOfWeekOverlap = true;
									break;
								}

						if ( (VesAvail[vesPointIter->getID()][(int)ceil(routeIter->getRouteStartTime()/24)])
							&& (!endOfWeekOverlap) )
						{
							//Assign vessel to the route
							routeIter->setRouteVessel(&(*vesPointIter));
							vesPointIter->setIsVesselUsed(true);
							vesPointIter->addVesselRoute(&(*routeIter)); //add route to "vesselRoutes" vector

							//Update vessel availability vector
							for (i = routeStartIdx; i <= routeEndIdx; i++)
								VesAvail[vesPointIter->getID()][i] = false;

							vesAssigned = true;
							break;
						}//end if 
						endOfWeekOverlap = false;
					}//end for
				}//end if*/

				if (!vesAssigned) //Still not assigned? Bad bad
				{
					//cout << "Cannot find available vessel!!!" << endl;
					isSchedFeasible = false;
					break; //breaking out of while(!vesAssigned)
				}
				else
					isSchedFeasible = true; //if we reach here then the schedule is feasible
			}//end while(!vesAssigned)

			if (!isSchedFeasible)
				break; //break out of "for each route" loop

			//cout << routeIter->getRouteEndTime() << endl;

			
			
			vesAssigned = false;
		}//end for (each route)

}//end while(!isSchedFeasible)
		/*	for (k = 0; k < VesAvail.size(); k++)
			{
				for (i = 0; i < VesAvail[k].size(); i++)
					cout << VesAvail[k][i] << " ";
				cout << endl;
			}
			cout << endl;*/
	//printWeeklySchedule();
//cout << "Attemts to generate initial feasible solution = " << numTries << endl;
//cout << "LB on number of routes = " << computeLB_numRoutes() << endl;
}

Route* WeeklySchedule::getRoute(int routeNum)
{
	return &Routes[routeNum];
}



int WeeklySchedule::computeLB_numRoutes ()
{
	int lb1 = 0;
	double lb2 = 0;
	double lb3 = 0;
	double maxCap = 0;

	vector <OffshoreInst>::iterator it;
	vector <SupVes>::iterator iter;

	for (iter = SchedVessels.begin(); iter != SchedVessels.end(); ++iter)
		if (iter->getCapacity() > maxCap)
			maxCap = iter->getCapacity();

	for (it = SchedInstals.begin(); it != SchedInstals.end(); ++it)
	{
		lb2 += it->getVisitFreq();
		lb3 += it->getWeeklyDemand();
		if (it->getVisitFreq() > lb1)
			lb1 = it->getVisitFreq();
	}

	//cout << "lb1 = " << lb1 << ", lb2 = " << lb2 << ", lb3 = " << lb3 << endl;
	//cout << "maxCap = " << maxCap << "MaxInst = " << Instance->getMaxInst() << endl;

	lb2 = max(lb1, (int) ceil( lb2/Instance->getMaxInst() ) );
	return max( (int)lb2, (int)ceil (lb3/maxCap) );

}


void WeeklySchedule::printWeeklySchedule()
{
	cout << "Number of routes = " << Routes.size() << endl;
	vector <Route>::iterator it;
	int i, k, l;
	vector <OffshoreInst>::iterator instIter;

	for (k = 0; k < VesAvail.size(); k++)
	{
		for (i = 0; i < VesAvail[k].size(); i++)
			cout << VesAvail[k][i] << " ";

		cout << endl;
	}

	//Print visit day combinations
	cout << "Installations: Visit day combinations" << endl;
	for (instIter = SchedInstals.begin(); instIter != SchedInstals.end(); ++instIter)
	{
		cout << instIter->getInstName() << ": ";
		for (l = 0; l < instIter->CurVisitDayComb.size(); l++)
			cout << instIter->CurVisitDayComb[l] << " ";

		cout << endl;
	}

	for (it = Routes.begin(); it != Routes.end(); ++it)
	{
		cout << "____________" << it->getRouteVes()->getName() << " id = "
			<< it->getRouteVes()->getID() << "_____________" << endl;
		cout << "Route number " << it->getRouteNum() << endl;
		it->printRoute();
	}
}


double WeeklySchedule::computeSchedCost()
{	
	vector < SupVes >::iterator vesIter;

	vector <Visit> routeVisits;
	vector <Visit>::iterator visitIter;

	vector <Route*> vesRoutes;
	vector <Route*>::iterator routeIter;
	
	double cost = 0;
	double sailTime, baseTime, instTime;
	double routeSlack;
	

	for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
	{
			
		if (vesIter->getIsVesselUsed())
		{
			//cout << "Vessel " << vesIter->getName() << " is used" << endl;
			sailTime = 0; 
			baseTime = 0;
			instTime = 0;

			vesRoutes.clear();
			vesRoutes = vesIter->getVesselRoutes();
			//cout << "Vessel = " << vesIter->getName() << endl;
			//cout << "VesRoutes size = " << vesRoutes.size() << endl;

			for (routeIter = vesRoutes.begin(); routeIter != vesRoutes.end(); ++routeIter)
			{
					routeSlack = 0;
					routeSlack = (*routeIter)->MAX_ROUTE_DUR - 
						(*routeIter)->computeRouteDuration();
					while (routeSlack > 24)
						routeSlack -= 24;

					
					routeVisits.clear();
					routeVisits = (*routeIter)->getVisitObjects();

					//cout << "Route " << (*routeIter)->getRouteNum() << endl;

					baseTime += (*routeIter)->getInstAtPos(0)->getLayTime() + 
								routeSlack;

					for (visitIter = routeVisits.begin(); visitIter != routeVisits.end(); ++visitIter)
					{//Compute Sailing and Installation times
						if (visitIter < routeVisits.end() - 1 )
						{
							//cout << visitIter->getOffshoreInst()->getInstName() << endl;
							if (visitIter != routeVisits.begin())
								instTime += (visitIter->getVisitEnd() - visitIter->getVisitStart() );
							//cout << visitIter->getOffshoreInst()->getSeqNumb() << endl;
							sailTime += visitIter->getOffshoreInst()->Dist[(visitIter+1)->getOffshoreInst()->getSeqNumb()]
								/ vesIter->getSpeed();		
						}
					}//end for visitIter
				}//end for routeIter
			

			//cout << "Vessel " << vesIter->getName() << " time distribution:" << endl;
			//cout << "Base = " << baseTime << " Installation = " << instTime <<
			//	" Sailing = " << sailTime << endl;

			cost += vesIter->getVesselCost() + 
				baseTime * (vesIter->getFCBase() ) * (vesIter->getFCCosts() ) + 
				sailTime * (vesIter->getFCSailing() ) * (vesIter->getFCCosts() ) +
				instTime * (vesIter->getFCInstallation() ) * (vesIter->getFCCosts() );

			/*
			cout << "Vessel " << vesIter->getName() << endl;
			cout << "Number of routes = " << vesRoutes.size() << endl;
			cout << "Base time = " << baseTime << endl
				<< "Inst time = " << instTime << endl
				<< "Sailing time = " << sailTime << endl<< endl;

				*/

		}//end if (isVesselUsed)

	}//end for vesIter

	return cost;
}

double WeeklySchedule::computeTotalSlack()
{
	double totSlack, routeSlack;
	vector <Route>::iterator routeIter;

	totSlack = 0;

	for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
	{
					routeSlack = routeIter->MAX_ROUTE_DUR - 
						routeIter->computeRouteDuration();
					while (routeSlack > 24)
						routeSlack -= 24;

					totSlack += routeSlack;
	}

	return totSlack;
}


bool WeeklySchedule::assignVessels()
{
	//Assign vessels to routes
		VesAvail.resize(SchedVessels.size());
		int planHorizon = NUMBER_OF_DAYS + (int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);

		vector <SupVes>::iterator vesPointIter;
		int i, k;

		//Make all vessels available
		for (k = 0; k < VesAvail.size(); k++)
		{
			VesAvail[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				VesAvail[k][i] = true;
		}

		//Clear vessel data
		for (vesPointIter = SchedVessels.begin(); vesPointIter != SchedVessels.end();
					++vesPointIter)
		{
			vesPointIter->setIsVesselUsed(false);
			vesPointIter->clearVesselRoutes();
		}

		vector <Route>::iterator routeIter;
		bool vesAssigned = false;
		int routeEndIdx, routeStartIdx;
		bool endOfWeekOverlap = false;
		double rouDur;
		bool isSchedFeasible;


		for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
		{
			routeStartIdx = (int) ceil(routeIter->getRouteStartTime()/24);
			routeEndIdx = (int) ceil( (routeIter->getRouteEndTime() - 
							routeIter->getInstAtPos(0)->getLayTime() +
							routeIter->getRouteMinSlack() )/24 );
			rouDur = routeIter->getRouteEndTime() - routeIter->getRouteStartTime();
			while (!vesAssigned) 
			{		
				//Take a vessel. See if it fits by demand and is available
				for (vesPointIter = SchedVessels.begin(); vesPointIter != SchedVessels.end();
					++vesPointIter)
				{
					//Additional check to see if the route overlaps 
					//with the first route next week
					if (routeEndIdx >= (NUMBER_OF_DAYS+1))
						for (i = 0; i <= routeEndIdx % (NUMBER_OF_DAYS+1); i++)
							if(!VesAvail[vesPointIter->getID()][i+1])
							{
								endOfWeekOverlap = true;
								break;
							}

					if ((vesPointIter->getCapacity() >= routeIter->computeRouteDemand()) &&
						(VesAvail[vesPointIter->getID()][(int) ceil(routeIter->getRouteStartTime()/24)] ) 
						&& (!endOfWeekOverlap) && 
						(rouDur < routeIter->MAX_ROUTE_DUR + routeIter->getRouteAcceptanceTime())
						&& vesPointIter->getIsVesselUsed() )
					{
							//Assign vessel to the route
							routeIter->setRouteVessel(& (*vesPointIter) );
							vesPointIter->setIsVesselUsed(true);
							vesPointIter->addVesselRoute(&(*routeIter)); //add route to "vesselRoutes" vector

							//Update vessel availability vector
							for (i = routeStartIdx; i <= routeEndIdx; i++)
								VesAvail[vesPointIter->getID()][i] = false;

							vesAssigned = true;
							break;
					}//end if 

					if ((vesPointIter->getCapacity() >= routeIter->computeRouteDemand()) &&
						(VesAvail[vesPointIter->getID()][(int) ceil(routeIter->getRouteStartTime()/24)] ) 
						&& (!endOfWeekOverlap) && 
						(rouDur < routeIter->MAX_ROUTE_DUR + routeIter->getRouteAcceptanceTime()) )
					{
							//Assign vessel to the route
							routeIter->setRouteVessel(& (*vesPointIter) );
							vesPointIter->setIsVesselUsed(true);
							vesPointIter->addVesselRoute(&(*routeIter)); //add route to "vesselRoutes" vector

							//Update vessel availability vector
							for (i = routeStartIdx; i <= routeEndIdx; i++)
								VesAvail[vesPointIter->getID()][i] = false;

							vesAssigned = true;
							break;
					}//end if 

					endOfWeekOverlap = false;
				}//end for

				if (!vesAssigned) //Still not assigned? Bad bad
				{
					cout << "Cannot find available vessel!!!" << endl;
					isSchedFeasible = false;
					break; //breaking out of while(!vesAssigned)
				}
				else
					isSchedFeasible = true; //if we reach here then the schedule is feasible
			}//end while(!vesAssigned)

			if (!isSchedFeasible)
				break; //break out of "for each route" loop

			//cout << routeIter->getRouteEndTime() << endl;

			/*
			for (k = 0; k < VesAvail.size(); k++)
			{
				for (i = 0; i < VesAvail[k].size(); i++)
					cout << VesAvail[k][i] << " ";
				cout << endl;
			}*/
			vesAssigned = false;
		}//end for (each route)

	return isSchedFeasible;
}

void WeeklySchedule::reassignVesselsToRoutes()
{
	//Assign vessels prioritizing already used ones
	//Assign them intelligently
	//We have to clear and update all the vessel data:
	//1)Route class; 2) WeeklySchedule class; 3) Visit class;


	int i, j, k;
	vector <SupVes>::iterator vesPointIter;
	vector <SupVes>::iterator vesIter;

	vector <vector <bool> > vesAvailability; //local vessel availability vector
	vesAvailability.resize(VesAvail.size());
	int planHorizon = NUMBER_OF_DAYS + (int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);
	for (k = 0; k < VesAvail.size(); k++)
		{
			vesAvailability[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				vesAvailability[k][i] = VesAvail[k][i];
		}

	bool isAssignmentFeasible;
	bool allRoutesReassigned;
	bool endOfWeekOverlap;

	vector <Route*>::iterator routeIter;
	vector <Route*> vesRoutes;

	int routeEndIdx, routeStartIdx;
	double rouDur;

	vector <bool> isRouteReassigned;
	vector <SupVes*> assignToVessel;

	for (vesPointIter = SchedVessels.begin(); vesPointIter != SchedVessels.end(); ++vesPointIter)
	{
		if (vesPointIter->getIsVesselUsed())
		{
			vesRoutes.clear();
			vesRoutes = vesPointIter->getVesselRoutes();
			isRouteReassigned.clear();
			isRouteReassigned.resize(vesRoutes.size());
			assignToVessel.resize(vesRoutes.size());

			j=0;
			allRoutesReassigned = true;

			for (routeIter = vesRoutes.begin(); routeIter != vesRoutes.end(); ++routeIter)
			{
				routeStartIdx = (int) ceil((*routeIter)->getRouteStartTime()/24);
				//cout << "RouteStartIdx = " << routeStartIdx << endl;

				routeEndIdx = (int) ceil( ( (*routeIter)->getRouteEndTime() - 
								(*routeIter)->getInstAtPos(0)->getLayTime() +
								(*routeIter)->getRouteMinSlack() )/24 );
				//cout<< "RouteEndIdx = " << routeEndIdx << endl;

				rouDur = (*routeIter)->getRouteEndTime() - (*routeIter)->getRouteStartTime();


				//Try to assign route to a different vessel
				for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
					if ( (vesIter->getID() < vesPointIter->getID()) ||
						(vesIter->getIsVesselUsed()) )
					{
						isAssignmentFeasible = true;
						endOfWeekOverlap = false;

						for (i = routeStartIdx; i <= routeEndIdx; i++)
						{
							if(!vesAvailability[vesIter->getID()][i])
							{
								isAssignmentFeasible = false;
								break;
							}

							if ( (i <= planHorizon - (NUMBER_OF_DAYS+1))
								&& (!vesAvailability[vesIter->getID()][i + NUMBER_OF_DAYS]) )
								{
									isAssignmentFeasible = false;
									break;
								}
						}

						//Additional check to see if the route overlaps 
						//with the first route next week
						if (routeEndIdx >= (NUMBER_OF_DAYS+1) )
							for (i = 0; i <= routeEndIdx % (NUMBER_OF_DAYS+1); i++)
								if(!vesAvailability[vesIter->getID()][i+1])
								{
									endOfWeekOverlap = true;
									break;
								}
						//Can we reassign the route to another vessel?
						if ((vesIter->getCapacity() >= (*routeIter)->computeRouteDemand()) && 
							(isAssignmentFeasible) &&
							(rouDur < (*routeIter)->MAX_ROUTE_DUR + (*routeIter)->getRouteAcceptanceTime())
							&& (!endOfWeekOverlap) )
						{
								//Assign vessel to the route
							isRouteReassigned[j] = true;
							assignToVessel[j] = &(*vesIter);

							//Update vessel availability vector
							for (i = routeStartIdx; i <= routeEndIdx; i++)
								vesAvailability[vesIter->getID()][i] = false;

							break; //no need to assign the same route to several vessels
						}//end if 
						else
							isRouteReassigned[j] = false;
					}// end if (vesIter != vesPointIter) and for (vesIter)
					j++;		
			}//end for (routeIter)

			//Did we relocate all the routes
			for (j = 0; j < isRouteReassigned.size(); j++)
				if (!isRouteReassigned[j])
				{
					allRoutesReassigned = false;
					break;
				}

			if (allRoutesReassigned) //Do the actual modifications
			{
				for (k = 0; k < planHorizon; k++)
					vesAvailability[vesPointIter->getID()][k] = true;

				VesAvail = vesAvailability;
				vesPointIter->setIsVesselUsed(false);

				for (i = 0; i < vesRoutes.size(); i++)
				{
					//Assign vessel to the route
					vesRoutes[i]->setRouteVessel(assignToVessel[i]);
					//add route to "vesselRoutes" vector
					assignToVessel[i]->setIsVesselUsed(true);
					assignToVessel[i]->addVesselRoute(vesRoutes[i]); 
				}
				vesPointIter->clearVesselRoutes(); 

				//cout << "Improvement in ReassignVesselsToRoutes()!!!!" << endl;
			}
			else
				vesAvailability = VesAvail;
			/*			
			for (k = 0; k < VesAvail.size(); k++)
			{
				for (i = 0; i < VesAvail[k].size(); i++)
					cout << VesAvail[k][i] << " ";
				cout << endl;
			}
			cout << endl;
			*/
		}//end if
	
	}//end for (vesPointIter)
}


void WeeklySchedule::reduceNumberOfRoutes()
{
	//Redistributing a route visits between other routes
	//Making sure they are still evenly spread

	vector <Route>::iterator routeIter;
	vector <Route>::iterator routeRelocIter;
	vector <Route>::iterator routeIntermedIter;

	vector <Visit> routeVisits;
	vector <Visit> routeRelocVisits;
	vector <Visit>::iterator visitIter;

	vector <SupVes>::iterator vesIter;

	int i = 0, j = 1, k = 0, l, m;
	int routeStartIndex, routeEndIndex;

	vector <Route> savedRoutes; //additional vector of routes
	vector <Route> intermedRoutes;
	savedRoutes.resize(Routes.size());
	intermedRoutes.resize(Routes.size());

	vector < vector <int> > VisCombs;

	int LB = computeLB_numRoutes();

	int routeDays;

	for (l = 0; l < Routes.size(); l++)
	{
		savedRoutes[l] = Routes[l];
		intermedRoutes[l] = Routes[l];
	}

	double routeFromDurBefore, routeFromDurAfter;
	double routeToDurBefore, routeToDurAfter;
	double schedCostBefore, schedCostAfter;
	double bestDeltaObj;
	int bestVarIdx;

	bool improvementFound = true;
	bool shouldReassign = false;

	vector <vector <bool> > vesAvailability; //local vessel availability vector
	vesAvailability.resize(VesAvail.size());
	int planHorizon = NUMBER_OF_DAYS + 
		(int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);

	for (k = 0; k < VesAvail.size(); k++)
		{
			vesAvailability[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				vesAvailability[k][i] = VesAvail[k][i];
		}


	//cout << "-----------Entering reduceNumberOfRoutes----------" << endl;
while (improvementFound)
{
	improvementFound = false;
	i = 0; j = 1; k = 0;

	for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
	{//Try to redistribute visits of each route: greedily first
		routeDays = (int)ceil( (routeIter->computeRouteDuration()+ routeIter->getInstAtPos(0)->getLayTime()
			 + routeIter->getRouteMinSlack() )/24);
	
		routeVisits.clear();
		routeVisits = routeIter->getVisitObjects();
		VisCombs.clear();
		VisCombs.resize(routeIter->getVisitObjects().size());

		//Save the visit day combinations of installations on the route
		for (l = 1; l < routeIter->getVisitObjects().size()-1; l++)
			for (m = 0; m < routeVisits[l].getOffshoreInst()->getVisitFreq(); m++)
				VisCombs[l].push_back(routeVisits[l].getOffshoreInst()->CurVisitDayComb[m]);


		for (visitIter = routeVisits.begin()+1; visitIter != routeVisits.end()-1; ++visitIter)
		{
			
			VisVariations.clear();
			bestDeltaObj = numeric_limits<double>::max();
			k = 0;
			

			for (routeRelocIter = Routes.begin(); routeRelocIter != Routes.end(); ++routeRelocIter)
			{
				//do not reinsert in the same route, check load feasibility
				//if the installation is already on the route and even spread
				if ( (routeIter != routeRelocIter)
				&& (routeRelocIter->computeRouteDemand() + visitIter->getOffshoreInst()->getWeeklyDemand()
				/ visitIter->getOffshoreInst()->getVisitFreq() < routeRelocIter->getRouteVes()->getCapacity())
				&& ( !routeRelocIter->isInstOnRoute( (visitIter->getOffshoreInst()) ) ) 
				&& (isDepartureSpreadEven(&(*routeIter), &(*routeRelocIter), &(*visitIter)) ) 
				&& (routeRelocIter->getVisitObjects().size() - 2 < routeRelocIter->getMaxVisits()) )
				{
					
					schedCostBefore = computeSchedCost();

					//cout << "Sched Before!!!" << endl;
					//printWeeklySchedule();

					routeToDurBefore = routeRelocIter->computeRouteDuration();
					routeRelocIter->insertInstalVisit(1, *visitIter);
					routeRelocIter->updateVisitVector();
					routeRelocIter->intelligentReorder();
					routeToDurAfter = routeRelocIter->computeRouteDuration();

					if (routeToDurAfter < routeRelocIter->MAX_ROUTE_DUR + routeRelocIter->getRouteAcceptanceTime() )
					{	
						routeFromDurBefore = routeIter->computeRouteDuration();
						routeIter->deleteInstalVisit( 1 );
						routeIter->updateVisitVector();
						routeIter->intelligentReorder();
						routeFromDurAfter = routeIter->computeRouteDuration();

						if (routeFromDurAfter > routeRelocIter->MIN_ROUTE_DUR + 
							routeRelocIter->getRouteAcceptanceTime() )
						{

							schedCostAfter = computeSchedCost();

							//cout << "Sched After!!!" << endl;
							//printWeeklySchedule();
							
							//Initialize VisitVariation objects
							VisitVariation visVar(&(*visitIter));
							visVar.setRouteFrom(&(*routeIter));
							visVar.setRouteTo(&(*routeRelocIter));
							visVar.setRouteFromDurDecrease(routeFromDurBefore - routeFromDurAfter);
							visVar.setRouteToDurIncrease(routeToDurAfter - routeToDurBefore);
							visVar.setDeltaObj(schedCostAfter - schedCostBefore);

							VisVariations.push_back(visVar);
						}

						//Restore affected routes.
						for (routeIntermedIter = intermedRoutes.begin(); 
							routeIntermedIter->getRouteNum() !=  routeIter->getRouteNum();
							++routeIntermedIter)
							;
						*routeIter = *routeIntermedIter;
					}
					
					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  routeRelocIter->getRouteNum();
						++routeIntermedIter)
						;
					*routeRelocIter = *routeIntermedIter;
					
				}//end if
					
				k++;
			}//end for routeRelocIter
			
			//No feasible insertion found - go on to the next routeIter
			if (VisVariations.size() == 0)
			{
				//cout << "No insertion found" << endl;
				break; // breaking out of for (each visitIter)
			}
			else // Identify best relocation (in terms of obj. function)
			{
				for (l = 0; l < VisVariations.size(); l++)
					if (VisVariations[l].getDeltaObj() < bestDeltaObj)
					{
						bestDeltaObj = VisVariations[l].getDeltaObj();
						bestVarIdx = l;
						//cout << "Best DeltaObj = " << bestDeltaObj << endl;
					}
			
				/*cout << "Relocating Installation " << VisVariations[bestVarIdx].getVarVisit()->getOffshoreInst()->getInstName() << endl;
				cout << "Best insertion from route " << VisVariations[bestVarIdx].getRouteFrom()->getRouteStartTime() 
					<< " to route " << VisVariations[bestVarIdx].getRouteTo()->getRouteStartTime() << endl;

				cout << "DeltaObj = " << VisVariations[bestVarIdx].getDeltaObj() << endl;
				cout << "RouteFromDurDecrease = " << VisVariations[bestVarIdx].getRouteFromDurDecrease() << endl;
				cout << "RouteToDurIncrease = " << VisVariations[bestVarIdx].getRouteToDurIncrease() << endl;

				cout << "Printing the routes BEFORE insertions" << endl << endl;

				VisVariations[bestVarIdx].getRouteFrom()->printRoute();
				VisVariations[bestVarIdx].getRouteTo()->printRoute();
				*/

				//Implement best relocation
				VisVariations[bestVarIdx].getRouteTo()->insertInstalVisit(1, *visitIter);
				VisVariations[bestVarIdx].getRouteFrom()->deleteInstalVisit( 1 );

				VisVariations[bestVarIdx].getRouteTo()->updateVisitVector();
				VisVariations[bestVarIdx].getRouteFrom()->updateVisitVector();

				VisVariations[bestVarIdx].getRouteTo()->intelligentReorder();
				//VisVariations[bestVarIdx].getRouteFrom()->intelligentReorder();

				//Update visit day combination
				updateVisDayComb(VisVariations[bestVarIdx].getRouteFrom(),
				VisVariations[bestVarIdx].getRouteTo(), &(*visitIter) );

				/*cout << endl;

				cout << "New visit day combination for the installation " << visitIter->getOffshoreInst()->getInstName()
					<< " is ";
				for (l = 0; l < visitIter->getOffshoreInst()->CurVisitDayComb.size(); l++)
					cout << visitIter->getOffshoreInst()->CurVisitDayComb[l] << " ";

				cout << endl;

				cout << "Printing the routes after insertions" << endl << endl;

				
				VisVariations[bestVarIdx].getRouteFrom()->printRoute();
				VisVariations[bestVarIdx].getRouteTo()->printRoute();
				*/

				//Update Intermediate route vector
				for (routeIntermedIter = intermedRoutes.begin(); 
					routeIntermedIter->getRouteNum() !=  
					VisVariations[bestVarIdx].getRouteFrom()->getRouteNum();
					++routeIntermedIter)
						;
				*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteFrom());

				for (routeIntermedIter = intermedRoutes.begin(); 
					routeIntermedIter->getRouteNum() !=  
					VisVariations[bestVarIdx].getRouteTo()->getRouteNum();
					++routeIntermedIter)
					;
				*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteTo());
			}
			j++;
		}//end for visitIter

		//If all visits that were relocated successfully
		//Update relevant data structures
		if (routeIter->getVisitObjects().size() == 2) //only base at the beginning and the end
		{
			cout << "In REDUCE_NUM_ROUTES erasing route number " << 
			routeIter->getRouteNum() << endl; 

			routeStartIndex = (int) ceil(routeIter->getRouteStartTime()/24);
			routeEndIndex = routeStartIndex + routeDays - 1;
			

			for (l = routeStartIndex; l <= routeEndIndex; l++)
				VesAvail[routeIntermedIter->getRouteVes()->getID()][l]=true;

			//routeIntermedIter->getRouteVes()->eraseVesselRoute(routeIntermedIter->getRouteNum());
			intermedRoutes.erase(intermedRoutes.begin()+i);
			Routes.erase(Routes.begin()+i);
			savedRoutes.erase(savedRoutes.begin()+i);

			//For each vessel clear and update vesselRoutes vector
			for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
					{
						if (vesIter->getVesselRoutes().size() > 0)
						{
							vesIter->clearVesselRoutes();
							for (routeIter = Routes.begin(); 
								routeIter != Routes.end(); ++routeIter)
								if (vesIter->getID() == routeIter->getRouteVes()->getID() )
									vesIter->addVesselRoute(&(*routeIter));
						}
						else
							vesIter->setIsVesselUsed(false);
					}//end for

			improvementFound = true;
			break;
				
		}//end if (routeIter->getVisitObjects().size() == 2)
		else 
		{
			//Restore the Routes vector
			for (l = 0; l < Routes.size(); l++)
			{
				Routes[l] = savedRoutes[l];
				intermedRoutes[l] = savedRoutes[l];
			}

			//Restore visit day combinations
			for (l = 1; l < routeIter->getVisitObjects().size()-1; l++)
			{
				routeVisits[l].getOffshoreInst()->CurVisitDayComb.clear();
				for (m = 0; m < routeVisits[l].getOffshoreInst()->getVisitFreq(); m++)
					routeVisits[l].getOffshoreInst()->CurVisitDayComb.push_back(VisCombs[l][m]);
			}
		}
		
		i++;
	}//end for routeIter

	//Stop if we reached LB in terms of number of routes
	if ( Routes.size() == LB )
		break; //break out of while loop

}//end while (improvementFound) 
		
//cout << "After reducing the number of routes" << endl;
//printWeeklySchedule();

}

void WeeklySchedule::reduceRouteDurTotDays()
{
	//Redistributing a route visits between other routes
	//Making sure they are still evenly spread

	
	vector <Route>::iterator routeIter;
	vector <Route>::iterator routeRelocIter;
	vector <Route>::iterator routeIntermedIter;

	vector <Visit> routeVisits;
	vector <Visit> routeRelocVisits;
	vector <Visit>::iterator visitIter;

	vector <SupVes>::iterator vesIter;

	int i = 0, j = 1, k = 0, l, m;
	int routeStartIndex, routeEndIndex;

	vector <Route> savedRoutes; //additional vector of routes
	vector <Route> intermedRoutes;
	savedRoutes.resize(Routes.size());
	intermedRoutes.resize(Routes.size());

	vector < vector <int> > VisCombs;

	int LB = computeLB_numRoutes();
	int totDays;
	int routeDays;

	for (l = 0; l < Routes.size(); l++)
	{
		savedRoutes[l] = Routes[l];
		intermedRoutes[l] = Routes[l];
		totDays += (int)ceil( (Routes[l].computeRouteDuration()+
			Routes[l].getInstAtPos(0)->getLayTime() + Routes[l].getRouteMinSlack())/24);
	}
	
	//cout << "BEFORE reduceRouteDurTotDays() Total route days = " << totDays << endl;

	double routeFromDurBefore, routeFromDurAfter;
	double routeToDurBefore, routeToDurAfter;
	double schedCostBefore, schedCostAfter;
	double bestDeltaObj;
	int bestVarIdx;

	bool improvementFound = true;
	bool shouldReassign = false;

	vector <vector <bool> > vesAvailability; //local vessel availability vector
	vesAvailability.resize(VesAvail.size());
	int planHorizon = NUMBER_OF_DAYS + 
		(int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);

	for (k = 0; k < VesAvail.size(); k++)
		{
			vesAvailability[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				vesAvailability[k][i] = VesAvail[k][i];
		}


	//cout << "-----------Entering reduceNumberOfRoutes----------" << endl;
while (improvementFound)
{
	improvementFound = false;
	i = 0;  k = 0;

	for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
		if ( (int)ceil( (routeIter->computeRouteDuration()+ routeIter->getInstAtPos(0)->getLayTime())/24) == 
			(int) ceil( ((double)routeIter->MAX_ROUTE_DUR + routeIter->getInstAtPos(0)->getLayTime())/24) ) //Only consider 3-day routes
	{//Try to redistribute visits of each route: greedily first

		routeDays = (int)ceil( (routeIter->computeRouteDuration()+ routeIter->getInstAtPos(0)->getLayTime()
			 + routeIter->getRouteMinSlack() )/24);
	
		routeVisits.clear();
		routeVisits = routeIter->getVisitObjects();
		VisCombs.clear();
		VisCombs.resize(routeIter->getVisitObjects().size());

		//Save the visit day combinations of installations on the route
		for (l = 1; l < routeIter->getVisitObjects().size()-1; l++)
			for (m = 0; m < routeVisits[l].getOffshoreInst()->getVisitFreq(); m++)
				VisCombs[l].push_back(routeVisits[l].getOffshoreInst()->CurVisitDayComb[m]);

		j = 1;

		for (visitIter = routeVisits.begin()+1; visitIter != routeVisits.end()-1; ++visitIter)
		{
			
			VisVariations.clear();
			bestDeltaObj = numeric_limits<double>::max();
			k = 0;
			

			for (routeRelocIter = Routes.begin(); routeRelocIter != Routes.end(); ++routeRelocIter)
			{
				//do not reinsert in the same route, check load feasibility
				//if the installation is already on the route and evenly spread
				if ( (routeIter != routeRelocIter)
				&& (routeRelocIter->computeRouteDemand() + visitIter->getOffshoreInst()->getWeeklyDemand()
				/ visitIter->getOffshoreInst()->getVisitFreq() < routeRelocIter->getRouteVes()->getCapacity())
				&& ( !routeRelocIter->isInstOnRoute( (visitIter->getOffshoreInst()) ) ) 
				&& (isDepartureSpreadEven(&(*routeIter), &(*routeRelocIter), &(*visitIter)) ) 
				&& (routeRelocIter->getVisitObjects().size() - 2 < routeRelocIter->getMaxVisits()) )
				{
					
					schedCostBefore = computeSchedCost();

					//cout << "Sched Before!!!" << endl;
					//printWeeklySchedule();

					routeToDurBefore = routeRelocIter->computeRouteDuration();
					routeRelocIter->insertInstalVisit(1, *visitIter);
					routeRelocIter->updateVisitVector();
					routeRelocIter->intelligentReorder();
					routeToDurAfter = routeRelocIter->computeRouteDuration();

					if ( (routeToDurAfter + routeRelocIter->getRouteMinSlack() < routeRelocIter->MAX_ROUTE_DUR + routeRelocIter->getRouteAcceptanceTime() )
						&& ( ceil( (routeToDurBefore + routeRelocIter->getRouteMinSlack() +
						routeRelocIter->getInstAtPos(0)->getLayTime())/24 ) 
							>= ceil( (routeToDurAfter + routeRelocIter->getRouteMinSlack() +
							routeRelocIter->getInstAtPos(0)->getLayTime())/24 ) ) )
					{	
						routeFromDurBefore = routeIter->computeRouteDuration();
						routeIter->deleteInstalVisit( j );
						routeIter->updateVisitVector();
						routeIter->intelligentReorder();
						routeFromDurAfter = routeIter->computeRouteDuration();

						if (routeFromDurAfter > routeIter->MIN_ROUTE_DUR + routeIter->getRouteAcceptanceTime())
						{

							schedCostAfter = computeSchedCost();

							//cout << "Sched After!!!" << endl;
							//printWeeklySchedule();
							
							//Initialize VisitVariation objects
							VisitVariation visVar(&(*visitIter));
							visVar.setRouteFrom(&(*routeIter));
							visVar.setRouteTo(&(*routeRelocIter));
							visVar.setRouteFromDurDecrease(routeFromDurBefore - routeFromDurAfter);
							visVar.setRouteToDurIncrease(routeToDurAfter - routeToDurBefore);
							visVar.setDeltaObj(schedCostAfter - schedCostBefore);

							VisVariations.push_back(visVar);
						}
							//Restore affected routes.
							for (routeIntermedIter = intermedRoutes.begin(); 
								routeIntermedIter->getRouteNum() !=  routeIter->getRouteNum();
								++routeIntermedIter)
								;
							*routeIter = *routeIntermedIter;

							//routeIter->setVisitIds();
								
					}
					
					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  routeRelocIter->getRouteNum();
						++routeIntermedIter)
						;
					*routeRelocIter = *routeIntermedIter;

					//routeRelocIter->setVisitIds();
					
				}//end if
					
				k++;
			}//end for routeRelocIter
			
			//No feasible insertion found - go on to the next routeIter
			if (VisVariations.size() == 0)
			{
				//cout << "No insertion found" << endl;
				j++;
				continue; // continue to next visit object
			}
			else // Identify best relocation (in terms of obj. function)
			{
				for (l = 0; l < VisVariations.size(); l++)
					if (VisVariations[l].getDeltaObj() < bestDeltaObj)
					{
						bestDeltaObj = VisVariations[l].getDeltaObj();
						bestVarIdx = l;
						//cout << "Best DeltaObj = " << bestDeltaObj << endl;
					}
			
				/*cout << "Relocating Installation " << VisVariations[bestVarIdx].getVarVisit()->getOffshoreInst()->getInstName() << endl;
				cout << "Best insertion from route " << VisVariations[bestVarIdx].getRouteFrom()->getRouteStartTime() 
					<< " to route " << VisVariations[bestVarIdx].getRouteTo()->getRouteStartTime() << endl;

				cout << "DeltaObj = " << VisVariations[bestVarIdx].getDeltaObj() << endl;
				cout << "RouteFromDurDecrease = " << VisVariations[bestVarIdx].getRouteFromDurDecrease() << endl;
				cout << "RouteToDurIncrease = " << VisVariations[bestVarIdx].getRouteToDurIncrease() << endl;

				cout << "Printing the routes BEFORE insertions" << endl << endl;

				VisVariations[bestVarIdx].getRouteFrom()->printRoute();
				VisVariations[bestVarIdx].getRouteTo()->printRoute();
				*/

				//Implement best relocation
				VisVariations[bestVarIdx].getRouteTo()->insertInstalVisit(1, *visitIter);
				VisVariations[bestVarIdx].getRouteFrom()->deleteInstalVisit( j );

				VisVariations[bestVarIdx].getRouteTo()->updateVisitVector();
				VisVariations[bestVarIdx].getRouteFrom()->updateVisitVector();

				VisVariations[bestVarIdx].getRouteTo()->intelligentReorder();
				//VisVariations[bestVarIdx].getRouteFrom()->intelligentReorder();

				//Update visit day combination
				updateVisDayComb(VisVariations[bestVarIdx].getRouteFrom(),
				VisVariations[bestVarIdx].getRouteTo(), &(*visitIter) );

				VisVariations[bestVarIdx].getRouteTo()->setVisitIds();
				VisVariations[bestVarIdx].getRouteFrom()->setVisitIds();

				//j++;

				/*cout << endl;

				cout << "New visit day combination for the installation " << visitIter->getOffshoreInst()->getInstName()
					<< " is ";
				for (l = 0; l < visitIter->getOffshoreInst()->CurVisitDayComb.size(); l++)
					cout << visitIter->getOffshoreInst()->CurVisitDayComb[l] << " ";

				cout << endl;

				cout << "Printing the routes after insertions" << endl << endl;

				
				VisVariations[bestVarIdx].getRouteFrom()->printRoute();
				VisVariations[bestVarIdx].getRouteTo()->printRoute();
				*/

				//Update Intermediate route vector
				for (routeIntermedIter = intermedRoutes.begin(); 
					routeIntermedIter->getRouteNum() !=  
					VisVariations[bestVarIdx].getRouteFrom()->getRouteNum();
					++routeIntermedIter)
						;
				*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteFrom());

				for (routeIntermedIter = intermedRoutes.begin(); 
					routeIntermedIter->getRouteNum() !=  
					VisVariations[bestVarIdx].getRouteTo()->getRouteNum();
					++routeIntermedIter)
					;
				*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteTo());
			}
		}//end for visitIter

		//If all visits that were relocated successfully
		//Update relevant data structures
		if (routeIter->getVisitObjects().size() == 2) //only base at the beginning and the end
		{
			cout << "In ReduceRouteDurTotDays erasing route number " << 
			routeIter->getRouteNum() << endl; 

			//Update the solution - THIS IS WRONG!!!
			routeStartIndex = (int) ceil(routeIter->getRouteStartTime()/24);

			routeEndIndex = routeStartIndex + routeDays - 1;
			

			for (l = routeStartIndex; l <= routeEndIndex; l++)
				VesAvail[routeIter->getRouteVes()->getID()][l]=true;

			//routeIntermedIter->getRouteVes()->eraseVesselRoute(routeIntermedIter->getRouteNum());
			intermedRoutes.erase(intermedRoutes.begin()+i);
			Routes.erase(Routes.begin()+i);
			savedRoutes.erase(savedRoutes.begin()+i);

			//For each vessel clear and update vesselRoutes vector
			for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
					{
						if (vesIter->getVesselRoutes().size() > 0)
						{
							vesIter->clearVesselRoutes();
							for (routeIter = Routes.begin(); 
								routeIter != Routes.end(); ++routeIter)
								if (vesIter->getID() == routeIter->getRouteVes()->getID() )
									vesIter->addVesselRoute(&(*routeIter));
						}
						else
							vesIter->setIsVesselUsed(false);
					}//end for

			improvementFound = true;
			break;
				
		}//end if (routeIter->getVisitObjects().size() == 2)
		else 
		{
			routeIter->intelligentReorder();
			routeIter->updateVisitVector();

			if ( (int)ceil( (routeIter->computeRouteDuration() + routeIter->getInstAtPos(0)->getLayTime()
				+ routeIter->getRouteMinSlack() )/24) < routeDays )
			{
				/*
				cout << "Improvement in reduceRouteDurTotDays()" << endl;
				cout << "Old route days = " << routeDays << endl;
				cout << "New route days = " << (int)ceil( (routeIter->computeRouteDuration() + routeIter->getInstAtPos(0)->getLayTime()
				+ routeIter->getRouteMinSlack() )/24) << endl;
				cout << "Route number = " << routeIter->getRouteNum() << endl;
				cout << "VESSEL: "<< routeIter->getRouteVes()->getName() << endl;
				routeIter->printRoute();
				*/

				//Update the solution
				routeStartIndex = (int) ceil(routeIter->getRouteStartTime()/24);
				routeEndIndex = (int) ceil ((routeIter->getRouteEndTime() - routeIter->getInstAtPos(0)->getLayTime() 
					+ routeIter->getRouteMinSlack() )/24);
			
				VesAvail[routeIter->getRouteVes()->getID()][routeEndIndex+1]=true;
				improvementFound = true;

				for (l = 0; l < Routes.size(); l++)
					savedRoutes[l] = Routes[l];
					
		
				//break;
			}
			else 
			{
				//Restore the Routes vector
				for (l = 0; l < Routes.size(); l++)
				{
					Routes[l] = savedRoutes[l];
					intermedRoutes[l] = savedRoutes[l];
				}

				//Restore visit day combinations
				for (l = 1; l < routeIter->getVisitObjects().size()-1; l++)
				{
					routeVisits[l].getOffshoreInst()->CurVisitDayComb.clear();
					for (m = 0; m < routeVisits[l].getOffshoreInst()->getVisitFreq(); m++)
						routeVisits[l].getOffshoreInst()->CurVisitDayComb.push_back(VisCombs[l][m]);
				}
			}//end else
		}//end else
		
		i++;
	}//end for routeIter

	
	//Stop if we reached LB in terms of number of routes
	if ( Routes.size() == LB )
		break; //break out of while loop

}//end while (improvementFound) 

totDays = 0;
for (l = 0; l < Routes.size(); l++)
		totDays += (int)ceil( (Routes[l].computeRouteDuration()+
			Routes[l].getInstAtPos(0)->getLayTime() + Routes[l].getRouteMinSlack())/24);
	
	//cout << "AFTER reduceRouteDurTotDays() Total route days = " << totDays << endl <<endl;

		
//printWeeklySchedule();
}

bool WeeklySchedule::isDepartureSpreadEven(Route *routeFrom, Route *routeTo, Visit *visit)
{
	vector <int> DayCombResult;
	DayCombResult.resize(visit->getOffshoreInst()->getVisitFreq());
	DayCombResult = visit->getOffshoreInst()->CurVisitDayComb;
	int removedDay = (int) ceil(routeFrom->getRouteStartTime()/24);
	int insertedDay = (int) ceil(routeTo->getRouteStartTime()/24);

	int i, j, k;
	bool dayInserted = false;

	//Keep the assigned visit day combination sorted
	for (i = 0; i < DayCombResult.size(); i++)
	{
		if (insertedDay == DayCombResult[i])
			return false;

		//cout << DayCombResult[i] << endl;
		if(insertedDay < DayCombResult[i])
		{
			DayCombResult.insert(DayCombResult.begin()+i, insertedDay);
			dayInserted = true;
			break;
		}
	}


	if (!dayInserted)
		DayCombResult.push_back(insertedDay);

	for (i = 0; i < DayCombResult.size(); i++)
		if ( removedDay == DayCombResult[i] )
			DayCombResult.erase(DayCombResult.begin()+i);

	//Check if the visit day combination satisfy the "evenly spread" requirements

	vector <int> auxVec;
	int accumDiff = 0;
	int planHor = 0;
	bool ind1 = false, ind2 = false;
	int numVisits, day;

	if ( (DayCombResult.size() == 1) || (DayCombResult.size() == 6) )
		return true;

	else if (DayCombResult.size() == 2)
	{
		//2 visits: at least one visit during 4 days
		//First indicator
		for (i = 1; i < DayCombResult.size(); i++)
			auxVec.push_back(DayCombResult[i] - DayCombResult[i-1]);

		/*
		if ( (DayCombResult[DayCombResult.size() - 1] == NUMBER_OF_DAYS - 1) && 
			(DayCombResult[0] == 1) )
				auxVec.push_back(1);
				*/

		for (i = 0; i < auxVec.size(); i++)
			if (auxVec[i] == 1)
				accumDiff++;

		if (accumDiff == visit->getOffshoreInst()->getVisitFreq() - 1)
			ind1 = false;
		else
			ind1 = true;


		//Second indicator
		planHor = 4;
		for (i = 1; i < NUMBER_OF_DAYS+1; i++)
		{
			numVisits = 0;
			for (j = 0; j < planHor; j++)
			{
				if ( (i+j) < NUMBER_OF_DAYS + 1)
					day = i + j;
				else
					day = (i+j)%(NUMBER_OF_DAYS+1) + 1;

				for (k = 0; k < DayCombResult.size(); k++)
					if (DayCombResult[k] == day)
						numVisits++;
			}//end for j

			if (numVisits == 0)
			{
				ind2 = true;
				break;
			}
		}//end for i
		
		if ( (ind1) && (!ind2) )
			return true;
		else 
			return false;

	}

	else if (DayCombResult.size() == 3)
	{
		//3 visits: at least one visit during 3 days
		//First indicator
		for (i = 1; i < DayCombResult.size(); i++)
			auxVec.push_back(DayCombResult[i] - DayCombResult[i-1]);

		/*
		if ( (DayCombResult[DayCombResult.size() - 1] == NUMBER_OF_DAYS - 1) && 
			(DayCombResult[0] == 1) )
				auxVec.push_back(1);
				*/

		for (i = 0; i < auxVec.size(); i++)
			if (auxVec[i] == 1)
				accumDiff++;

		if (accumDiff == visit->getOffshoreInst()->getVisitFreq() - 1)
			ind1 = false;
		else
			ind1 = true;


		//Second indicator
		planHor = 3;
		for (i = 1; i < NUMBER_OF_DAYS+1; i++)
		{
			numVisits = 0;
			for (j = 0; j < planHor; j++)
			{
				if ( (i+j) < NUMBER_OF_DAYS+1)
					day = i + j;
				else
					day = (i+j) % (NUMBER_OF_DAYS+1) + 1;

				for (k = 0; k < DayCombResult.size(); k++)
					if (DayCombResult[k] == day)
						numVisits++;
			}//end for j

			if (numVisits == 0)
			{
				ind2 = true;
				break;
			}
		}//end for i
		
		if ( (ind1) && (!ind2) )
			return true;
		else 
			return false;

	}
	else if (DayCombResult.size() == 4)
	{
		//4 visits: During a 4-day period at least two vessels
		//First indicator
		for (i = 1; i < DayCombResult.size(); i++)
			auxVec.push_back(DayCombResult[i] - DayCombResult[i-1]);

		/*
		if ( (DayCombResult[DayCombResult.size() - 1] == NUMBER_OF_DAYS - 1) && 
			(DayCombResult[0] == 1) )
				auxVec.push_back(1);
				*/

		for (i = 0; i < auxVec.size(); i++)
			if (auxVec[i] == 1)
				accumDiff++;

		if (accumDiff == visit->getOffshoreInst()->getVisitFreq() - 1)
			ind1 = false;
		else
			ind1 = true;

		//Second indicator
		planHor = 4;
		for (i = 1; i < NUMBER_OF_DAYS+1; i++)
		{
			numVisits = 0;
			for (j = 0; j < planHor; j++)
			{
				if ( (i+j) < NUMBER_OF_DAYS+1)
					day = i + j;
				else
					day = (i+j) % (NUMBER_OF_DAYS+1) + 1;

				for (k = 0; k < DayCombResult.size(); k++)
					if (DayCombResult[k] == day)
						numVisits++;
			}//end for j

			if (numVisits <= 1)
			{
				ind2 = true;
				break;
			}
		}//end for i
		
		if ( (ind1) && (!ind2) )
			return true;
		else 
			return false;
	}


	else if (DayCombResult.size() == 5)
	{
		//5 visits
		for (i = 1; i < DayCombResult.size(); i++)
			auxVec.push_back(DayCombResult[i] - DayCombResult[i-1]);

		/*
		if ( (DayCombResult[DayCombResult.size() - 1] == NUMBER_OF_DAYS - 1) && 
			(DayCombResult[0] == 1) )
				auxVec.push_back(1);
				*/

		for (i = 0; i < auxVec.size(); i++)
			if (auxVec[i] == 1)
				accumDiff++;

		if (accumDiff == visit->getOffshoreInst()->getVisitFreq() - 1)
			return false;
		else
			return true;
	}//end 5 visits

	else //DayCombResult of the wrong size
		cout << "Error in isDepartureSpreadEven!!!" << endl << endl;
}

void WeeklySchedule::updateVisDayComb(Route *routeFrom, Route *routeTo, Visit *visit)
{
	vector <int> DayCombResult;
	DayCombResult.resize(visit->getOffshoreInst()->getVisitFreq());
	DayCombResult = visit->getOffshoreInst()->CurVisitDayComb;
	int removedDay = (int) ceil(routeFrom->getRouteStartTime()/24);
	int insertedDay = (int) ceil(routeTo->getRouteStartTime()/24);

	int i;
	bool dayInserted = false;

	//Keep the assigned visit day combination sorted
	for (i = 0; i < DayCombResult.size(); i++)
	{
		//cout << DayCombResult[i] << endl;
		if(insertedDay < DayCombResult[i])
		{
			DayCombResult.insert(DayCombResult.begin()+i, insertedDay);
			dayInserted = true;
			break;
		}
	}


	if (!dayInserted)
		DayCombResult.push_back(insertedDay);

	for (i = 0; i < DayCombResult.size(); i++)
		if ( removedDay == DayCombResult[i] )
			DayCombResult.erase(DayCombResult.begin()+i);

	visit->getOffshoreInst()->CurVisitDayComb.clear();

	for (i = 0; i < DayCombResult.size(); i++)
		visit->getOffshoreInst()->CurVisitDayComb.push_back(DayCombResult[i]);

}

void WeeklySchedule::saveSchedule(WeeklySchedule &sched)
{
	//clear vectors
	sched.SchedInstals.clear();
	sched.Routes.clear();
	sched.SchedVessels.clear();
	sched.VisVariations.clear();

	sched.SchedInstals.resize(SchedInstals.size());
	sched.Routes.resize(Routes.size());
	sched.SchedVessels.resize(SchedVessels.size());
	sched.VisVariations.resize(VisVariations.size());

	vector <OffshoreInst>::iterator instIter;
	vector <Route>::iterator routeIter;
	vector <SupVes>::iterator vesIter;
	vector <VisitVariation>::iterator visVarIter;

	for (instIter = SchedInstals.begin(); instIter != SchedInstals.end(); ++instIter)
		sched.SchedInstals.push_back(*instIter);

	for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
		sched.Routes.push_back(*routeIter);
	
	for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
		sched.SchedVessels.push_back(*vesIter);

	for (visVarIter = VisVariations.begin(); visVarIter != VisVariations.end(); ++visVarIter)
		sched.VisVariations.push_back(*visVarIter);

	vector <Route*> vesRoutes;
	vector <Route*>::iterator routePointIter;
	vector <Visit> Visits;
	vector <Visit>::iterator visIter;



	/*
	cout << "Saving WeeklySchedule object!!!" << endl;

	for (vesIter = sched.SchedVessels.begin(); vesIter != sched.SchedVessels.end(); ++vesIter)
	{
		vesRoutes.clear();
		vesRoutes = vesIter->getVesselRoutes();
		cout << vesIter->getName() << endl;
		for(routePointIter = vesRoutes.begin(); routePointIter != vesRoutes.end(); ++routePointIter)
		{
			cout << (*routePointIter)->getRouteStartTime() << endl;
			Visits.clear();
			Visits = (*routePointIter)->getVisitObjects();
			for (visIter = Visits.begin(); visIter != Visits.end(); ++visIter)
				cout << visIter->getOffshoreInst()->getInstName() << " ";

			cout << endl;
		}
	}*/

	cout << "Schedule cost in saveSchedule = " << sched.computeSchedCost() << endl;
			
	sched.Instance = Instance;
	sched.numberOfRoutes = numberOfRoutes;
	sched.numberOfVessels = numberOfVessels;
}

void WeeklySchedule::relocateVisits() 
{
	vector <Route>::iterator routeIter;
	vector <Route>::iterator routeRelocIter;
	vector <Route>::iterator routeIntermedIter;

	vector <Visit> routeVisits;
	vector <Visit> routeRelocVisits;
	vector <Visit>::iterator visitIter;

	vector <SupVes>::iterator vesIter;

	int i, k, l;
	int routeStartIndex, routeEndIndex;

	vector <Route> intermedRoutes; //additional vector of routes
	intermedRoutes.resize(Routes.size());

	vector < vector <int> > VisCombs;

	for (l = 0; l < Routes.size(); l++)
		intermedRoutes[l] = Routes[l];

	double routeFromDurBefore, routeFromDurAfter;
	double routeToDurBefore, routeToDurAfter;
	double schedCostBefore, schedCostAfter;
	double bestDeltaObj;
	int bestVarIdx;

	bool improvementFound = true;
	bool numRoutesReduced;

	vector <vector <bool> > vesAvailability; //local vessel availability vector
	vesAvailability.resize(VesAvail.size());
	int planHorizon = NUMBER_OF_DAYS + 
		(int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);

	for (k = 0; k < VesAvail.size(); k++)
		{
			vesAvailability[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				vesAvailability[k][i] = VesAvail[k][i];
		}


	//cout << "-----------Entering RelocateVisits----------" << endl;
	//cout << "Before relocateVisits() schedule cost = " << computeSchedCost() << endl;

while (improvementFound)
{
	//cout << "Routes.size() = " << Routes.size() << endl;
	//cout << "intermedRoutes.size() = " << intermedRoutes.size() << endl;
	//printWeeklySchedule();

	improvementFound = false;
	numRoutesReduced = false;
	
	for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
	{//Try to relocate (some of the) route visits
	
		//cout << "routeIter->getRouteNumber() = " << routeIter->getRouteNum() << endl;

		routeVisits.clear();
		routeVisits = routeIter->getVisitObjects();
		VisCombs.clear();
		VisCombs.resize(routeIter->getVisitObjects().size());

		for (visitIter = routeVisits.begin()+1; visitIter != routeVisits.end()-1; ++visitIter)
		{
			VisVariations.clear();
			bestDeltaObj = numeric_limits<double>::max();

			for (routeRelocIter = Routes.begin(); routeRelocIter != Routes.end(); ++routeRelocIter)
			{
				//do not reinsert in the same route, check load feasibility
				//if the installation is already on the route and even spread
				//cout << "routeRelocIter->getRouteNumber = " << routeRelocIter->getRouteNum() << endl;

				
				if ( (routeIter != routeRelocIter)
				&& (routeRelocIter->computeRouteDemand() + visitIter->getOffshoreInst()->getWeeklyDemand()
				/ visitIter->getOffshoreInst()->getVisitFreq() < routeRelocIter->getRouteVes()->getCapacity())
				&& ( !routeRelocIter->isInstOnRoute( (visitIter->getOffshoreInst()) ) ) 
				&& (isDepartureSpreadEven(&(*routeIter), &(*routeRelocIter), &(*visitIter)) ) 
				&& (routeRelocIter->getVisitObjects().size() - 2 < routeRelocIter->getMaxVisits()) )
				{
					//cout << "Sched Before!!!" << endl;
					schedCostBefore = computeSchedCost();
		
					//printWeeklySchedule();

					routeToDurBefore = routeRelocIter->computeRouteDuration();
					routeRelocIter->insertInstalVisit(1, *visitIter);
					routeRelocIter->updateVisitVector();
					routeRelocIter->intelligentReorder();
					routeToDurAfter = routeRelocIter->computeRouteDuration();

					if ( (routeToDurAfter < routeRelocIter->MAX_ROUTE_DUR + routeRelocIter->getRouteAcceptanceTime())
						&& ( ceil( (routeToDurBefore + routeRelocIter->getRouteMinSlack() +
						routeRelocIter->getInstAtPos(0)->getLayTime())/24 )
							>= ceil( (routeToDurAfter + routeRelocIter->getRouteMinSlack() +
							routeRelocIter->getInstAtPos(0)->getLayTime())/24 ) ) )
					{	
						routeFromDurBefore = routeIter->computeRouteDuration();
						routeIter->deleteInstalVisit( visitIter->getIdNumber() );
						routeIter->updateVisitVector();
						routeIter->intelligentReorder();
						routeFromDurAfter = routeIter->computeRouteDuration();

						if (routeFromDurAfter > routeIter->MIN_ROUTE_DUR + 
							routeIter->getRouteAcceptanceTime() )
						{
							schedCostAfter = computeSchedCost();

							if ( (routeIter->getVisitObjects().size() == 2) &&
								(routeIter->getRouteVes()->getVesselRoutes().size()==1) )
								schedCostAfter -= routeIter->getRouteVes()->getVesselCost();

							//cout << "Sched After!!!" << endl;
							//printWeeklySchedule();
							
							//Initialize VisitVariation objects
							VisitVariation visVar(&(*visitIter));
							visVar.setRouteFrom(&(*routeIter));
							visVar.setRouteTo(&(*routeRelocIter));
							visVar.setRouteFromDurDecrease(routeFromDurBefore - routeFromDurAfter);
							visVar.setRouteToDurIncrease(routeToDurAfter - routeToDurBefore);
							visVar.setDeltaObj(schedCostAfter - schedCostBefore);

							VisVariations.push_back(visVar);
						}

						//Restore affected routes.
						for (routeIntermedIter = intermedRoutes.begin(); 
							routeIntermedIter->getRouteNum() !=  routeIter->getRouteNum();
							++routeIntermedIter)
							;
						*routeIter = *routeIntermedIter;
					}//end if
					
					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  routeRelocIter->getRouteNum();
						++routeIntermedIter)
						;
					*routeRelocIter = *routeIntermedIter;
					
				}//end if
					
			}//end for routeRelocIter
			
			//No feasible insertion found - go on to the next routeIter
			if (VisVariations.size() == 0)
			{
				//cout << "No insertion found" << endl;
				continue; // continue to the next visitIter
			}
			else // Identify best relocation (in terms of obj. function)
			{
				for (l = 0; l < VisVariations.size(); l++)
					if (VisVariations[l].getDeltaObj() < bestDeltaObj)
					{
						bestDeltaObj = VisVariations[l].getDeltaObj();
						bestVarIdx = l;
						//cout << "Best DeltaObj = " << bestDeltaObj << endl;
					}
				
				if ( bestDeltaObj < 0 )
				{
					/*
					cout << "Relocating Installation " << VisVariations[bestVarIdx].getVarVisit()->getOffshoreInst()->getInstName() << endl;
					
					cout << "OLD visit day combination for the installation " << visitIter->getOffshoreInst()->getInstName()
						<< " is ";
					for (l = 0; l < visitIter->getOffshoreInst()->CurVisitDayComb.size(); l++)
						cout << visitIter->getOffshoreInst()->CurVisitDayComb[l] << " ";

					cout << endl;

					cout << "Best insertion from route " << VisVariations[bestVarIdx].getRouteFrom()->getRouteStartTime() 
						<< " to route " << VisVariations[bestVarIdx].getRouteTo()->getRouteStartTime() << endl;

					cout << "DeltaObj = " << VisVariations[bestVarIdx].getDeltaObj() << endl;
					cout << "RouteFromDurDecrease = " << VisVariations[bestVarIdx].getRouteFromDurDecrease() << endl;
					cout << "RouteToDurIncrease = " << VisVariations[bestVarIdx].getRouteToDurIncrease() << endl;

					cout << "Position of visit on the route is " << visitIter->getIdNumber() << endl;
					cout << "Printing the routes BEFORE insertions" << endl << endl;

					VisVariations[bestVarIdx].getRouteFrom()->printRoute();
					VisVariations[bestVarIdx].getRouteTo()->printRoute();

					*/

					//Implement best relocation if it reduces the cost
					//cout << "Improvement in relocateVisits()" << endl;
					improvementFound = true;

					VisVariations[bestVarIdx].getRouteTo()->insertInstalVisit(1, *visitIter);
					VisVariations[bestVarIdx].getRouteFrom()->deleteInstalVisit(visitIter->getIdNumber());

					VisVariations[bestVarIdx].getRouteTo()->updateVisitVector();
					VisVariations[bestVarIdx].getRouteFrom()->updateVisitVector();

					VisVariations[bestVarIdx].getRouteTo()->intelligentReorder();
					VisVariations[bestVarIdx].getRouteFrom()->intelligentReorder();

					//Update visit day combination
					updateVisDayComb(VisVariations[bestVarIdx].getRouteFrom(),
					VisVariations[bestVarIdx].getRouteTo(), &(*visitIter) );
					/*
					cout << endl;
					cout << "New visit day combination for the installation " << visitIter->getOffshoreInst()->getInstName()
						<< " is ";
					for (l = 0; l < visitIter->getOffshoreInst()->CurVisitDayComb.size(); l++)
						cout << visitIter->getOffshoreInst()->CurVisitDayComb[l] << " ";

					cout << endl;

					cout << "Printing the routes after insertions" << endl << endl;
					
					VisVariations[bestVarIdx].getRouteFrom()->printRoute();
					VisVariations[bestVarIdx].getRouteTo()->printRoute();
					*/

					//Set visit sequential numbers
					VisVariations[bestVarIdx].getRouteFrom()->setVisitIds();
					VisVariations[bestVarIdx].getRouteTo()->setVisitIds();
					

					//Update intermediate route vectors
					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  
						VisVariations[bestVarIdx].getRouteFrom()->getRouteNum();
						++routeIntermedIter)
							;
					*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteFrom());

					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  
						VisVariations[bestVarIdx].getRouteTo()->getRouteNum();
						++routeIntermedIter)
						;
					*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteTo());

					//Check if we could reduce the number of routes
					if (VisVariations[bestVarIdx].getRouteFrom()->getVisitObjects().size() == 2)
					{
						cout << "Erasing route number " << 
							VisVariations[bestVarIdx].getRouteFrom()->getRouteNum() 
							<< endl; 

						i = 0;
						//Update the solution
						for (routeIntermedIter = intermedRoutes.begin(); 
							routeIntermedIter->getRouteNum() !=  
							VisVariations[bestVarIdx].getRouteFrom()->getRouteNum();
							++routeIntermedIter)
							i++;

						routeStartIndex = (int) ceil(routeIntermedIter->getRouteStartTime()/24);

						routeEndIndex = (int) ceil( (routeIntermedIter->getRouteEndTime() - 
							VisVariations[bestVarIdx].getRouteFrom()->getInstAtPos(0)->getLayTime() +
							routeIntermedIter->getRouteMinSlack() )/24 );
						

						for (l = routeStartIndex; l <= routeEndIndex; l++)
							VesAvail[routeIntermedIter->getRouteVes()->getID()][l]=true;

						//routeIntermedIter->getRouteVes()->eraseVesselRoute(routeIntermedIter->getRouteNum());
						intermedRoutes.erase(intermedRoutes.begin()+i);
						Routes.erase(Routes.begin()+i);
						
						numRoutesReduced = true;

						//reassignVesselsToRoutes();
						
					}
					//cout << "Before break 1" << endl;
					break;

				}//end if (bestDeltaObj < 0)
			
			}//end else

		}//end for visitIter

		if (numRoutesReduced)
		{
			//cout << "before break 2" << endl;
			break;	
		}
	}//end for routeIter

	if (numRoutesReduced)
	{
		//For each vessel clear and update vesselRoutes vector
		for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
				{
					if (vesIter->getVesselRoutes().size() > 0)
					{
						vesIter->clearVesselRoutes();
						for (routeIter = Routes.begin(); 
							routeIter != Routes.end(); ++routeIter)
							if (vesIter->getID() == routeIter->getRouteVes()->getID() )
								vesIter->addVesselRoute(&(*routeIter));
					}
					else
						vesIter->setIsVesselUsed(false);
				}//end for
	}//end if

}//end while (improvementFound)

//cout << "After relocateVisits() schedule cost = " << computeSchedCost() << endl;
}

void WeeklySchedule::writeScheduleToFile()
{
	const char* fileName;
	fileName = "C:/Aliaksandr Shyshou/Supply Vessels 2008-2009/Heuristics for PSVRP/MongstadInst_PSVRP_Heuristic/output.txt";
	ofstream solFile(fileName, ios::app);

	solFile << "Number of routes = " << Routes.size() << endl;
	vector <Route>::iterator it;
	int i, k, l;
	vector <OffshoreInst>::iterator instIter;

	solFile << "Vessel availability vector" << endl;

	for (k = 0; k < VesAvail.size(); k++)
	{
		for (i = 0; i < VesAvail[k].size(); i++)
			solFile << VesAvail[k][i] << " ";

		solFile << endl;
	}

	//Print visit day combinations
	solFile << "Installations: Visit day combinations" << endl;
	for (instIter = SchedInstals.begin(); instIter != SchedInstals.end(); ++instIter)
	{
		solFile << instIter->getInstName() << ": ";
		for (l = 0; l < instIter->CurVisitDayComb.size(); l++)
			solFile << instIter->CurVisitDayComb[l] << " ";

		solFile << endl;
	}

	vector < Visit >::iterator iter;
	vector <Visit> Visits;

	for (it = Routes.begin(); it != Routes.end(); ++it)
	{
		solFile << "____________" << it->getRouteVes()->getName() << " id = "
			<< it->getRouteVes()->getID() << "_____________" << endl;
		solFile << "Route number " << it->getRouteNum() << endl;
		Visits.clear();
		Visits = it->getVisitObjects();
		
		for (iter = Visits.begin(); iter != Visits.end(); ++iter)
		{
			solFile << iter->getOffshoreInst()->getInstName() << " ";
			solFile << "Arrival = " << iter->getVisitStart() << " ";
			solFile << "Departure = " << iter->getVisitEnd() << " ";
			solFile << "WaitTime = " << iter->getVisitWaitTime();
			solFile << endl;
		}

	solFile << endl;
	}


}

void WeeklySchedule::minimizeTotalSlack()
{
	vector <Route>::iterator routeIter;
	vector <Route>::iterator routeRelocIter;
	vector <Route>::iterator routeIntermedIter;

	vector <Visit> routeVisits;
	vector <Visit>::iterator visitIter;

	vector <SupVes>::iterator vesIter;

	int i, j, k, l;
	int routeStartIndex, routeEndIndex;

	vector <Route> intermedRoutes; //additional vector of routes
	vector <Route> savedRoutes; //to restore routes if infeasible
	intermedRoutes.resize(Routes.size());
	savedRoutes.resize(Routes.size());

	vector < vector <int> > VisCombs;

	for (l = 0; l < Routes.size(); l++)
	{
		intermedRoutes[l] = Routes[l];
		savedRoutes[l] = Routes[l];
	}

	double routeToSlackBefore, routeToSlackAfter;
	double routeFromSlackBefore, routeFromSlackAfter;
	double totalSlackBefore, totalSlackAfter;
	double schedCostBefore, schedCostAfter;

	double routeToDurBefore, routeToDurAfter;
	double routeFromDurBefore, routeFromDurAfter;

	double bestDeltaObj, bestDeltaSlack;
	int bestVarIdx;

	int routeDays;

	bool improvementFound = true;
	bool numRoutesReduced;

	vector <vector <bool> > vesAvailability; //local vessel availability vector
	vesAvailability.resize(VesAvail.size());
	int planHorizon = NUMBER_OF_DAYS + 
		(int) ceil((double)Routes.begin()->MAX_ROUTE_DUR/24);

	for (k = 0; k < VesAvail.size(); k++)
		{
			vesAvailability[k].resize(planHorizon);
			for (i = 0; i < planHorizon; i++)
				vesAvailability[k][i] = VesAvail[k][i];
		}

	//cout << "BEFORE minimizeTotalSlack()" << endl;
	//cout << "ScheduleCost = " << computeSchedCost() << endl;
	//cout << "Total slack = " << computeTotalSlack() << endl;

while (improvementFound)
{
	//cout << "Routes.size() = " << Routes.size() << endl;
	//cout << "intermedRoutes.size() = " << intermedRoutes.size() << endl;
	//printWeeklySchedule();

	improvementFound = false;
	numRoutesReduced = false;
	
	for (routeIter = Routes.begin(); routeIter != Routes.end(); ++routeIter)
	{//Try to relocate (some of the) route visits
	
		//cout << "routeIter->getRouteNumber() = " << routeIter->getRouteNum() << endl;

		
		routeVisits.clear();
		routeVisits = routeIter->getVisitObjects();
		VisCombs.clear();
		VisCombs.resize(routeIter->getVisitObjects().size());
	
		j=1;
		routeDays = (int) ceil( (routeIter->computeRouteDuration() + 
						routeIter->getInstAtPos(0)->getLayTime() +
						routeIter->getRouteMinSlack() )/24 );

		for (visitIter = routeVisits.begin()+1; visitIter != routeVisits.end()-1; ++visitIter)
		{
			
			VisVariations.clear();
			
			bestDeltaObj = numeric_limits<double>::max();
			bestDeltaSlack = numeric_limits<double>::max();
	

			for (routeRelocIter = Routes.begin(); routeRelocIter != Routes.end(); ++routeRelocIter)
			{
				//do not reinsert in the same route, check load feasibility
				//if the installation is already on the route and even spread
				//cout << "routeRelocIter->getRouteNumber = " << routeRelocIter->getRouteNum() << endl;

				
				if ( (routeIter != routeRelocIter)
				&& (routeRelocIter->computeRouteDemand() + visitIter->getOffshoreInst()->getWeeklyDemand()
				/ visitIter->getOffshoreInst()->getVisitFreq() < routeRelocIter->getRouteVes()->getCapacity())
				&& ( !routeRelocIter->isInstOnRoute( (visitIter->getOffshoreInst()) ) ) 
				&& (isDepartureSpreadEven(&(*routeIter), &(*routeRelocIter), &(*visitIter)) ) 
				&& (routeRelocIter->getVisitObjects().size() - 2 < routeRelocIter->getMaxVisits()) )
				{
					//printWeeklySchedule();
					schedCostBefore = computeSchedCost();
					totalSlackBefore = computeTotalSlack();

					routeToDurBefore = routeRelocIter->computeRouteDuration();
					routeToSlackBefore = routeRelocIter->computeRouteSlack();

					routeRelocIter->insertInstalVisit(1, *visitIter);
					routeRelocIter->updateVisitVector();
					routeRelocIter->intelligentReorder();
					routeToDurAfter = routeRelocIter->computeRouteDuration();
					routeToSlackAfter = routeRelocIter->computeRouteSlack();

					if ( (routeToDurAfter < routeRelocIter->MAX_ROUTE_DUR + routeRelocIter->getRouteAcceptanceTime())
						&& ( ceil( (routeToDurBefore + routeRelocIter->getRouteMinSlack() +
						routeRelocIter->getInstAtPos(0)->getLayTime())/24 )
							>= ceil( (routeToDurAfter + routeRelocIter->getRouteMinSlack() +
							routeRelocIter->getInstAtPos(0)->getLayTime())/24 ) ) )
					{	
						routeFromDurBefore = routeIter->computeRouteDuration();
						routeFromSlackBefore = routeIter->computeRouteSlack();
						routeIter->deleteInstalVisit( j );
						routeIter->updateVisitVector();
						routeIter->intelligentReorder();
						routeFromDurAfter = routeIter->computeRouteDuration();
						routeFromSlackAfter = routeIter->computeRouteSlack();

						if ( (routeFromDurAfter > routeIter->MIN_ROUTE_DUR + 
							routeIter->getRouteAcceptanceTime() ) && 
							( ceil( (routeFromDurBefore + routeRelocIter->getRouteMinSlack() +
						routeRelocIter->getInstAtPos(0)->getLayTime())/24 )
							> ceil( (routeFromDurAfter + routeRelocIter->getRouteMinSlack() +
							routeRelocIter->getInstAtPos(0)->getLayTime())/24 ) ) )
						{
							schedCostAfter = computeSchedCost();
							totalSlackAfter = computeTotalSlack();
							
							//cout << "Sched After!!!" << endl;
							//printWeeklySchedule();
							
							//Initialize VisitVariation objects
							VisitVariation visVar(&(*visitIter));
							visVar.setRouteFrom(&(*routeIter));
							visVar.setRouteTo(&(*routeRelocIter));
							visVar.setRouteFromDurDecrease(routeFromDurBefore - routeFromDurAfter);
							visVar.setRouteToDurIncrease(routeToDurAfter - routeToDurBefore);
							visVar.setTotalSlackDelta(totalSlackAfter - totalSlackBefore);
							visVar.setDeltaObj(schedCostAfter - schedCostBefore);

							VisVariations.push_back(visVar);
						}

						//Restore affected routes.
						for (routeIntermedIter = intermedRoutes.begin(); 
							routeIntermedIter->getRouteNum() !=  routeIter->getRouteNum();
							++routeIntermedIter)
							;
						*routeIter = *routeIntermedIter;
					}//end if
					
					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  routeRelocIter->getRouteNum();
						++routeIntermedIter)
						;
					*routeRelocIter = *routeIntermedIter;
					
				}//end if
					
			}//end for routeRelocIter
			
			//No feasible insertion found - go on to the next routeIter
			if (VisVariations.size() == 0)
			{
				//cout << "No insertion found" << endl;
				j++;
				continue; // continue to the next visitIter
			}
			else // Identify best relocation (in terms of obj. function)
			{
				for (l = 0; l < VisVariations.size(); l++)
					if (VisVariations[l].getTotalSlackDelta() < bestDeltaSlack)
					{
						bestDeltaSlack = VisVariations[l].getTotalSlackDelta();
						bestVarIdx = l;
						//cout << "Best DeltaObj = " << bestDeltaObj << endl;
					}
				
			
				/*
				cout << "Relocating Installation " << VisVariations[bestVarIdx].getVarVisit()->getOffshoreInst()->getInstName() << endl;
				
				cout << "OLD visit day combination for the installation " << visitIter->getOffshoreInst()->getInstName()
					<< " is ";
				for (l = 0; l < visitIter->getOffshoreInst()->CurVisitDayComb.size(); l++)
					cout << visitIter->getOffshoreInst()->CurVisitDayComb[l] << " ";

				cout << endl;

				cout << "Best insertion from route " << VisVariations[bestVarIdx].getRouteFrom()->getRouteStartTime() 
					<< " to route " << VisVariations[bestVarIdx].getRouteTo()->getRouteStartTime() << endl;

				cout << "DeltaObj = " << VisVariations[bestVarIdx].getDeltaObj() << endl;
				cout << "RouteFromDurDecrease = " << VisVariations[bestVarIdx].getRouteFromDurDecrease() << endl;
				cout << "RouteToDurIncrease = " << VisVariations[bestVarIdx].getRouteToDurIncrease() << endl;

				cout << "Position of visit on the route is " << visitIter->getIdNumber() << endl;
				cout << "Printing the routes BEFORE insertions" << endl << endl;

				VisVariations[bestVarIdx].getRouteFrom()->printRoute();
				VisVariations[bestVarIdx].getRouteTo()->printRoute();

				*/

				//Implement best relocation if it reduces the cost
				//cout << "Improvement in relocateVisits()" << endl;
				improvementFound = true;

				VisVariations[bestVarIdx].getRouteTo()->insertInstalVisit(1, *visitIter);
				VisVariations[bestVarIdx].getRouteFrom()->deleteInstalVisit( j );

				VisVariations[bestVarIdx].getRouteTo()->updateVisitVector();
				VisVariations[bestVarIdx].getRouteFrom()->updateVisitVector();

				VisVariations[bestVarIdx].getRouteTo()->intelligentReorder();
				VisVariations[bestVarIdx].getRouteFrom()->intelligentReorder();

				//j++;

				//Update visit day combination
				updateVisDayComb(VisVariations[bestVarIdx].getRouteFrom(),
				VisVariations[bestVarIdx].getRouteTo(), &(*visitIter) );
				/*
				cout << endl;
				cout << "New visit day combination for the installation " << visitIter->getOffshoreInst()->getInstName()
					<< " is ";
				for (l = 0; l < visitIter->getOffshoreInst()->CurVisitDayComb.size(); l++)
					cout << visitIter->getOffshoreInst()->CurVisitDayComb[l] << " ";

				cout << endl;

				cout << "Printing the routes after insertions" << endl << endl;
				
				VisVariations[bestVarIdx].getRouteFrom()->printRoute();
				VisVariations[bestVarIdx].getRouteTo()->printRoute();
				*/

				//Set visit sequential numbers
				VisVariations[bestVarIdx].getRouteFrom()->setVisitIds();
				VisVariations[bestVarIdx].getRouteTo()->setVisitIds();
				

				//Update intermediate route vectors
				for (routeIntermedIter = intermedRoutes.begin(); 
					routeIntermedIter->getRouteNum() !=  
					VisVariations[bestVarIdx].getRouteFrom()->getRouteNum();
					++routeIntermedIter)
						;
				*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteFrom());

				for (routeIntermedIter = intermedRoutes.begin(); 
					routeIntermedIter->getRouteNum() !=  
					VisVariations[bestVarIdx].getRouteTo()->getRouteNum();
					++routeIntermedIter)
					;
				*routeIntermedIter = *(VisVariations[bestVarIdx].getRouteTo());

				//Check if we could reduce the number of routes
				if (VisVariations[bestVarIdx].getRouteFrom()->getVisitObjects().size() == 2)
				{
					cout << "Erasing route number " << 
						VisVariations[bestVarIdx].getRouteFrom()->getRouteNum() 
						<< endl; 

					i = 0;
					//Update the solution
					for (routeIntermedIter = intermedRoutes.begin(); 
						routeIntermedIter->getRouteNum() !=  
						VisVariations[bestVarIdx].getRouteFrom()->getRouteNum();
						++routeIntermedIter)
						i++;

					routeStartIndex = (int) ceil(routeIntermedIter->getRouteStartTime()/24);

					routeEndIndex = routeStartIndex + routeDays - 1;
					

					for (l = routeStartIndex; l <= routeEndIndex; l++)
						VesAvail[routeIntermedIter->getRouteVes()->getID()][l]=true;

					//routeIntermedIter->getRouteVes()->eraseVesselRoute(routeIntermedIter->getRouteNum());
					intermedRoutes.erase(intermedRoutes.begin()+i);
					Routes.erase(Routes.begin()+i);
					
					numRoutesReduced = true;

					//reassignVesselsToRoutes();
					
				}
				//cout << "Before break 1" << endl;
				else if ( (int)ceil( (routeIter->computeRouteDuration() + routeIter->getInstAtPos(0)->getLayTime()
						+ routeIter->getRouteMinSlack() )/24) < routeDays )
				{
					/*
					cout << "Improvement in reduceRouteDurTotDays()" << endl;
					cout << "Old route days = " << routeDays << endl;
					cout << "New route days = " << (int)ceil( (routeIter->computeRouteDuration() + routeIter->getInstAtPos(0)->getLayTime()
					+ routeIter->getRouteMinSlack() )/24) << endl;
					cout << "Route number = " << routeIter->getRouteNum() << endl;
					cout << "VESSEL: "<< routeIter->getRouteVes()->getName() << endl;
					routeIter->printRoute();
					*/

					//Update the solution
					routeStartIndex = (int) ceil(routeIter->getRouteStartTime()/24);
					routeEndIndex = (int) ceil ((routeIter->getRouteEndTime() - routeIter->getInstAtPos(0)->getLayTime() 
						+ routeIter->getRouteMinSlack() )/24);
				
					VesAvail[routeIter->getRouteVes()->getID()][routeEndIndex+1]=true;
				}//end else if

				break;
			}//end else
		}//end for visitIter

		if (numRoutesReduced)
		{
			//cout << "before break 2" << endl;
			break;	
		}
	}//end for routeIter

	if (numRoutesReduced)
	{
		//For each vessel clear and update vesselRoutes vector
		for (vesIter = SchedVessels.begin(); vesIter != SchedVessels.end(); ++vesIter)
				{
					if (vesIter->getVesselRoutes().size() > 0)
					{
						vesIter->clearVesselRoutes();
						for (routeIter = Routes.begin(); 
							routeIter != Routes.end(); ++routeIter)
							if (vesIter->getID() == routeIter->getRouteVes()->getID() )
								vesIter->addVesselRoute(&(*routeIter));
					}
					else
						vesIter->setIsVesselUsed(false);
				}//end for
	}//end if

}//end while (improvementFound)

//cout << "After minimizeTotalSlack()" << endl;
//cout << "ScheduleCost = " << computeSchedCost()<<endl;
//cout << "Total slack = " << computeTotalSlack() << endl << endl;

}

