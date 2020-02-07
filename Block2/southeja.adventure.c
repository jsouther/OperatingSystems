#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

//Define boolean
#define true 1
#define false 0
typedef int bool;

#define NUM_ROOMS_USED 7

struct room {
    char name[8];
    int numOutboundConnections;
    struct room* outboundConnections[6];
    char roomType[11];
};

//Linked list node struct - For tracking visited rooms.
struct node {
    struct node* next;
    struct room* visitedRoom;
};

//Function prototypes
char* FindNewestDir();
struct room* ReadRooms(char* dirName, int numRooms);
struct room* GetRoomByName(char* name, struct room roomArray[]);
struct room* GetStartRoom(struct room roomArray[]);
void PrintPossibleConnections(struct room* room);
struct room* MoveRooms(struct room* room, struct room roomArray[]);
bool IsValidInput(char* input, char* rooms[], int numRooms);
struct node* NewNode(struct room* room);
void CleanUpLinkedList(struct node* node);
void PrintLinkedList(struct node* node);
void* WriteCurrentTime();
void ReadFirstLine(char* fileName);

pthread_t timeKeeper;
pthread_mutex_t timeKeeperLock = PTHREAD_MUTEX_INITIALIZER;

int main() {
    //Lock mutex to main
    pthread_mutex_lock(&timeKeeperLock);

    //Create time keeper thread
    pthread_create(&timeKeeper, NULL, WriteCurrentTime, NULL);

    //Get the name of the newest created rooms directory and read all the data from the files contained within.
    char* newestDirName = FindNewestDir();
    struct room* allRooms = ReadRooms(newestDirName, NUM_ROOMS_USED);
    free(newestDirName);

    //Retrieve the struct for the starting room.
    struct room* currentRoom;
    currentRoom = GetStartRoom(allRooms);

    //Initialize linked list to track rooms visited.
    struct node* head = malloc(sizeof(struct node));
    head->next = NULL;
    head->visitedRoom = NULL;

    struct node* flag = head;
    struct node* nextNode;

    //Counter for the number of steps taken to complete the game.
    int numSteps = 0;
    
    //Loop the game until the END_ROOM is found.
    while  (strcmp(currentRoom->roomType, "END_ROOM") != 0) {
        //Move to a new room
        currentRoom = MoveRooms(currentRoom, allRooms);

        //If the linked list is not empty, add a node at the end with the current room info.
        if (head->visitedRoom != NULL) {
            struct node* nextNode = NewNode(currentRoom);
            flag->next = nextNode;
            flag = nextNode;
        }

        //If the linked list is empty, create the head node.
        else {
            head->visitedRoom = currentRoom;
        }

        numSteps++;
    }

    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps);
    PrintLinkedList(head);

    //Free allocated linkedList memory
    CleanUpLinkedList(head);
    free(allRooms);

    //Cancel the thread if it was never run. Allow it to run to completion so no memory is lost.
    pthread_cancel(timeKeeper);
    pthread_mutex_unlock(&timeKeeperLock);
    pthread_join(timeKeeper, NULL);
    pthread_mutex_destroy(&timeKeeperLock);

    return 0;
}


//Function: Find the name of the newest created directory in the calling directory.
char* FindNewestDir() {
    int newestDirTime = -1;
    char targetDirPrefix[32] = "southeja.rooms.";
    char* newestDirName = malloc(sizeof(char) * 256);
    memset(newestDirName, '\0', sizeof(newestDirName));

    DIR* dirToCheck;
    struct dirent* fileInDir;
    struct stat dirAttributes;

    //Open the current directory
    dirToCheck = opendir(".");

    //If the directory could be opened.
    if (dirToCheck > 0) {
        //Loop through all directories
        while ((fileInDir = readdir(dirToCheck)) != NULL) {
            //If the directory contains the target prefix
            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) {
                //Get the directory stats
                stat(fileInDir->d_name, &dirAttributes);

                //Set the newest time and name to the greater time
                if ((int)dirAttributes.st_mtime > newestDirTime) {
                    newestDirTime = (int)dirAttributes.st_mtime;
                    memset(newestDirName, '\0', sizeof(newestDirName));
                    strcpy(newestDirName, fileInDir->d_name);
                }
            }
        }
    }

    closedir(dirToCheck);

    return newestDirName;
}

//Function: Read room data into a list of structs and return the list. 
//Takes a directory name and number of room files to read as input.
struct room* ReadRooms(char* dirName, int numRooms) {
    //Allocate a list of room* with memory for the number of rooms.
    struct room* rooms = malloc(sizeof(struct room) * numRooms);

