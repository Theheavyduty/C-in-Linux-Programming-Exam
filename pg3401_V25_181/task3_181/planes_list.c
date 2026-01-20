#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "planes_list.h"

static PlaneNode* createNode(const struct Planes *p)
{
	PlaneNode *n;
	// Allocate memory for the node 
	n = malloc(sizeof *n);
	if (!n) {
		perror("Failed to malloc");
		exit(1);
	}
	// Ensure all fields start at zero 
	memset(n, 0, sizeof *n);
	// Copy the flight data into the node 
	n->data = *p;
	return n;
}

PlaneNode* appendPlane(PlaneNode *head, const struct Planes *p)
{
	PlaneNode *n;
	PlaneNode *t;
	// Create a new node 
	n = createNode(p);
	// If list is empty, new node becomes head
	if (!head) {
		return n;
	}
	// Move pointer to the real head of the list
	t = head;
	while (t->prev) {
		t = t->prev;
	}
	// Traverse to the tail
	while (t->next) {
		t = t->next;
	}
	// Link the new node at the end
	t->next = n;
	n->prev = t;
	return head;
}

void printList(const PlaneNode *head)
{
	int cnt, i, j;
	PlaneNode **arr;
	const PlaneNode *p;
	const PlaneNode *q;

	// Rewind to head 
	p = head;
	while (p && p->prev) {
		p = p->prev;
	}
	// Count flights
	cnt = 0;
	for (q = p; q; q = q->next) {
		cnt++;
	}
	if (!cnt) {
		printf("No flights to show.\n");
		return;
	}
	// Allocate and zero pointer array
	arr = malloc(cnt * sizeof *arr);
	if (!arr) {
		perror("Failed to malloc");
		exit(1);
	}
	memset(arr, 0, cnt * sizeof *arr);
	// Populate array 
	i = 0;
	for (q = p; q; q = q->next) {
		arr[i++] = (PlaneNode*)q;
	}
	// Sort by available seats 
	for (i = 0; i < cnt - 1; i++) {
		for (j = i + 1; j < cnt; j++) {
			int avail_i = arr[i]->data.SEATS - arr[i]->data.passengerCount;
			int avail_j = arr[j]->data.SEATS - arr[j]->data.passengerCount;
			if (avail_i > avail_j) {
				PlaneNode *tmp = arr[i];
				arr[i] = arr[j];
				arr[j] = tmp;
			}
		}
	}
	// Print sorted flights 
	for (i = 0; i < cnt; i++) {
		PlaneNode *f = arr[i];
		int avail = f->data.SEATS - f->data.passengerCount;
		printf("Flight %s → Dest: %s, Seats: %d, Time: %04d, Booked: %d (Avail: %d)\n",
				f->data.FLIGHTID,
				f->data.DESTINATION,
				f->data.SEATS,
				f->data.TIME,
				f->data.passengerCount,
				avail);
	}
	free(arr);
}

PlaneNode* addFlight(PlaneNode *head)
{
	struct Planes p;
	// Gather flight info
	printf("Enter FlightID: ");
	scanf("%9s", p.FLIGHTID);
	printf("Enter Destination: ");
	scanf("%49s", p.DESTINATION);
	printf("Enter #Seats: ");
	scanf("%d", &p.SEATS);
	printf("Enter Departure Time HHMM: ");
	scanf("%d", &p.TIME);
	// Validate seats
	if (p.SEATS < 1) {
		printf("Must have at least one seat.\n");
		return head;
	}
	// Init and allocate passenger array 
	p.passengerCount = 0;
	p.passengers = malloc(p.SEATS * sizeof *p.passengers);
	if (!p.passengers) {
		perror("Failed to malloc");
		exit(1);
	}
	memset(p.passengers, 0, p.SEATS * sizeof *p.passengers);
	// Append new flight 
	return appendPlane(head, &p);
}

PlaneNode* retrieveFlight(PlaneNode *head, const char *id)
{
	PlaneNode *p;
	// Rewind to head
	p = head;
	while (p && p->prev) {
		p = p->prev;
	}
	// Search by ID 
	while (p) {
		if (strcmp(p->data.FLIGHTID, id) == 0) {
			printf("Flight %s → Dest: %s, Seats: %d, Time: %04d, Booked: %d\n",
					p->data.FLIGHTID,
					p->data.DESTINATION,
					p->data.SEATS,
					p->data.TIME,
					p->data.passengerCount);
			return p;
		}
		p = p->next;
	}
	printf("Flight %s not found.\n", id);
	return NULL;
}

