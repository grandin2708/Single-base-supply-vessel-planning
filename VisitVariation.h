#pragma once

class Visit;
class Route;

class VisitVariation
{
	//private members
	Route* routeFrom;
	Route* routeTo;
	Visit* varVisit; //pointer to a visit being relocated
	int insertionPos;
	int removalPos;
	double deltaObj;
	double routeToDurIncrease;
	double routeFromDurDecrease;
	double totalSlackDelta;

public:
	VisitVariation(void);
	VisitVariation(Visit *varVis);

	Route* getRouteFrom() {return routeFrom;}
	Route* getRouteTo() {return routeTo;}
	Visit* getVarVisit() {return varVisit;}
	int getInsertionPos() {return insertionPos;}
	int getRemovalPos() {return removalPos;}
	double getDeltaObj() {return deltaObj;}
	double getRouteToDurIncrease() {return routeToDurIncrease;}
	double getRouteFromDurDecrease() {return routeFromDurDecrease;}
	double getTotalSlackDelta() {return totalSlackDelta;}

	void setRouteFrom(Route* rouFr) {routeFrom = rouFr;}
	void setRouteTo(Route* rouTo) {routeTo = rouTo;}
	void setInsertionPos (int insPos) {insertionPos = insPos;}
	void setRemovalPos (int remPos) {removalPos = remPos;}
	void setDeltaObj (double delObj) {deltaObj = delObj;}
	void setRouteToDurIncrease (double rouIncr) {routeToDurIncrease = rouIncr;}
	void setRouteFromDurDecrease (double rouDecr) {routeFromDurDecrease = rouDecr;}
	void setTotalSlackDelta (double delta) {totalSlackDelta = delta;}
};
