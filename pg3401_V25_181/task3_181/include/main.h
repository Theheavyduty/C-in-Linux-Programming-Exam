#ifndef MAIN_H
#define MAIN_H

#define MAX_NAME_LEN       50
#define MAX_FLIGHTID_LEN   10
#define MAX_DEST_LEN       64
#define MAX_PASSENGERS    100


struct PASSENGERS {
    int  SEAT_NUMBER;
    char NAME[MAX_NAME_LEN];
    int  AGE;
};


struct Planes {
    char FLIGHTID[MAX_FLIGHTID_LEN];
    char DESTINATION[MAX_DEST_LEN];
    int  SEATS;                    /* total seats on plane   */
    int  TIME;                     /* e.g. departure time    */
    int  passengerCount;           /* how many in array now  */
    struct PASSENGERS passengers[MAX_PASSENGERS];
};

#endif /* MAIN_H */