PlaneNode* deleteFlight(PlaneNode *head, const char *id)
{
	PlaneNode *p;
	// Find node 
	p = head;
	while (p && strcmp(p->data.FLIGHTID, id) != 0) {
		p = p->next;
	}
	if (!p) {
		printf("Flight %s not found.\n", id);
		return head;
	}
	// Unlink 
	if (p->prev) {
		p->prev->next = p->next;
	} else {
		head = p->next;
	}
	if (p->next) {
		p->next->prev = p->prev;
	}
	// Free memory 
	free(p->data.passengers);
	free(p);
	printf("Deleted flight %s.\n", id);
	return head;
}

int addPassengerToFlight(PlaneNode *head, const char *flightID)
{
	PlaneNode *f;
	struct PASSENGERS pax;
	int i;
	// Find flight 
	f = retrieveFlight(head, flightID);
	if (!f) {
		return 1;
	}
	//Check full
	if (f->data.passengerCount >= f->data.SEATS) {
		printf("Flight is full.\n");
		return 2;
	}
	// Get passenger info 
	printf("Enter passenger name: ");
	scanf("%49s", pax.NAME);
	printf("Enter seat number: ");
	scanf("%d", &pax.SEAT_NUMBER);
	printf("Enter age: ");
	scanf("%d", &pax.AGE);
	// Validate seat 
	if (pax.SEAT_NUMBER < 1 || pax.SEAT_NUMBER > f->data.SEATS) {
		printf("Invalid seat. Must be 1–%d.\n", f->data.SEATS);
		return 3;
	}
	// Check if taken 
	for (i = 0; i < f->data.passengerCount; i++) {
		if (f->data.passengers[i].SEAT_NUMBER == pax.SEAT_NUMBER) {
			printf("Seat %d already booked.\n", pax.SEAT_NUMBER);
			return 4;
		}
	}
	// Insert sorted 
	i = f->data.passengerCount;
	while (i > 0 && pax.SEAT_NUMBER < f->data.passengers[i-1].SEAT_NUMBER) {
		f->data.passengers[i] = f->data.passengers[i-1];
		i--;
	}
	f->data.passengers[i] = pax;
	f->data.passengerCount++;
	printf("Passenger %s booked on seat %d.\n", pax.NAME, pax.SEAT_NUMBER);
	return 0;
}

PlaneNode* retrieveFlightByIndex(PlaneNode *head, int index)
{
	PlaneNode *p;
	int i;
	// Validate index 
    if (index < 1) {
		printf("Invalid flight number %d.\n", index);
		return NULL;
	}
	// Rewind to head 
	p = head;
	while (p && p->prev) {
		p = p->prev;
	}
	// Advance 
    for (i = 1; p && i < index; i++) {
		p = p->next;
	}
	if (!p) {
		printf("Flight number %d not found.\n", index);
		return NULL;
	}
	// Print summary 
	printf("Flight %s → Dest: %s, Seats: %d, Time: %04d, Booked: %d\n",
			p->data.FLIGHTID,
			p->data.DESTINATION,
			p->data.SEATS,
			p->data.TIME,
			p->data.passengerCount);
	// List passengers 
	if (p->data.passengerCount > 0) {
		printf("Passengers (%d):\n", p->data.passengerCount);
		for (i = 0; i < p->data.passengerCount; i++) {
			struct PASSENGERS *q = &p->data.passengers[i];
			printf("  - Name: %s, Seat: %d, Age: %d\n",
					q->NAME, q->SEAT_NUMBER, q->AGE);
		}
	} else {
		printf("(no passengers booked)\n");
	}
	return p;
}

