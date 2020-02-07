#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

//Define boolean
#define true 1
#define false 0
typedef int bool;


struct room {
    int id;
    char name[8];
    int numOutboundConnections;
    struct room* outboundConnections[6];
    bool hasType;
    char roomType[11];
};

#define NUM_ROOMS 10
#define NUM_ROOMS_USED 7

//Function prototypes
bool IsGraphFull(struct room ray[]);
void AddRandomConnection(struct room roomArray[]);
struct room* GetRandomRoom(struct room roomArray[]);
bool CanAddConnectionFrom(struct room* x);
bool ConnectionAlreadyExists(struct room* x, struct room* y);
void ConnectRoom(struct room* x, struct room* y);
bool IsSameRoom(struct room* x, struct room* y);
void GenerateRoomFile(struct room* someRoom, char* dirName);

int main(int argc) {
    //Seed rand
    srand(time(NULL));

    //If southeja.buildrooms is called with arguments, exit.
    if (argc > 1) {
    	fprintf(stderr, "Error: Function takes no arguments!\n");
	exit(1);
    }

    //Retrieve the process id
    int pid = getpid();
    char mypid[6];
    memset(mypid, '\0', 6);
    sprintf(mypid, "%d", pid);


    char dirName[21];
    memset(dirName, '\0', 21);
    sprintf(dirName, "southeja.rooms.%s", mypid);

    //Create the directory
    int result = mkdir(dirName, 0755);
    if (result != 0) {
        fprintf(stderr, "unable to create directory");
        exit(1);
    }

    //Create a list of room names
    char* roomNames[NUM_ROOMS];
    roomNames[0] = "Nuxvar";
    roomNames[1] = "Norton";
    roomNames[2] = "Stanlow";
    roomNames[3] = "Dalelry";
    roomNames[4] = "OldHam";
    roomNames[5] = "Lullin";
    roomNames[6] = "Malrton";
    roomNames[7] = "Padstow";
    roomNames[8] = "Solime";
    roomNames[9] = "Cromer";

    //Create a list of rooms
    struct room allRooms[NUM_ROOMS];
    
    //Initialize values for rooms in list
    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        allRooms[i].id = i;
        strcpy(allRooms[i].name, roomNames[i]);
        allRooms[i].numOutboundConnections = 0;
	    allRooms[i].hasType = false;
    }

    int roomsRemaining = NUM_ROOMS;
    struct room chosenRooms[NUM_ROOMS_USED];
    struct room* chosenRoom;

    //Randomly choose the 7 rooms to be created
    for (i = 0; i < NUM_ROOMS_USED; i++) {
        int randomPosition = rand() % roomsRemaining;
        chosenRoom = &allRooms[randomPosition];

        chosenRooms[i] = allRooms[randomPosition];

        //For the chosen index, copy each position one to the left so the chosen index is effectively deleted.
        int y;
        for (y = randomPosition; y < NUM_ROOMS - 1; y++) {
            allRooms[y] = allRooms[y+1];
        }
        roomsRemaining--;
    }

    //Randomly assign start room
    int randomStartRoomIndex = rand() % NUM_ROOMS_USED;
    strcpy(chosenRooms[randomStartRoomIndex].roomType, "START_ROOM");
    chosenRooms[randomStartRoomIndex].hasType = true;

    //Randomly assign end room and then the rest are assigned as mid room
    int randomEndRoomIndex = rand() % NUM_ROOMS_USED;
    while (randomStartRoomIndex == randomEndRoomIndex) {
    	randomEndRoomIndex = rand () % NUM_ROOMS_USED;
    }
    strcpy(chosenRooms[randomEndRoomIndex].roomType, "END_ROOM");
    chosenRooms[randomEndRoomIndex].hasType = true;

    for (i = 0; i < NUM_ROOMS_USED; i++) {
    	if (chosenRooms[i].hasType == false) {
		    strcpy(chosenRooms[i].roomType, "MID_ROOM");
		    chosenRooms[i].hasType = true;
	    }
    }

    // Create all connections in graph
    while (IsGraphFull(chosenRooms) == false) {
        AddRandomConnection(chosenRooms);
    }

    //Generate the file for each room
    for (i = 0; i < NUM_ROOMS_USED; i++) {
    	GenerateRoomFile(&chosenRooms[i], dirName);
    }

    return 0;
}

//Function: Returns true if all rooms have 3 to 6 outbound connections, false otherwise.
bool IsGraphFull(struct room roomArray[]) {
    int i;
    for (i = 0; i < NUM_ROOMS_USED; i++) {
        if (roomArray[i].numOutboundConnections < 3 || roomArray[i].numOutboundConnections > 6) {
            return false;
        }
    }
    return true;
}

//Function: Adds a random, valid outbound connection from a Room to another Room.
void AddRandomConnection(struct room roomArray[]) {
    struct room* A;
    struct room* B;

    //Get rooms until a connection can be made.
    while(true)
    {
        A = GetRandomRoom(roomArray);

        if (CanAddConnectionFrom(A) == true)
        break;
    }

    //Get a 2nd room which can make a connection, isn't the same as A, and doesn't already connect to A.
    do
    {
        B = GetRandomRoom(roomArray);
    }
    while(CanAddConnectionFrom(B) == false || IsSameRoom(A, B) == true || ConnectionAlreadyExists(A, B) == true);

    //Connect A to B and B to A
    ConnectRoom(A, B);
    ConnectRoom(B, A);
}

//Function: Returns a random Room, does NOT validate if connection can be added.
struct room* GetRandomRoom(struct room roomArray[]) {
    int randomRoomID = rand() % NUM_ROOMS_USED;
    return &roomArray[randomRoomID];
}

//Function: Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
bool CanAddConnectionFrom(struct room* x) {
    return x->numOutboundConnections < 6;
}

//Function: Returns true if a connection from Room x to Room y already exists, false otherwise
bool ConnectionAlreadyExists(struct room* x, struct room* y) {
    int i;
    for (i = 0; i < x->numOutboundConnections; i++) {
        if (x->outboundConnections[i] == y) {
            return true;
        }
    }

    return false;
}

//Function: Connects Rooms x and y together, does not check if this connection is valid
void ConnectRoom(struct room* x, struct room* y) {
    x->outboundConnections[x->numOutboundConnections] = y;
    x->numOutboundConnections++;
}

//Function: Returns true if Rooms x and y are the same Room, false otherwise
bool IsSameRoom(struct room* x, struct room* y) {   
    return x->id == y->id;
}

//Function: Create a room file
void GenerateRoomFile(struct room* someRoom, char* dirName) {
	char path[50];
	memset(path, '\0', 50);
	sprintf(path, "%s/%s", dirName, someRoom->name);

    //Print room name to file
	FILE* newFile;
	newFile = fopen(path, "w");
	fprintf(newFile, "ROOM NAME: %s\n", someRoom->name);
	
    //Print connections to file
	int i;
	for (i = 0; i < someRoom->numOutboundConnections; i++) {
		fprintf(newFile, "CONNECTION %d: %s\n", i + 1, someRoom->outboundConnections[i]->name);
	}

    //Print room type to file
	fprintf(newFile, "%s\n", someRoom->roomType);
	fclose(newFile);
}