    //Initialize the room id and numOutboundConnections.
    int i;
    for (i = 0; i < NUM_ROOMS_USED; i++) {
        rooms[i].numOutboundConnections = 0;
    }

    DIR* dirToOpen;
    struct dirent* fileInDir;
    FILE* fileToRead;
    
    size_t len = 0;
    size_t read;
    char* line = NULL;

    //Copy directory name into filepath string and append '/' so filename can be added later.
    char filePath[256];
    memset(filePath, '\0', sizeof(filePath));
    strcpy(filePath, dirName);
    strcat(filePath, "/");

    dirToOpen = opendir(dirName);

    //Read all the room names into the room list so that later room connections can be read into the correct room struct
    int index = 0;
    //If the directory could be opened
    if (dirToOpen > 0) {
        //while there are files to read
        while ((fileInDir = readdir(dirToOpen)) != NULL) {
            //Skip files starting with '.'
            if (fileInDir->d_name[0] != '.') {
                //Append the filename to the filepath so it can be opened
                strcat(filePath, fileInDir->d_name);
                fileToRead = fopen(filePath, "r");
                
                //If file could not be opened
                if (fileToRead == NULL) {
                    fprintf(stderr, "error reading file\n");
                    exit(1);
                }

                //Get the first line of the file (Room Name)
                read = getline(&line, &len, fileToRead);
                
                char roomName[8];

                //Pull out only the room name from the file and put it into the room list
                sscanf(line, "%*s %*s %s", roomName);
                strcpy(rooms[index].name, roomName);
                
                fclose(fileToRead);
                strcpy(filePath, dirName);
                strcat(filePath, "/");

                index += 1;
            }
        }
    }
    closedir(dirToOpen);


    strcpy(filePath, dirName);
    strcat(filePath, "/");

    dirToOpen = opendir(dirName);

    //Loop through files to get all the connections and room types
    index = 0;
    //if directory could be opened
    if (dirToOpen > 0) {
        //while there are files to read
        while ((fileInDir = readdir(dirToOpen)) != NULL) {
            //Skip files starting with '.'
            if (fileInDir->d_name[0] != '.') {
                strcat(filePath, fileInDir->d_name);
                fileToRead = fopen(filePath, "r");
                
                if (fileToRead == NULL) {
                    fprintf(stderr, "error reading file\n");
                    exit(1);
                }

                //Read the first line which has already been used previously
                read = getline(&line, &len, fileToRead);
                
                char connection[8];
                char roomType[11];
                int connectionCount = 0;
                //While the files has lines to read
                while ((read = getline(&line, &len, fileToRead)) != -1) {
                    //Get the first word of the line
                    sscanf(line, "%s", connection);
                    //If the first word is "CONNECTION"
                    if (strcmp(connection, "CONNECTION") == 0 ) {
                        //Get the name of the connection and add it to the struct of the current room
                        sscanf(line, "%*s %*s %s", connection);
                        rooms[index].outboundConnections[connectionCount] = GetRoomByName(connection, rooms);
                        rooms[index].numOutboundConnections += 1;
                        connectionCount += 1;
                    }
                    //If the first word isn't connection get the roomtype and add it to struct of current room
                    else {
                        sscanf(line, "%s", roomType);
                        strcpy(rooms[index].roomType, roomType);
                    }
                }
                
                fclose(fileToRead);
                strcpy(filePath, dirName);
                strcat(filePath, "/");

                index += 1;
            }
        }
    }

    free(line);
    closedir(dirToOpen);

    return rooms;
}

//Function: get a room struct pointer by the rooms name. Takes a name string and room list as input.
struct room* GetRoomByName(char* name, struct room roomArray[]) {
    int i;
    for (i = 0; i < NUM_ROOMS_USED; i++) {
        if (strcmp(name, roomArray[i].name) == 0) {
            return &roomArray[i];
        }
    }

    //if a room isn't found, error
    fprintf(stderr, "Room struct not found!\n");
    exit(1);
}

//Function: Get the start room pointer. Takes a room list as input.
struct room* GetStartRoom(struct room roomArray[]) {
    int i;
    for (i = 0; i < NUM_ROOMS_USED; i++) {
        if (strcmp(roomArray[i].roomType, "START_ROOM") == 0) {
            return &roomArray[i];
        }
    }

    //Error if a start room is not found.
    fprintf(stderr, "Start room not found!\n");
    exit(1);
}