int changePassengerSeat(PlaneNode *head, const char *flightID,
						const char *passengerName, int newSeat)
{
	PlaneNode *f;
	struct PASSENGERS tmp;
	int idx, i;
	// Find flight 
	f = retrieveFlight(head, flightID);
	if (!f) {
		return 1;
	}
	// Validate new seat 
	if (newSeat < 1 || newSeat > f->data.SEATS) {
		printf("Invalid seat. Must be 1–%d.\n", f->data.SEATS);
		return 3;
	}
	// Locate passenger and check availability 
	idx = -1;
	for (i = 0; i < f->data.passengerCount; i++) {
		if (strcmp(f->data.passengers[i].NAME, passengerName) == 0) {
			idx = i;
		}
		if (f->data.passengers[i].SEAT_NUMBER == newSeat) {
			printf("Seat %d already booked.\n", newSeat);
			return 4;
		}
	}
	if (idx < 0) {
		printf("Passenger %s not on %s.\n", passengerName, flightID);
		return 2;
	}
	// Remove and reinsert at new position
	tmp = f->data.passengers[idx];
	tmp.SEAT_NUMBER = newSeat;
	for (i = idx; i < f->data.passengerCount - 1; i++) {
		f->data.passengers[i] = f->data.passengers[i+1];
	}
	i = f->data.passengerCount - 1;
	while (i > 0 && tmp.SEAT_NUMBER < f->data.passengers[i-1].SEAT_NUMBER) {
		f->data.passengers[i] = f->data.passengers[i-1];
		i--;
	}
	f->data.passengers[i] = tmp;
	printf("Seat for %s changed to %d\n", passengerName, newSeat);
	return 0;
}

void findFlightsByDestination(PlaneNode *head, const char *dest)
{
	PlaneNode *p;
	PlaneNode **arr;
	int cnt, i, j;
	PlaneNode *tmp;

	// Count matching flights 
	cnt = 0;
	for (p = head; p; p = p->next) {
		if (strcmp(p->data.DESTINATION, dest) == 0) {
			cnt++;
		}
	}
	if (!cnt) {
		printf("No flights to %s.\n", dest);
		return;
	}
	// Collect pointers 
	arr = malloc(cnt * sizeof *arr);
	if (!arr) {
		perror("Failed to malloc");
		exit(1);
	}
	memset(arr, 0, cnt * sizeof *arr);
	i = 0;
	for (p = head; p; p = p->next) {
		if (strcmp(p->data.DESTINATION, dest) == 0) {
			arr[i++] = p;
		}
	}
	// Sort by available seats 
	for (i = 0; i < cnt - 1; i++) {
		for (j = i + 1; j < cnt; j++) {
			int ai = arr[i]->data.SEATS - arr[i]->data.passengerCount;
			int aj = arr[j]->data.SEATS - arr[j]->data.passengerCount;
			if (ai > aj) {
				tmp = arr[i];
				arr[i] = arr[j];
				arr[j] = tmp;
			}
		}
	}
	// Print
	for (i = 0; i < cnt; i++) {
		PlaneNode *f = arr[i];
		int avail = f->data.SEATS - f->data.passengerCount;
		printf("  %s at %04d (seats %d, booked %d, avail %d)\n",
				f->data.FLIGHTID,
				f->data.TIME,
				f->data.SEATS,
				f->data.passengerCount,
				avail);
	}
	free(arr);
}

void printFlightsOfPassenger(PlaneNode *head, const char *name)
{
	PlaneNode *p;
	int i, found;
	found = 0;
	//Go through each flight
	for (p = head; p; p = p->next) {
		//Check each passenger for that flight
		for (i = 0; i < p->data.passengerCount; i++) {
			//Compare
			if (strcmp(p->data.passengers[i].NAME, name) == 0) {
				printf("  %s → %s at %04d (seat %d)\n",
						p->data.FLIGHTID,
						p->data.DESTINATION,
						p->data.TIME,
						p->data.passengers[i].SEAT_NUMBER);
				found = 1;
			}
		}
	}
	if (!found) {
		printf("%s is on no flights.\n", name);
	}
}

void printPassengersWithMultipleBookings(PlaneNode *head)
{
	PlaneNode *p;
	PlaneNode *q;
	int i, j, cnt;
	const char *nm;
	
	// For each flight in the list
	for (p = head; p; p = p->next) {
		//Check each passenger on that list
		for (i = 0; i < p->data.passengerCount; i++) {
			nm = p->data.passengers[i].NAME;
			cnt = 0;
			//Check each flight again
			for (q = head; q; q = q->next) {
				//And its passengers
				for (j = 0; j < q->data.passengerCount; j++) {
					//Then compare if they match
					if (strcmp(q->data.passengers[j].NAME, nm) == 0) {
						cnt++;
					}
				}
			}
			//Print
			if (cnt > 1) {
				printf("%s booked on %d flights\n", nm, cnt);
			}
		}
	}
}
//Ensures that there is no memory leak
void freeAllFlights(PlaneNode *head) {
    PlaneNode *p, *next;

    // rewind to the head of the list 
    p = head;
    while (p && p->prev) p = p->prev;

    // traverse and free
    while (p) {
        next = p->next;
		// free passenger array
        free(p->data.passengers);
		// free the node
        free(p);                   
        p = next;
    }
}

