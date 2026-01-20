#include <stdio.h>
#include <stdlib.h>
#include "planes_list.h"

int main(void) {
	PlaneNode *flights = NULL;
	int choice;
	char buf[MAX_DEST_LEN];
	//Prints the menu options
    do {
        printf(
	"0.  Exit\n"
	"1.  Add Flight\n"
	"2.  Add Passenger to Flight\n"
	"3.  Retrieve Flight Details by ID\n"
	"4.  Find Flights by Destination (sorted by avail seats)\n"
	"5.  Delete Flight\n"
	"6.  Change Passenger Seat\n"
	"7.  Print All Flights for a Passenger\n"
	"8.  Print Passengers on Multiple Flights\n"
	"Please enter your choice: ");
		if (scanf("%d", &choice) != 1) {
			while (getchar()!='\n');
			continue;
		}
		//Switch case based on the userinput
		switch (choice) {
			case 1:
				flights = addFlight(flights);
				break;
            case 2:
    			printf("Available flights:\n");
    			printList(flights);
    			printf("Please specify FlightID: ");
    			scanf("%9s", buf);
				addPassengerToFlight(flights, buf);
				break;
			case 3: {
				int num;
				printf("Please specify flight number (1 = first flight): ");
				if (scanf("%d", &num) != 1) {
						while (getchar() != '\n');
							printf("Invalid input.\n");
							break;
						}
    				retrieveFlightByIndex(flights, num);
    				break;
				}
			case 4:
				printf("Please specify Destination:");
				scanf("%49s", buf);
				findFlightsByDestination(flights, buf);
				break;
			case 5:
				printList(flights);
				printf("Please specify FlightID:");
				scanf("%9s", buf);
				flights = deleteFlight(flights, buf);
				break;
			case 6: {
				printList(flights);
				char name[MAX_NAME_LEN];
				int seat;
				printf("Please specify FlightID:");
				scanf("%9s", buf);
				printf("Please specify passenger name:");
				scanf("%49s", name);
				printf("Please specify the seat number:");
				scanf("%d", &seat);
				changePassengerSeat(flights, buf, name, seat);
				break;
			}
            case 7:
				printf("Please specify passenger name:");
				scanf("%49s", buf);
				printFlightsOfPassenger(flights, buf);
				break;
			case 8:
				printPassengersWithMultipleBookings(flights);
				break;
			case 0:
				printf("Goodbye!");
                break;
			default:
				printf("Invalid choice.\n");
		}
	} while (choice != 0);
	//Calls a function that free all from the list to ensure no memory leak
	freeAllFlights(flights);
	return 0;
}