//Function: Print the possible connections to a given room.
void PrintPossibleConnections(struct room* room) {
    printf("POSSIBLE CONNECTIONS: ");

    int i;
    for (i = 0; i < room->numOutboundConnections; i++) {
        if (i < room->numOutboundConnections - 1) {
            printf("%s, ", room->outboundConnections[i]->name);
        }
        else {
            printf("%s.\n", room->outboundConnections[i]->name);
        }
    }
}

//Function: Move the player from one room to the next.
//Takes the current room and list of all rooms as input.
struct room* MoveRooms(struct room* room, struct room roomArray[]) {
    char* validRooms[room->numOutboundConnections];

    //Get the names of all rooms that connect to current room
    int i;
    for (i = 0; i < room->numOutboundConnections; i++) {
        validRooms[i] = room->outboundConnections[i]->name;
    }
    
    size_t userInput;
    size_t len = 0;
    char* enteredLine = NULL;

    //Loop until the user enters a valid room name or asks for the time
    do {
        printf("CURRENT LOCATION: %s\n", room->name);
        printf("POSSIBLE CONNECTIONS: ");
        for (i = 0; i < room->numOutboundConnections; i++) {
            if (i < room->numOutboundConnections - 1) {
                printf("%s, ", room->outboundConnections[i]->name);
            }
            else {
                printf("%s.\n", room->outboundConnections[i]->name);
            }
        }
        printf("WHERE TO? >");
        userInput = getline(&enteredLine, &len, stdin);
        sscanf(enteredLine, "%s", enteredLine); //remove newline character or remove space and all chars after
        
        //If user calls time allow the timeKeeper thread to run
        if (strcmp(enteredLine, "time") == 0) {
            pthread_mutex_unlock(&timeKeeperLock);
            pthread_join(timeKeeper, NULL);
            pthread_mutex_lock(&timeKeeperLock);

            pthread_create(&timeKeeper, NULL, WriteCurrentTime, NULL);

            ReadFirstLine("currentTime.txt");
        }

    } while (IsValidInput(enteredLine, validRooms, room->numOutboundConnections) != true);

    printf("\n");

    //Store the getLine user input into a variable to avoid memory loss when the line is returned at end of function
    char storeLine[8];
    memset(storeLine, '\0', sizeof(storeLine));
    strcpy(storeLine, enteredLine);

    free(enteredLine);

    return GetRoomByName(storeLine, roomArray);

}

//Function: Check if input string matches a list of room names.
bool IsValidInput(char* input, char* rooms[], int numRooms) {
    int i;
    for (i = 0; i < numRooms; i++) {
        if (strcmp(input, rooms[i]) == 0) {
            return true;
        }
        
        if (strcmp(input, "time") == 0) {
            return false;
        }
    }

    printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
    return false;
}

//Function: Add a node for the given room to the end of the linkedList
struct node* NewNode(struct room* room) {
    struct node* node = malloc(sizeof(struct node));
    node->visitedRoom = room;
    node->next = NULL;

    return node;
}

//Function: Sequentially print names stored in linkedlist
void PrintLinkedList(struct node* node) {
    while (node != NULL) {
        printf("%s\n", node->visitedRoom->name);
        node = node->next;
    }
}

//Function: Deallocate memory used by the linkedlist
void CleanUpLinkedList(struct node* node) {
    
    while (node->next != NULL) {
        struct node* temp = node;
        node = temp->next;
        free(temp);
    }

    free(node);
}

//Function: Write current time to a txt file.
void* WriteCurrentTime() {
    //lock mutex so thread is runnable.
    pthread_mutex_lock(&timeKeeperLock);

    //test if the thread has been canceled, so rest of function does not execute
    pthread_testcancel();

    char outstr[200];
    time_t t;
    struct tm* temp;
    char* format = "%l:%M%P, %A, %B %d, %Y";

    t = time(NULL);
    temp = localtime(&t);
    strftime(outstr, sizeof(outstr), format, temp);

    char fileName[16];
    memset(fileName, '\0', sizeof(fileName));
    strcpy(fileName, "currentTime.txt");

    FILE* fileToWrite;
    fileToWrite = fopen(fileName, "w");
    fprintf(fileToWrite, outstr);
    fclose(fileToWrite);

    pthread_mutex_unlock(&timeKeeperLock);

    return NULL;
}

//Function: Read the first line of given file.
void ReadFirstLine(char* fileName) {
    FILE* fileToRead;
    fileToRead = fopen(fileName, "r");

    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    nread = getline(&line, &len, fileToRead);

    printf("\n%s\n\n", line);

    free(line);
    fclose(fileToRead);
}