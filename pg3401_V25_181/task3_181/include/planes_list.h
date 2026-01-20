#ifndef PLANES_LIST_H
#define PLANES_LIST_H

#include <stddef.h>

#define MAX_NAME_LEN 50
#define MAX_FLIGHTID_LEN 6
#define MAX_DEST_LEN 50

struct PASSENGERS {
	char NAME[MAX_NAME_LEN];
	int  SEAT_NUMBER;
	int  AGE;
};

struct Planes {
	char FLIGHTID[MAX_FLIGHTID_LEN];
	char DESTINATION[MAX_DEST_LEN];
	int SEATS;
	int TIME;
	int passengerCount;   
	struct PASSENGERS *passengers;     
};

typedef struct PlaneNode {
	struct Planes     data;
	struct PlaneNode *prev, *next;
} PlaneNode;

PlaneNode* appendPlane(PlaneNode *head, const struct Planes *p);
void printList(const PlaneNode *head);
PlaneNode* addFlight(PlaneNode *head);
PlaneNode* retrieveFlightByIndex(PlaneNode *head, int index);
PlaneNode* retrieveFlight(PlaneNode *head, const char *flightID);
PlaneNode* deleteFlight(PlaneNode *head, const char *flightID);
int addPassengerToFlight(PlaneNode *head, const char *flightID);
int changePassengerSeat(PlaneNode *head, const char *flightID, const char *passengerName,int newSeat);
void findFlightsByDestination(PlaneNode *head, const char *dest);
void printFlightsOfPassenger(PlaneNode *head, const char *passengerName);
void printPassengersWithMultipleBookings(PlaneNode *head);
void freeAllFlights(PlaneNode *head);

#endif // PLANES_LIST_H
