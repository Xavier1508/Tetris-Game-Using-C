#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <windows.h>

// Constants
#define MAX_USERNAME 21
#define MIN_USERNAME 4
#define MAX_PASSWORD 25
#define MIN_PASSWORD 8
#define HASH_SIZE 27
#define MAX_PLAYERS_PER_PAGE 17
#define BOX_WIDTH 28
#define BOX_HEIGHT 19
#define BOX_OFFSET_X 15
#define BOX_OFFSET_Y 2
#define BLOCK_SIZE 6
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Data structures
typedef struct User {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int score;
    int clearedRow;
    int totalBlock;
    struct User* next;
} User;

typedef struct Block {
    char shape[BLOCK_SIZE][BLOCK_SIZE];
    int width;
    int height;
    int area;
    struct Block* next;
    struct Block* prev;
} Block;

typedef struct NextBlock {
    Block* block;
    struct NextBlock* next;
} NextBlock;

// Global variables
bool isPaused = false;
int fallSpeed = 500;
char screenBuffer[BOX_HEIGHT][BOX_WIDTH];
int score = 0;
int level = 1;
User* hashTable[HASH_SIZE];
Block* blockList = NULL;
NextBlock* nextBlockList = NULL;
char gameBox[BOX_HEIGHT][BOX_WIDTH];
int currentPlayerScore = 0;
int currentClearedRow = 0;
int currentPlacedBlock = 0;
User currentUser;
bool sortAscending = true;
bool isValidPosition(int col, int row, Block* block);
void placeBlock(int row, int col);
bool checkGameOver(void);
bool shadowBuffer[BOX_HEIGHT][BOX_WIDTH] = {0};
Block* currentBlock = NULL;
int currentRow = 0;
int currentCol = 0;
int prevRow = -1, prevCol = -1;
Block* prevBlock = NULL;

// Function prototypes
void displayPausedMessage(void);
void clearScreen();
void gotoxy(int x, int y);
void displayTitle();
void homePage();
void playGame();
void viewPlayers();
void adminPage();
void exitProgram();
Block* createDefaultBlock();
int hashFunction(char username[]);
void loadUsers();
void saveUsers();
void loadBlocks();
void saveBlocks();
void insertUser(User* newUser);
User* findUser(char username[]);
void loginOrRegister(char username[]);
bool validateUsername(char username[]);
bool validatePassword(char password[], bool isRegistered);
void displayPasswordRequirements(char password[]);
void initializeGame();
void displayGamePage();
void gameLoop();
void gameOverMenu();
void addNewBlock();
void removeBlock();
void rotateBlock(Block* block, int currentCol, int currentRow);
void clearRow();
Block* createRandomBlock();
NextBlock* createNextBlockList();
void addBlockToNextList();
void freeResources();
void moveToPosition(int x, int y);
bool initializeGameBox(void);
void freeBlock(Block* block);
void updateShadowBlock(void);
void moveBlockLeft(void);
void moveBlockRight(void);
void moveBlockDown(void);
void hardDrop(void);
void checkCompletedLines(void);
void checkAndClearLines(void);
void resetCursor();
void increaseDifficulty();
void renderShadow();
void resetTerminal(void);
void cleanupGameResources(void);
void updateScore(int linesCleared);

void clearScreen() {
    system("cls");
}

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void displayTitle() {
    printf("___  _________     _____  _____  _____  _____  _____  _____ \n");
    printf("|  \\/  || ___ \\   |_   _||  ___||_   _|| ___ \\|_   _|/  ___|\n");
    printf("| .  . || |_/ /     | |  | |__    | |  | |_/ /  | |  \\ `--. \n");
    printf("| |\\/| ||    /      | |  |  __|   | |  |    /   | |   `--. \\\n");
    printf("| |  | || |\\ \\  _   | |  | |___   | |  | |\\ \\  _| |_ /\\__/ /\n");
    printf("\\_|  |_/\\_| \\_|(_)  \\_/  \\____/   \\_/  \\_| \\_| \\___/ \\____/\n");
}

int hashFunction(char username[]) {
    if (username[0] == '\0') return 0;
    
    char firstChar = tolower(username[0]);
    
    if (firstChar >= 'a' && firstChar <= 'z') {
        return firstChar - 'a';
    } else {
        return 26;
    }
}

void insertUser(User* newUser) {
    int index = hashFunction(newUser->username);
    
    if (hashTable[index] == NULL) {
        hashTable[index] = newUser;
        newUser->next = NULL;
        return;
    }
    
    if (newUser->score > hashTable[index]->score) {
        newUser->next = hashTable[index];
        hashTable[index] = newUser;
        return;
    }
    
    User* current = hashTable[index];
    while (current->next != NULL && current->next->score >= newUser->score) {
        current = current->next;
    }
    
    newUser->next = current->next;
    current->next = newUser;
}

User* findUser(char username[]) {
    int index = hashFunction(username);
    User* current = hashTable[index];
    
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

void saveUsers() {
    FILE* file = fopen("user.txt", "w");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    
    char** writtenUsernames = (char**)malloc(sizeof(char*) * 100);
    if (writtenUsernames == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return;
    }
    
    int userCount = 0;
    
    for (int i = 0; i < HASH_SIZE; i++) {
        User* current = hashTable[i];
        while (current != NULL) {
            bool alreadyWritten = false;
            for (int j = 0; j < userCount; j++) {
                if (writtenUsernames[j] != NULL && strcmp(current->username, writtenUsernames[j]) == 0) {
                    alreadyWritten = true;
                    break;
                }
            }
            
            if (!alreadyWritten) {
                fprintf(file, "%s,%s,%d,%d,%d\n", 
                        current->username, 
                        current->password, 
                        current->score, 
                        current->clearedRow, 
                        current->totalBlock);
                
                writtenUsernames[userCount] = (char*)malloc(strlen(current->username) + 1);
                if (writtenUsernames[userCount] != NULL) {
                    strcpy(writtenUsernames[userCount], current->username);
                    userCount++;
                }
            }
            current = current->next;
        }
    }
    
    for (int i = 0; i < userCount; i++) {
        free(writtenUsernames[i]);
    }
    free(writtenUsernames);
    
    fclose(file);
}

void loadUsers() {
    for (int i = 0; i < HASH_SIZE; i++) {
        User* current = hashTable[i];
        while (current != NULL) {
            User* temp = current;
            current = current->next;
            free(temp);
        }
        hashTable[i] = NULL;
    }
    
    FILE* file = fopen("user.txt", "r");
    if (file == NULL) {
        file = fopen("user.txt", "w");
        if (file != NULL) {
            fclose(file);
        }
        return;
    }
    
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) {
            continue;
        }
        
        User* newUser = (User*)malloc(sizeof(User));
        if (newUser == NULL) {
            printf("Memory allocation failed\n");
            fclose(file);
            exit(1);
        }
        
        strcpy(newUser->username, "");
        strcpy(newUser->password, "");
        newUser->score = 0;
        newUser->clearedRow = 0;
        newUser->totalBlock = 0;
        newUser->next = NULL;
        
        char* token = strtok(line, ",");
        if (token != NULL) {
            strcpy(newUser->username, token);
            
            token = strtok(NULL, ",");
            if (token != NULL) {
                strcpy(newUser->password, token);
                
                token = strtok(NULL, ",");
                if (token != NULL) {
                    newUser->score = atoi(token);
                    
                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        newUser->clearedRow = atoi(token);
                        
                        token = strtok(NULL, ",");
                        if (token != NULL) {
                            newUser->totalBlock = atoi(token);
                        }
                    }
                }
            }
        }
        
        if (strlen(newUser->username) > 0) {
            if (findUser(newUser->username) == NULL) {
                insertUser(newUser);
            } else {
                free(newUser);
            }
        } else {
            free(newUser);
        }
    }
    fclose(file);
}


void loadBlocks() {
    FILE* file = fopen("block.txt", "r");
    if (file == NULL) {
        printf("Error: Cannot open block.txt file\n");
        return;
    }
    
    char line[100];
    Block* currentBlock = NULL;
    int row = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; 
        
        if (strcmp(line, "=") == 0) {
            if (currentBlock != NULL) {
                int minX = BLOCK_SIZE, maxX = 0, minY = BLOCK_SIZE, maxY = 0;
                int blockArea = 0;
                
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    for (int j = 0; j < BLOCK_SIZE; j++) {
                        if (currentBlock->shape[i][j] == '#') {
                            blockArea++;
                            if (j < minX) minX = j;
                            if (j > maxX) maxX = j;
                            if (i < minY) minY = i;
                            if (i > maxY) maxY = i;
                        }
                    }
                }
                
                currentBlock->width = maxX - minX + 1;
                currentBlock->height = maxY - minY + 1;
                currentBlock->area = blockArea;
                
                if (blockList == NULL) {
                    blockList = currentBlock;
                    currentBlock->next = NULL;
                    currentBlock->prev = NULL;
                } else {
                    Block* last = blockList;
                    while (last->next != NULL) {
                        last = last->next;
                    }
                    last->next = currentBlock;
                    currentBlock->prev = last;
                    currentBlock->next = NULL;
                }
            }
            
            currentBlock = (Block*)malloc(sizeof(Block));
            if (currentBlock == NULL) {
                printf("Memory allocation failed\n");
                exit(1);
            }
            
            for (int i = 0; i < BLOCK_SIZE; i++) {
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    currentBlock->shape[i][j] = '.';
                }
            }
            
            row = 0;
        } else {
            if (currentBlock != NULL && row < BLOCK_SIZE) {
                size_t lineLength = strlen(line);
                for (int j = 0; j < BLOCK_SIZE && j < (int)lineLength; j++) {
                    currentBlock->shape[row][j] = line[j];
                }
                row++;
            }
        }
    }
    
    if (currentBlock != NULL) {
        int minX = BLOCK_SIZE, maxX = 0, minY = BLOCK_SIZE, maxY = 0;
        int blockArea = 0;
        
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (currentBlock->shape[i][j] == '#') {
                    blockArea++;
                    if (j < minX) minX = j;
                    if (j > maxX) maxX = j;
                    if (i < minY) minY = i;
                    if (i > maxY) maxY = i;
                }
            }
        }
        
        currentBlock->width = maxX - minX + 1;
        currentBlock->height = maxY - minY + 1;
        currentBlock->area = blockArea;
        
        if (blockList == NULL) {
            blockList = currentBlock;
            currentBlock->next = NULL;
            currentBlock->prev = NULL;
        } else {
            Block* last = blockList;
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = currentBlock;
            currentBlock->prev = last;
            currentBlock->next = NULL;
        }
    }
    
    fclose(file);
}

void saveBlocks() {
    FILE* file = fopen("block.txt", "w");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }
    
    Block* current = blockList;
    while (current != NULL) {
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                fprintf(file, "%c", current->shape[i][j]);
            }
            fprintf(file, "\n");
        }
        
        if (current->next != NULL) {
            fprintf(file, "=\n");
        } else {
            fprintf(file, "=");
        }
        
        current = current->next;
    }
    
    fclose(file);
}

bool validateUsername(char username[]) {
    int length = strlen(username);
    if (length < MIN_USERNAME || length > MAX_USERNAME - 1) {
        printf("Error: Username must be between %d-%d characters long\n", MIN_USERNAME, MAX_USERNAME - 1);
        return false;
    }
    
    for (int i = 0; i < length; i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || 
              (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || 
              c == '.' || c == '-' || c == '_')) {
            printf("Error: Username can only contain alphanumeric characters, '.', '-', and '_'\n");
            return false;
        }
    }
    
    return true;
}

bool validatePassword(char password[], bool isRegistered) {
    if (isRegistered) {
        if (strcmp(password, currentUser.password) != 0) {
            printf("Error: Incorrect password!\n");
            return false;
        }
        return true;
    } else {
        int length = strlen(password);
        if (length < MIN_PASSWORD || length > MAX_PASSWORD - 1) {
            printf("Error: Password must be between %d-%d characters long\n", MIN_PASSWORD, MAX_PASSWORD - 1);
            return false;
        }
        
        bool hasUpper = false;
        bool hasLower = false;
        bool hasSymbolOrNumber = false;
        
        for (int i = 0; i < length; i++) {
            char c = password[i];
            if (c >= 'A' && c <= 'Z') {
                hasUpper = true;
            } else if (c >= 'a' && c <= 'z') {
                hasLower = true;
            } else if ((c >= '0' && c <= '9') || 
                       (c >= 33 && c <= 126 && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z'))) {
                hasSymbolOrNumber = true;
            }
        }
        
        if (!hasUpper) {
            printf("Error: Password must contain at least one uppercase letter\n");
            return false;
        }
        if (!hasLower) {
            printf("Error: Password must contain at least one lowercase letter\n");
            return false;
        }
        if (!hasSymbolOrNumber) {
            printf("Error: Password must contain at least one number or symbol\n");
            return false;
        }
        
        return true;
    }
}

void displayPasswordRequirements(char password[]) {
    int length = strlen(password);
    bool hasUppercase = false;
    bool hasLowercase = false;
    bool hasSymbolOrNumber = false;
    
    for (int i = 0; i < length; i++) {
        char c = password[i];
        if (c >= 'A' && c <= 'Z') {
            hasUppercase = true;
        } else if (c >= 'a' && c <= 'z') {
            hasLowercase = true;
        } else if ((c >= '0' && c <= '9') || 
                   (c >= 33 && c <= 126 && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z'))) {
            hasSymbolOrNumber = true;
        }
    }
    
    printf("Password Requirements:\n");
    printf("%c Has length between 8-24\n", (length >= MIN_PASSWORD && length <= MAX_PASSWORD - 1) ? 'v' : ' ');
    printf("%c Contains uppercase characters\n", hasUppercase ? 'v' : ' ');
    printf("%c Contains lowercase characters\n", hasLowercase ? 'v' : ' ');
    printf("%c Contains symbols or numeric characters\n", hasSymbolOrNumber ? 'v' : ' ');
}

void loginOrRegister(char username[]) {
    loadUsers();
    
    User* existingUser = findUser(username);
    char password[MAX_PASSWORD] = {0};
    char confirmPassword[MAX_PASSWORD] = {0};
    char ch;
    int i;
    
    if (existingUser != NULL) {
        clearScreen();
        displayTitle();
        printf("User '%s' found! Do you want to login? (y/n): ", username);
        ch = getch();
        printf("%c\n", ch);
        fflush(stdout);
        
        if (ch != 'y') {
            homePage();
            return;
        }

        printf("Password: (Press ESC to go back)\n");
        printf("-----------------------------------\n");
        fflush(stdout);
        i = 0;
        while (1) {
            ch = getch();
            
            if (ch == 27) {
                homePage();
                return;
            } else if (ch == 13) {
                password[i] = '\0';
                break;
            } else if (ch == 8 || ch == 127) {
                if (i > 0) {
                    i--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else if (ch == 3) {
                printf("\nExiting program...\n");
                exit(0);
            } else if (i < MAX_PASSWORD - 1 && ch >= 32 && ch <= 126) {
                password[i++] = ch;
                printf("*");
                fflush(stdout);
            }
        }
        printf("\n-----------------------------------\n");
        fflush(stdout);

        if (strcmp(existingUser->password, password) == 0) {
            strcpy(currentUser.username, existingUser->username);
            strcpy(currentUser.password, existingUser->password);
            currentUser.score = existingUser->score;
            currentUser.clearedRow = existingUser->clearedRow;
            currentUser.totalBlock = existingUser->totalBlock;
            
            if (strcmp(username, "Admin") == 0) {
                adminPage();
            } else {
                playGame();
            }
        } else {
            printf("Error: Wrong password!\n");
            printf("Press any key to continue...");
            fflush(stdout);
            getch();
            homePage();
        }
    } else {
        clearScreen();
        displayTitle();
        printf("User '%s' not found! Do you want to register? (y/n): ", username);
        ch = getch();
        printf("%c\n", ch);
        fflush(stdout);

        if (ch != 'y') {
            homePage();
            return;
        }

        bool hasLength = false;
        bool hasUpper = false;
        bool hasLower = false;
        bool hasSymbolOrNumber = false;
        
        memset(password, 0, MAX_PASSWORD);
        
        while (1) {
            clearScreen();
            displayTitle();
            
            printf("Password: (Press ESC to go back)\n");
            printf("-----------------------------------\n");
            for (size_t j = 0; j < strlen(password); j++) {
                printf("*");
            }
            printf("\n-----------------------------------\n");
            
            printf("[%c] Length 8-24 (inclusive)\n", hasLength ? 'v' : ' ');
            printf("[%c] contains upper case character\n", hasUpper ? 'v' : ' ');
            printf("[%c] contains lower case character\n", hasLower ? 'v' : ' ');
            printf("[%c] contains symbol or numeric character\n", hasSymbolOrNumber ? 'v' : ' ');
            fflush(stdout);
            
            i = strlen(password);
            while (1) {
                ch = getch();
                

                if (ch == 27) {
                    homePage();
                    return;
                } else if (ch == 13) {
                    if (i == 0) continue;
                    password[i] = '\0';
                    break;
                } else if (ch == 8 || ch == 127) {
                    if (i > 0) {
                        i--;
                        password[i] = '\0';
                        
                        hasLength = (i >= 8 && i <= 24);
                        hasUpper = hasLower = hasSymbolOrNumber = false;
                        for (int j = 0; j < i; j++) {
                            if (isupper(password[j])) hasUpper = true;
                            if (islower(password[j])) hasLower = true;
                            if (isdigit(password[j]) || ispunct(password[j])) hasSymbolOrNumber = true;
                        }
                        
                        clearScreen();
                        displayTitle();
                        printf("Password: (Press ESC to go back)\n");
                        printf("-----------------------------------\n");
                        for (int j = 0; j < i; j++) {
                            printf("*");
                        }
                        printf("\n-----------------------------------\n");
                        printf("[%c] Length 8-24\n", hasLength ? 'v' : ' ');
                        printf("[%c] Contains uppercase\n", hasUpper ? 'v' : ' ');
                        printf("[%c] Contains lowercase\n", hasLower ? 'v' : ' ');
                        printf("[%c] Contains symbol/number\n", hasSymbolOrNumber ? 'v' : ' ');
                        fflush(stdout);
                    }
                } else if (ch == 3) {
                    printf("\nExiting program...\n");
                    exit(0);
                } else if (i < MAX_PASSWORD - 1 && ch >= 32 && ch <= 126) {
                    password[i++] = ch;
                    
                    hasLength = (i >= 8 && i <= 24);
                    
                    hasUpper = false;
                    hasLower = false;
                    hasSymbolOrNumber = false;
                    

                    for (int j = 0; j < i; j++) {
                        if (isupper(password[j])) hasUpper = true;
                        if (islower(password[j])) hasLower = true;
                        if (isdigit(password[j]) || ispunct(password[j])) hasSymbolOrNumber = true;
                    }
                    
                    clearScreen();
                    displayTitle();
                    printf("Password: (Press ESC to go back)\n");
                    printf("-----------------------------------\n");
                    for (int j = 0; j < i; j++) {
                        printf("*");
                    }
                    printf("\n-----------------------------------\n");
                    
                    printf("[%c] Length 8-24 (inclusive)\n", hasLength ? 'v' : ' ');
                    printf("[%c] contains upper case character\n", hasUpper ? 'v' : ' ');
                    printf("[%c] contains lower case character\n", hasLower ? 'v' : ' ');
                    printf("[%c] contains symbol or numeric character\n", hasSymbolOrNumber ? 'v' : ' ');
                    fflush(stdout);
                }
            }
            
            if (hasLength && hasUpper && hasLower && hasSymbolOrNumber) {
                break;
            } else {
                printf("\nPassword does not meet all requirements.\n");
                printf("Press any key to try again...");
                fflush(stdout);
                getch();
                memset(password, 0, MAX_PASSWORD);
            }
        }

        clearScreen();
        displayTitle();
        printf("Confirm password: (Press ESC to go back)\n");
        printf("-----------------------------------\n");
        printf("\n-----------------------------------\n");
        printf("Please re-enter your password to confirm\n");
        fflush(stdout);
        
        i = 0;
        while (1) {
            ch = getch();
            
            if (ch == 27) { 
                homePage();
                return;
            } else if (ch == 13) {
                if (i == 0) continue;
                confirmPassword[i] = '\0';
                break;
            } else if (ch == 8 || ch == 127) {
                if (i > 0) {
                    i--;
                    confirmPassword[i] = '\0';
                    clearScreen();
                    displayTitle();
                    printf("Confirm password: (Press ESC to go back)\n");
                    printf("-----------------------------------\n");
                    for (int j = 0; j < i; j++) {
                        printf("*");
                    }
                    printf("\n-----------------------------------\n");
                    printf("Please re-enter your password to confirm\n");
                    fflush(stdout);
                }
            } else if (ch == 3) {
                printf("\nExiting program...\n");
                exit(0);
            } else if (i < MAX_PASSWORD - 1 && ch >= 32 && ch <= 126) {
                confirmPassword[i++] = ch;
                clearScreen();
                displayTitle();
                printf("Confirm password: (Press ESC to go back)\n");
                printf("-----------------------------------\n");
                for (int j = 0; j < i; j++) {
                    printf("*");
                }
                printf("\n-----------------------------------\n");
                printf("Please re-enter your password to confirm\n");
                fflush(stdout);
            }
        }
        
        if (strcmp(password, confirmPassword) != 0) {
            printf("\nError: Passwords do not match!\n");
            printf("Press any key to continue...");
            fflush(stdout);
            getch();
            homePage();
            return;
        }

        User* newUser = (User*)malloc(sizeof(User));
        if (newUser == NULL) {
            printf("Memory allocation failed\n");
            exit(1);
        }

        strcpy(newUser->username, username);
        strcpy(newUser->password, password);
        newUser->score = 0;
        newUser->clearedRow = 0;
        newUser->totalBlock = 0;
        newUser->next = NULL;

        insertUser(newUser);
        saveUsers();

        strcpy(currentUser.username, newUser->username);
        strcpy(currentUser.password, newUser->password);
        currentUser.score = newUser->score;
        currentUser.clearedRow = newUser->clearedRow;
        currentUser.totalBlock = newUser->totalBlock;

        printf("Registration successful!\n");
        printf("Press any key to continue...");
        fflush(stdout);
        getch();
        playGame();
    }
}

int getMilliseconds() {
    return (int)clock();
}

void initializeGame() {
    memset(gameBox, ' ', sizeof(gameBox));
    memset(shadowBuffer, 0, sizeof(shadowBuffer));
    
    currentPlayerScore = 0;
    currentClearedRow = 0;
    currentPlacedBlock = 0;
    level = 1;
    fallSpeed = 500;
    
    if (!blockList) loadBlocks();
    
    if (!currentBlock) {
        currentBlock = createDefaultBlock();
        if (!currentBlock) {
            fprintf(stderr, "Fatal error: Could not create initial block\n");
            exit(1);
        }
    }
    
    currentRow = 0;
    currentCol = (BOX_WIDTH - currentBlock->width) / 2;
    
    if (nextBlockList != NULL) {
        NextBlock* current = nextBlockList->next;
        while (current != nextBlockList) {
            NextBlock* temp = current;
            current = current->next;
            free(temp->block);
            free(temp);
        }
        free(nextBlockList->block);
        free(nextBlockList);
    }
    
    nextBlockList = createNextBlockList();
    if (!nextBlockList) {
        fprintf(stderr, "Fatal error: Could not create next block list\n");
        exit(1);
    }
    
    srand((unsigned int)time(NULL));
    
    isPaused = false;
}

Block* createDefaultBlock() {
    Block* block = (Block*)malloc(sizeof(Block));
    if (!block) return NULL;

    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block->shape[i][j] = '.';
        }
    }

    block->shape[1][2] = '#';
    block->shape[1][3] = '#';
    block->shape[1][4] = '#';
    block->shape[2][2] = '#';
    block->shape[2][3] = '#';
    block->shape[2][4] = '#';
    block->shape[3][2] = '#';
    block->shape[3][3] = '#';
    block->shape[3][4] = '#';
    block->shape[4][2] = '#';
    block->shape[4][3] = '#';
    block->shape[4][4] = '#';

    block->next = NULL;
    block->prev = NULL;
    
    int minRow = BLOCK_SIZE, maxRow = -1;
    int minCol = BLOCK_SIZE, maxCol = -1;
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (block->shape[i][j] == '#') {
                if (i < minRow) minRow = i;
                if (i > maxRow) maxRow = i;
                if (j < minCol) minCol = j;
                if (j > maxCol) maxCol = j;
            }
        }
    }
    
    if (maxRow == -1 || maxCol == -1) {
        block->width = 0;
        block->height = 0;
        block->area = 0;
    } else {
        block->width = (maxCol - minCol + 1);
        block->height = (maxRow - minRow + 1);
        
        block->area = 0;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (block->shape[i][j] == '#') {
                    block->area++;
                }
            }
        }
    }
    
    return block;
}
    
    void freeBlock(Block* block) {
        if (block) {
            free(block);
        }
    }

NextBlock* createNextBlockList() {
    if (blockList == NULL) {
        fprintf(stderr, "Critical error: blockList is NULL\n");
        return NULL;
    }
    
    NextBlock* header = (NextBlock*)malloc(sizeof(NextBlock));
    if (header == NULL) {
        fprintf(stderr, "Critical error: Memory allocation failed for nextBlockList header\n");
        exit(1);
    }
    
    header->block = NULL;
    header->next = header;
    
    for (int i = 0; i < 5; i++) {
        Block* randomBlock = createRandomBlock();
        if (randomBlock == NULL) {
            fprintf(stderr, "Warning: Failed to create random block, retrying...\n");
            i--; 
            continue;
        }
        
        NextBlock* newNode = (NextBlock*)malloc(sizeof(NextBlock));
        if (newNode == NULL) {
            fprintf(stderr, "Critical error: Memory allocation failed for nextBlock node\n");
            free(randomBlock);
            
            NextBlock* current = header->next;
            while (current != header) {
                NextBlock* temp = current;
                current = current->next;
                free(temp->block);
                free(temp);
            }
            free(header);
            
            exit(1);
        }
        
        newNode->block = randomBlock;
        newNode->next = header->next;
        header->next = newNode;
    }
    
    return header;
}

void addBlockToNextList() {
    Block* newBlock = createRandomBlock();
    if (!newBlock) {
        fprintf(stderr, "Error: Failed to create random block\n");
        return;
    }
    
    NextBlock* newNode = (NextBlock*)malloc(sizeof(NextBlock));
    if (!newNode) {
        fprintf(stderr, "Error: Memory allocation failed for new nextBlock node\n");
        freeBlock(newBlock);
        return;
    }
    
    newNode->block = newBlock;
    
    if (!nextBlockList) {
        nextBlockList = (NextBlock*)malloc(sizeof(NextBlock));
        if (!nextBlockList) {
            fprintf(stderr, "Error: Memory allocation failed for nextBlockList header\n");
            freeBlock(newBlock);
            free(newNode);
            return;
        }
        nextBlockList->block = NULL;
        nextBlockList->next = nextBlockList;
    }
    
    NextBlock* current = nextBlockList;
    
    int count = 0;
    while (current->next != nextBlockList) {
        current = current->next;
        
        if (++count > 1000) {
            fprintf(stderr, "Error: Possible circular list corruption detected\n");
            freeBlock(newBlock);
            free(newNode);
            return;
        }
    }
    
    newNode->next = nextBlockList;
    current->next = newNode;
}

Block* getNextBlock() {
    Block* returnBlock = NULL;
    
    if (!nextBlockList || nextBlockList->next == nextBlockList) {
        if (nextBlockList) {
            for (int i = 0; i < 3; i++) {
                addBlockToNextList();
            }
            
            if (nextBlockList->next == nextBlockList) {
                returnBlock = createDefaultBlock();
                if (!returnBlock) returnBlock = createRandomBlock();
                return returnBlock;
            }
        } else {
            returnBlock = createDefaultBlock();
            if (!returnBlock) returnBlock = createRandomBlock();
            return returnBlock;
        }
    }
    
    NextBlock* firstNode = nextBlockList->next;
    returnBlock = firstNode->block;
    
    nextBlockList->next = firstNode->next;
    
    free(firstNode); 
    
    if (!returnBlock) {
        returnBlock = createDefaultBlock();
        if (!returnBlock) returnBlock = createRandomBlock();
    }
    
    addBlockToNextList();
    
    return returnBlock;
}

void displayNextBlockPreview(int previewX, int previewY) {
    if (nextBlockList == NULL || nextBlockList->next == nextBlockList) {
        int previewWidth = 6;
        int previewHeight = 6;
        
        moveToPosition(previewX, previewY - 1);
        printf("Next Block:");
        
        for (int x = 0; x < previewWidth + 2; x++) {
            moveToPosition(previewX - 1 + x, previewY - 2);
            printf("-");
            moveToPosition(previewX - 1 + x, previewY + previewHeight);
            printf("-");
        }
        
        for (int y = 0; y < previewHeight + 1; y++) {
            moveToPosition(previewX - 1, previewY - 1 + y);
            printf("|");
            moveToPosition(previewX + previewWidth, previewY - 1 + y);
            printf("|");
        }
        
        moveToPosition(previewX + 1, previewY + 2);
        printf("Empty");
        
        return;
    }
    
    Block* nextBlock = nextBlockList->next->block;
    if (nextBlock == NULL) {
        fprintf(stderr, "Error: Invalid block in preview\n");
        
        NextBlock* invalidNode = nextBlockList->next;
        nextBlockList->next = invalidNode->next;
        free(invalidNode);
        
        if (nextBlockList->next != nextBlockList) {
            nextBlock = nextBlockList->next->block;
            if (nextBlock == NULL) {
                return;
            }
        } else {
            return;
        }
    }
    
    int previewWidth = 6;
    int previewHeight = 6;
    
    for (int y = 0; y < previewHeight; y++) {
        for (int x = 0; x < previewWidth; x++) {
            moveToPosition(previewX + x, previewY + y);
            printf(" ");
        }
    }
    
    moveToPosition(previewX, previewY - 1);
    printf("Next Block:");
    
    int blockWidth = (nextBlock->width > 0 && nextBlock->width <= 10) ? nextBlock->width : 4;
    int blockHeight = (nextBlock->height > 0 && nextBlock->height <= 10) ? nextBlock->height : 4;
    
    int centerX = previewX + previewWidth / 2 - blockWidth / 2;
    int centerY = previewY + previewHeight / 2 - blockHeight / 2;
    
    for (int i = 0; i < blockHeight; i++) {
        for (int j = 0; j < blockWidth; j++) {
            if (i < 0 || i >= BLOCK_SIZE || j < 0 || j >= BLOCK_SIZE) {
                continue;
            }
            
            bool isBlock = false;
            
            if (nextBlock->shape[i][j] == '#') {
                isBlock = true;
            } else if (nextBlock->shape[i][j] == 1) {
                isBlock = true;
            }
            
            if (isBlock) {
                int displayX = centerX + j;
                int displayY = centerY + i;
                
                if (displayX >= previewX && displayX < previewX + previewWidth &&
                    displayY >= previewY && displayY < previewY + previewHeight) {
                    moveToPosition(displayX, displayY);
                    printf("#");
                }
            }
        }
    }
    
    for (int x = 0; x < previewWidth + 2; x++) {
        moveToPosition(previewX - 1 + x, previewY - 2);
        printf("-");
        moveToPosition(previewX - 1 + x, previewY + previewHeight);
        printf("-");
    }
    
    for (int y = 0; y < previewHeight + 1; y++) {
        moveToPosition(previewX - 1, previewY - 1 + y);
        printf("|");
        moveToPosition(previewX + previewWidth, previewY - 1 + y);
        printf("|");
    }
}

void freeNextBlockList() {
    if (nextBlockList == NULL) {
        return;
    }
    
    if (nextBlockList->next == nextBlockList) {
        if (nextBlockList->block != NULL) {
            free(nextBlockList->block);
            nextBlockList->block = NULL;
        }
        
        free(nextBlockList);
        nextBlockList = NULL;
        return;
    }
    
    NextBlock* header = nextBlockList;
    NextBlock* current = nextBlockList->next;
    NextBlock* next = NULL;
    
    int count = 0;
    const int MAX_NODES = 1000;
    
    while (current != header && current != NULL && count < MAX_NODES) {
        next = current->next;
        
        if (current->block != NULL) {
            free(current->block);
            current->block = NULL;
        }
        
        free(current);
        
        current = next;
        count++;
    }
    
    if (count >= MAX_NODES) {
        fprintf(stderr, "Warning: Possible memory leak in freeNextBlockList (hit node limit)\n");
    }
    
    if (header->block != NULL) {
        free(header->block);
        header->block = NULL;
    }
    
    free(header);
    nextBlockList = NULL;
}

Block* createRandomBlock() {
    if (blockList == NULL) {
        fprintf(stderr, "Error: Block list is not initialized\n");
        Block* defaultBlock = createDefaultBlock();
        if (!defaultBlock) {
            fprintf(stderr, "Fatal error: Cannot create default block\n");
            exit(1);
        }
        return defaultBlock;
    }

    int totalBlocks = 0;
    int validBlocks = 0;
    Block* temp = blockList;
    while (temp != NULL) {
        totalBlocks++;
        if (temp->area > 0) {
            validBlocks++;
        }
        temp = temp->next;
    }
    
    if (totalBlocks == 0) {
        fprintf(stderr, "Error: No blocks available\n");
        Block* defaultBlock = createDefaultBlock();
        if (!defaultBlock) {
            fprintf(stderr, "Fatal error: Cannot create default block\n");
            exit(1);
        }
        return defaultBlock;
    }
    
    if (validBlocks == 0) {
        fprintf(stderr, "Error: No valid blocks with area > 0 available\n");
        Block* defaultBlock = createDefaultBlock();
        if (!defaultBlock) {
            fprintf(stderr, "Fatal error: Cannot create default block\n");
            exit(1);
        }
        return defaultBlock;
    }

    Block* selectedBlock = NULL;
    
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;
    
    do {
        int randomIndex = rand() % totalBlocks;

        selectedBlock = blockList;
        for (int i = 0; i < randomIndex && selectedBlock != NULL; i++) {
            selectedBlock = selectedBlock->next;
        }
        
        if (selectedBlock == NULL) {
            continue;
        }
        
        attempts++;
        if (attempts >= MAX_ATTEMPTS) {
            fprintf(stderr, "Warning: Could not select a valid block after %d attempts\n", MAX_ATTEMPTS);
            Block* defaultBlock = createDefaultBlock();
            if (!defaultBlock) {
                fprintf(stderr, "Fatal error: Cannot create default block\n");
                exit(1);
            }
            return defaultBlock;
        }
        
    } while (selectedBlock == NULL || selectedBlock->area <= 0);

    Block* copy = (Block*)malloc(sizeof(Block));
    if (copy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            copy->shape[i][j] = ' ';
        }
    }

    for (int i = 0; i < BLOCK_SIZE && i < selectedBlock->height; i++) {
        for (int j = 0; j < BLOCK_SIZE && j < selectedBlock->width; j++) {
            copy->shape[i][j] = selectedBlock->shape[i][j];
        }
    }

    copy->width = selectedBlock->width;
    copy->height = selectedBlock->height;
    copy->area = selectedBlock->area;
    copy->next = NULL;
    copy->prev = NULL;

    if (copy->area <= 0) {
        fprintf(stderr, "Warning: Created block has invalid area, fixing...\n");
        copy->area = 0;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (copy->shape[i][j] == '#') {
                    copy->area++;
                }
            }
        }
        
        if (copy->area <= 0) {
            fprintf(stderr, "Error: Could not create a valid block, using default\n");
            free(copy);
            Block* defaultBlock = createDefaultBlock();
            if (!defaultBlock) {
                fprintf(stderr, "Fatal error: Cannot create default block\n");
                exit(1);
            }
            return defaultBlock;
        }
    }

    return copy;
}

void displayGamePage() {
    fprintf(stderr, "Entering displayGamePage()\n");
    
    resetCursor();
    
    memset(screenBuffer, ' ', sizeof(screenBuffer));
    
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            screenBuffer[i][j] = (gameBox[i][j] == '#') ? '#' : '.';
        }
    }
    
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            if (shadowBuffer[i][j] && screenBuffer[i][j] == '.') {
                screenBuffer[i][j] = '*';
            }
        }
    }
    
    if (currentBlock != NULL) {
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (currentBlock->shape[i][j] == '#') {
                    int y = currentRow + i;
                    int x = currentCol + j;
                    if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                        screenBuffer[y][x] = '#';
                    }
                }
            }
        }
    }
    
    char nextBlockPreview[6][9];
    memset(nextBlockPreview, '.', sizeof(nextBlockPreview));
    for (int i = 0; i < 6; i++) {
        nextBlockPreview[i][8] = '\0';
    }
    
    if (nextBlockList && nextBlockList->next != NULL) {
        Block* nextBlock = nextBlockList->next->block;
        if (nextBlock != NULL) {
            int offsetY = (6 - BLOCK_SIZE) / 2;
            int offsetX = (8 - BLOCK_SIZE) / 2;
            for (int i = 0; i < BLOCK_SIZE; i++) {
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    if (nextBlock->shape[i][j] == '#') {
                        int previewY = i + offsetY;
                        int previewX = j + offsetX;
                        if (previewY >= 0 && previewY < 6 && previewX >= 0 && previewX < 8) {
                            nextBlockPreview[previewY][previewX] = '#';
                        }
                    }
                }
            }
        }
    }
    
    printf("+================+    +===========================+    +==========+\n");
    printf("| Score: %6d |    |                           |    | Next      |\n", currentPlayerScore);
    printf("+================+    +===========================+    +==========+\n");
    printf("| Clear Row: %4d |    |", currentClearedRow);
    
    for (int i = 0; i < BOX_HEIGHT; i++) {
        if (i == 0) {
        } else {
            printf("|                |    |");
        }
        
        printf(" ");
        for (int j = 0; j < BOX_WIDTH; j++) {
            printf("%c", screenBuffer[i][j]);
        }
        printf(" |    |%s|\n", (i < 6) ? nextBlockPreview[i] : "        ");
    }
    
    printf("+================+    +===========================+    +==========+\n");
    printf("| Blocks: %6d |\n", currentPlacedBlock);
    printf("+================+\n");
    printf("| Controls:      |\n");
    printf("| 'a' left       |\n");
    printf("| 'd' right      |\n");
    printf("| 's' down       |\n");
    printf("| 'w' rotate     |\n");
    printf("| ' ' hard drop  |\n");
    printf("+================+\n");
    printf("| Notes:         |\n");
    printf("| > Press 'p' to pause the game      |\n");
    printf("| > Press 'l' to move the next block |\n");
    printf("+================+\n");
    
    if (isPaused) {
        displayPausedMessage();
    }
    
    fflush(stdout);
    
    fprintf(stderr, "Exiting displayGamePage()\n");
}

void resetCursor() {
    #ifdef _WIN32
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut != INVALID_HANDLE_VALUE) {
        COORD coord = {0, 0};
        SetConsoleCursorPosition(hStdOut, coord);
    } else {
        printf("\033[H");
        fflush(stdout);
    }
    #else
    printf("\033[H");
    fflush(stdout);
    
    #ifdef HAVE_TERMIOS_H
    if (isatty(STDOUT_FILENO)) {
        if (cursor_address != NULL) {
            tputs(tgoto(cursor_address, 0, 0), 1, putchar);
        }
    }
    #endif
    #endif
    
    static int positioning_attempts = 0;
    if (positioning_attempts > 3) {
        printf("\033[2J");
        positioning_attempts = 0;
    } else {
        positioning_attempts++;
    }
}

void renderToBuffer() {
    memset(screenBuffer, ' ', sizeof(screenBuffer));
    
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            if (gameBox[i][j] == '#') {
                screenBuffer[i][j] = '#';
            } else {
                screenBuffer[i][j] = '.';
            }
        }
    }
    
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            if (shadowBuffer[i][j] && screenBuffer[i][j] == '.') {
                screenBuffer[i][j] = '*';
            }
        }
    }
    
    if (currentBlock) {
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (currentBlock->shape[i][j] == '#') {
                    int y = currentRow + i;
                    int x = currentCol + j;
                    if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                        screenBuffer[y][x] = '#';
                    }
                }
            }
        }
    }
}

void flushBuffer() {
    resetCursor();
    
    printf("Score                                         Next    \n");
    printf("+============+    +===========================+   +==========+\n");
    
    char scoreStr[15], rowStr[15], blockStr[15];
    sprintf(scoreStr, "%d", currentPlayerScore);
    sprintf(rowStr, "%d", currentClearedRow);
    sprintf(blockStr, "%d", currentPlacedBlock);
    
    char nextBlockPreview[6][9];
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            nextBlockPreview[i][j] = '.';
        }
        nextBlockPreview[i][8] = '\0';
    }
    
    if (nextBlockList && nextBlockList->next != NULL) {
        NextBlock* nextNode = nextBlockList->next;
        Block* nextBlock = nextNode->block;
        
        if (nextBlock != NULL) {
            for (int i = 0; i < BLOCK_SIZE && i < 6; i++) {
                for (int j = 0; j < BLOCK_SIZE && j < 8; j++) {
                    if (nextBlock->shape[i][j] == '#') {
                        nextBlockPreview[i][j] = '#';
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < BOX_HEIGHT; i++) {
        if (i == 0) {
            printf("|%12s|    |", scoreStr);
        } else if (i == 1) {
            printf("+============+    |");
        } else if (i == 2) {
            printf("               |");
        } else if (i == 3) {
            printf("Clear Row       |");
        } else if (i == 4) {
            printf("+============+    |");
        } else if (i == 5) {
            printf("|%12s|    |", rowStr);
        } else if (i == 6) {
            printf("+============+    |");
        } else if (i == 7) {
            printf("Block            |");
        } else if (i == 8) {
            printf("+============+    |");
        } else if (i == 9) {
            printf("|%12s|    |", blockStr);
        } else if (i == 10) {
            printf("+============+    |");
        } else if (i == 11) {
            printf("               |");
        } else if (i == 12) {
            printf("Controls:       |");
        } else if (i == 13) {
            printf("               |");
        } else if (i == 14) {
            printf("'a' left       |");
        } else if (i == 15) {
            printf("'d' right      |");
        } else if (i == 16) {
            printf("'s' down       |");
        } else if (i == 17) {
            printf("'w' rotate     |");
        } else if (i == 18) {
            printf("' ' hard       |");
        } else if (i == 19) {
            printf("drop           |");
        } else {
            printf("               |");
        }
        
        for (int j = 0; j < BOX_WIDTH; j++) {
            putchar(screenBuffer[i][j]);
        }
        
        if (i >= 0 && i <= 5) {
            printf("|   |%s|\n", nextBlockPreview[i]);
        } else if (i == 6) {
            printf("|   +==========+\n");
        } else if (i == 7) {
            printf("|   notes:     \n");
        } else if (i == 8) {
            printf("|   ==========  \n");
        } else if (i == 9) {
            printf("|   > Press 'p' \n");
        } else if (i == 10) {
            printf("|     to pause  \n");
        } else if (i == 11) {
            printf("|     the game \n");
        } else if (i == 12) {
            printf("|   > Press 'l'\n");
        } else if (i == 13) {
            printf("|     to move  \n");
        } else if (i == 14) {
            printf("|     the next \n");
        } else if (i == 15) {
            printf("|     Block    \n");
        } else {
            printf("|             \n");
        }
    }
    
    printf("               +===========================+             \n");
    
    if (isPaused) {
        displayPausedMessage();
    }
}

void displayPausedMessage() {
    int midX = BOX_OFFSET_X + (BOX_WIDTH / 2) - 3;
    int midY = BOX_OFFSET_Y + (BOX_HEIGHT / 2);
    
    moveToPosition(midX, midY);
    printf("PAUSED");
}

void checkAndClearLines() {
    int linesCleared = 0;

    for (int row = BOX_HEIGHT - 1; row >= 0; row--) {
        bool isLineFull = true;

        for (int col = 0; col < BOX_WIDTH; col++) {
            if (gameBox[row][col] == ' ') {
                isLineFull = false;
                break;
            }
        }

        if (isLineFull) {
            linesCleared++;
            currentClearedRow++;

            for (int moveRow = row; moveRow > 0; moveRow--) {
                for (int col = 0; col < BOX_WIDTH; col++) {
                    gameBox[moveRow][col] = gameBox[moveRow - 1][col];
                }
            }

            for (int col = 0; col < BOX_WIDTH; col++) {
                gameBox[0][col] = ' ';
            }

            row++;
        }
    }

    if (linesCleared > 0) {
        updateScore(linesCleared);

        if (score % 1000 == 0) {
            increaseDifficulty();
        }
    }
}

void updateScore(int linesCleared) {
    int points = 0;
    switch (linesCleared) {
        case 1: points = 100; break;
        case 2: points = 300; break;
        case 3: points = 500; break;
        case 4: points = 800; break;
    }

    points *= (level + 1);

    score += points;
}

void increaseDifficulty() {
    level++;
    
    fallSpeed = MAX(100, 500 - (level * 50));
}

void gameLoop() {
    bool gameOver = false;
    int lastFallTime = 0;
    int currentTime = 0;
    int frameTime = 0;
    int frameDelay = 100;
    int initRetries = 0;
    const int MAX_INIT_RETRIES = 3;
    
    int prevRow = -1;
    int prevCol = -1;
    Block* prevBlock = NULL;
    bool needFullRedraw = true;
    
    resetTerminal();
    
    while (!initializeGameBox() && initRetries < MAX_INIT_RETRIES) {
        printf("Failed to initialize game box. Retrying... (%d/%d)\n", 
               ++initRetries, MAX_INIT_RETRIES);
        Sleep(500);
        resetTerminal();
    }
    
    if (initRetries >= MAX_INIT_RETRIES) {
        printf("Failed to initialize game after %d attempts. Exiting.\n", MAX_INIT_RETRIES);
        return;
    }
    
    initRetries = 0;
    while ((nextBlockList = createNextBlockList()) == NULL && initRetries < MAX_INIT_RETRIES) {
        printf("Failed to create block list. Retrying... (%d/%d)\n", 
               ++initRetries, MAX_INIT_RETRIES);
        Sleep(500);
    }
    
    if (nextBlockList == NULL) {
        printf("Failed to create block list after %d attempts. Exiting.\n", MAX_INIT_RETRIES);
        return;
    }
    
    initRetries = 0;
    while ((currentBlock = getNextBlock()) == NULL && initRetries < MAX_INIT_RETRIES) {
        printf("Failed to get initial block. Retrying... (%d/%d)\n", 
               ++initRetries, MAX_INIT_RETRIES);
        Sleep(500);
        
        if (initRetries >= MAX_INIT_RETRIES - 1) {
            currentBlock = createRandomBlock();
            if (currentBlock) break;
        }
    }
    
    if (!currentBlock) {
        printf("Failed to load blocks after %d attempts. Game terminated.\n", MAX_INIT_RETRIES);
        cleanupGameResources();
        return;
    }
    
    currentRow = 0;
    currentCol = (BOX_WIDTH - currentBlock->width) / 2;
    
    clearScreen();
    resetCursor();
    
    updateShadowBlock();
    
    renderToBuffer();
    flushBuffer();
    
    lastFallTime = getMilliseconds();
    frameTime = lastFallTime;
    
    while (!gameOver) {
        if (!currentBlock) {
            printf("Error: Current block is null. Attempting to create new block.\n");
            currentBlock = createRandomBlock();
            if (!currentBlock) {
                printf("Fatal error: Cannot create a new block. Game terminated.\n");
                gameOver = true;
                break;
            }
            currentRow = 0;
            currentCol = (BOX_WIDTH - currentBlock->width) / 2;
            needFullRedraw = true;
        }
        
        if (!nextBlockList) {
            printf("Error: Next block list is null. Attempting to recreate.\n");
            nextBlockList = createNextBlockList();
            if (!nextBlockList) {
                printf("Fatal error: Cannot recreate block list. Game terminated.\n");
                gameOver = true;
                break;
            }
            needFullRedraw = true;
        }
        
        currentTime = getMilliseconds();
        
        int elapsed = currentTime - frameTime;
        
        if (kbhit()) {
            char key = getch();
            
            if (key == 'p' || key == 'P') {
                isPaused = !isPaused;
                needFullRedraw = true;
                resetCursor();
            }
            else if (isPaused) {
                isPaused = false;
                needFullRedraw = true;
                resetCursor();
            }
            else if (!isPaused && currentBlock) {
                switch (key) {
                    case 'a':
                    case 'A':
                        if (currentBlock) moveBlockLeft();
                        needFullRedraw = true;
                        break;
                    case 'd':
                    case 'D':
                        if (currentBlock) moveBlockRight();
                        needFullRedraw = true;
                        break;
                    case 's':
                    case 'S':
                        if (currentBlock) moveBlockDown();
                        needFullRedraw = true;
                        break;
                    case 'w':
                    case 'W':
                        if (currentBlock) {
                            Block tempBlock = *currentBlock;
                            rotateBlock(&tempBlock, currentCol, currentRow);
                            needFullRedraw = true;
                        }
                        break;
                    case ' ':
                        if (currentBlock) {
                            hardDrop();
                            needFullRedraw = true;
                        }
                        break;
                    case 'l':
                    case 'L': {
                        if (!nextBlockList || !nextBlockList->next) break;
                        
                        NextBlock* first = nextBlockList->next;
                        nextBlockList->next = first->next;
                        
                        NextBlock* last = nextBlockList;
                        while (last->next != NULL && last->next != nextBlockList) last = last->next;
                        
                        if (last != NULL) {
                            last->next = first;
                            first->next = nextBlockList;
                        }
                        
                        needFullRedraw = true;
                        break;
                    }
                    case 'q':
                    case 'Q':
                        gameOver = true;
                        break;
                    case 'r':
                    case 'R':
                        clearScreen();
                        resetCursor();
                        needFullRedraw = true;
                        break;
                }
                
                if (needFullRedraw && currentBlock) {
                    updateShadowBlock();
                }
            }
        }
        
        if (isPaused) {
            if (needFullRedraw) {
                renderToBuffer();
                displayPausedMessage();
                flushBuffer();
                needFullRedraw = false;
            }
            
            Sleep(5);
            continue;
        }
        
        if (elapsed > frameDelay) {
            needFullRedraw = needFullRedraw || (prevBlock != currentBlock || 
                                              prevRow != currentRow || 
                                              prevCol != currentCol);
            
            if (currentBlock && currentTime - lastFallTime >= fallSpeed) {
                if (isValidPosition(currentCol, currentRow + 1, currentBlock)) {
                    currentRow++;
                    updateShadowBlock();
                    needFullRedraw = true;
                } else {
                    if (currentBlock) {
                        placeBlock(currentRow, currentCol);
                        
                        checkAndClearLines();
                    }
                    
                    Block* nextBlock = getNextBlock();
                    
                    if (!nextBlock) {
                        nextBlock = createRandomBlock();
                        if (!nextBlock) {
                            printf("No blocks remaining. Game over.\n");
                            gameOver = true;
                            break;
                        }
                    }
                    
                    currentBlock = nextBlock;
                    if (!currentBlock) {
                        printf("Failed to get a valid block. Game over.\n");
                        gameOver = true;
                        break;
                    }
                    
                    currentRow = 0;
                    currentCol = (BOX_WIDTH - currentBlock->width) / 2;
                    
                    if (!isValidPosition(currentCol, currentRow, currentBlock)) {
                        int offset = 1;
                        bool foundValidPosition = false;
                        
                        while (offset < BOX_WIDTH) {
                            if (isValidPosition(currentCol + offset, currentRow, currentBlock)) {
                                currentCol += offset;
                                foundValidPosition = true;
                                break;
                            }
                            if (isValidPosition(currentCol - offset, currentRow, currentBlock)) {
                                currentCol -= offset;
                                foundValidPosition = true;
                                break;
                            }
                            offset++;
                        }
                        
                        if (!foundValidPosition) {
                            gameOver = true;
                            break;
                        }
                    }
                    
                    if (currentBlock) {
                        updateShadowBlock();
                    }
                    needFullRedraw = true;
                }
                
                lastFallTime = currentTime;
            }
            
            if (needFullRedraw) {
                static int frameCount = 0;
                if (++frameCount % 30 == 0) {
                    resetCursor();
                }
                
                renderToBuffer();
                flushBuffer();
                needFullRedraw = false;
            }
            
            prevRow = currentRow;
            prevCol = currentCol;
            prevBlock = currentBlock;
            
            frameTime = currentTime;
        }
        
        Sleep(5);
    }
    
    resetCursor();
    
    gameOverMenu();
    
    cleanupGameResources();
}

void resetTerminal() {
    resetCursor();
    
    printf("\033[2J");
    
    fflush(stdout);
    
    #ifdef _WIN32
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hStdOut, 7);
    }
    #else
    printf("\033[0m");
    #endif
}

void cleanupGameResources() {
    if (nextBlockList) {
        NextBlock* current = nextBlockList->next;
        while (current != nextBlockList) {
            NextBlock* temp = current;
            current = current->next;
            free(temp);
        }
        free(nextBlockList);
        nextBlockList = NULL;
    }
    
    if (currentBlock) {
        free(currentBlock);
        currentBlock = NULL;
    }
}

void moveToPosition(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

void rotateBlock(Block* block, int currentCol, int currentRow) {
    if (!block) return;

    Block rotated = *block;
    
    char temp[BLOCK_SIZE][BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            temp[j][BLOCK_SIZE - 1 - i] = rotated.shape[i][j];
        }
    }
    memcpy(rotated.shape, temp, sizeof(temp));
    
    int minX = BLOCK_SIZE, maxX = -1, minY = BLOCK_SIZE, maxY = -1;
    rotated.area = 0;
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (rotated.shape[i][j] == '#') {
                minX = (j < minX) ? j : minX;
                maxX = (j > maxX) ? j : maxX;
                minY = (i < minY) ? i : minY;
                maxY = (i > maxY) ? i : maxY;
                rotated.area++;
            }
        }
    }
    
    rotated.width = (maxX >= minX) ? (maxX - minX + 1) : 0;
    rotated.height = (maxY >= minY) ? (maxY - minY + 1) : 0;
    
    if (isValidPosition(currentCol, currentRow, &rotated)) {
        memcpy(block->shape, rotated.shape, sizeof(rotated.shape));
        block->width = rotated.width;
        block->height = rotated.height;
        block->area = rotated.area;
        
        updateShadowBlock();
    }
}

bool isValidPosition(int col, int row, Block* b) {
    if (b == NULL) {
        return false;
    }
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (b->shape[i][j] == '#') {
                int x = col + j;
                int y = row + i;
                
                if (x < 0 || x >= BOX_WIDTH) 
                    return false;
                
                if (y >= BOX_HEIGHT) 
                    return false;
                
                if (y >= 0 && gameBox[y][x] != ' ') 
                    return false;
            }
        }
    }
    
    return true;
}

void updateShadowBlock() {
    memset(shadowBuffer, 0, sizeof(shadowBuffer));

    if (!currentBlock) return;

    int shadowRow = currentRow;
    
    while (isValidPosition(currentCol, shadowRow + 1, currentBlock)) {
        shadowRow++;
    }
    
    if (shadowRow == currentRow) {
        return;
    }
    
    for (int i = 0; i < currentBlock->height; i++) {
        for (int j = 0; j < currentBlock->width; j++) {
            if (currentBlock->shape[i][j] == '#') {
                int y = shadowRow + i;
                int x = currentCol + j;
                
                if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                    if (gameBox[y][x] != '#') {
                        shadowBuffer[y][x] = 1;
                    }
                }
            }
        }
    }
    
    renderShadow();
}

void renderShadow() {
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            if (shadowBuffer[i][j]) {
                moveToPosition(j + BOX_OFFSET_X, i + BOX_OFFSET_Y);
                printf("*");
            }
        }
    }
    
    fflush(stdout);
}

void renderActiveBlock() {
    if (currentBlock == NULL) {
        return;
    }
    
    for (int i = 0; i < currentBlock->height; i++) {
        for (int j = 0; j < currentBlock->width; j++) {
            if (currentBlock->shape[i][j] == 1 || currentBlock->shape[i][j] == '#') {
                int y = currentRow + i;
                int x = currentCol + j;
                
                if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                    screenBuffer[y][x] = '#';
                    
                    moveToPosition(x + BOX_OFFSET_X, y + BOX_OFFSET_Y);
                    printf("#");
                }
            }
        }
    }
}

void clearActiveBlock() {
    if (currentBlock == NULL) {
        return;
    }
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (currentBlock->shape[i][j] == 1 || currentBlock->shape[i][j] == '#') {
                int y = currentRow + i;
                int x = currentCol + j;
                
                if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                    if (gameBox[y][x] != '#') {
                        moveToPosition(x + BOX_OFFSET_X, y + BOX_OFFSET_Y);
                        
                        if (shadowBuffer[y][x]) {
                            printf("*");
                        } else {
                            printf(".");
                        }
                    }
                }
            }
        }
    }
}

void renderGameBox() {
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            moveToPosition(j + BOX_OFFSET_X, i + BOX_OFFSET_Y);
            
            if (gameBox[i][j] == '#') {
                printf("#");
            } else {
                if (shadowBuffer[i][j]) {
                    printf("*");
                } else {
                    printf(".");
                }
            }
        }
    }
}

bool initializeGameBox() {
    for (int i = 0; i < BOX_HEIGHT; i++) {
        for (int j = 0; j < BOX_WIDTH; j++) {
            gameBox[i][j] = ' ';
        }
    }

    memset(shadowBuffer, 0, sizeof(shadowBuffer));

    score = 0;
    level = 1;
    fallSpeed = 500;
    currentClearedRow = 0;
    currentPlacedBlock = 0;
    
    renderGameBox();
    return true;
}

void moveBlockRight() {
    if (currentBlock == NULL) {
        return;
    }
    
    if (isValidPosition(currentCol + 1, currentRow, currentBlock)) {
        clearActiveBlock();
        
        currentCol++;
        
        updateShadowBlock();
        
        renderActiveBlock();
    }
}

void moveBlockDown() {
    if (currentBlock == NULL) {
        return;
    }
    
    if (isValidPosition(currentCol, currentRow + 1, currentBlock)) {
        clearActiveBlock();
        
        currentRow++;
        
        updateShadowBlock();
        
        renderActiveBlock();
    } else {
        placeBlock(-1, -1);
    }
}

void hardDrop() {
    if (currentBlock == NULL) {
        return;
    }
    
    clearActiveBlock();
    
    int newRow = currentRow;
    while (isValidPosition(currentCol, newRow + 1, currentBlock)) {
        newRow++;
    }
    
    currentRow = newRow;
    
    placeBlock(currentRow, currentCol);
    
    currentBlock = getNextBlock();
    
    if (currentBlock == NULL || !isValidPosition(
        (BOX_WIDTH - currentBlock->width)/2,
        0,
        currentBlock
    )) {
        gameOverMenu();
    }
    
    renderGameBox();
    updateShadowBlock();
}

void updateGameDisplay() {
    updateShadowBlock();
    
    renderActiveBlock();
    
    moveToPosition(BOX_WIDTH + BOX_OFFSET_X + 2, BOX_OFFSET_Y);
    printf("Score: %d", score);
    
    moveToPosition(BOX_WIDTH + BOX_OFFSET_X + 2, BOX_OFFSET_Y + 1);
    printf("Level: %d", level);
    
    displayNextBlockPreview(BOX_WIDTH + BOX_OFFSET_X + 2, BOX_OFFSET_Y + 3);
    
    fflush(stdout);
}

void moveBlockLeft() {
    if (currentBlock == NULL) {
        return;
    }
    
    if (isValidPosition(currentCol - 1, currentRow, currentBlock)) {
        clearActiveBlock();
        
        currentCol--;
        
        updateShadowBlock();
        
        renderActiveBlock();
    }
}

void placeBlock(int row, int col) {
    if (row == -1 && col == -1) {
        if (currentBlock == NULL) {
            return;
        }
        
        for (int i = 0; i < currentBlock->height; i++) {
            for (int j = 0; j < currentBlock->width; j++) {
                if (currentBlock->shape[i][j] == '#') {
                    int y = currentRow + i;
                    int x = currentCol + j;
                    
                    if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                        gameBox[y][x] = '#';
                    }
                }
            }
        }
        
        currentPlacedBlock++;
        
        memset(shadowBuffer, 0, sizeof(shadowBuffer));
        
        checkAndClearLines();
        
        NextBlock* temp = nextBlockList->next;
        if (temp != NULL) {
            nextBlockList->next = temp->next;
            
            currentBlock = temp->block;
            
            free(temp);
            
            addBlockToNextList();
        } else {
            currentBlock = createRandomBlock();
        }
        
        currentRow = 0;
        currentCol = BOX_WIDTH / 2 - currentBlock->width / 2;
        
        if (!isValidPosition(currentCol, currentRow, currentBlock)) {
            gameOverMenu();
        }
        
        renderGameBox();
        
        updateShadowBlock();
    } 
    else {
        if (currentBlock == NULL) {
            return;
        }
        
        int savedRow = currentRow;
        int savedCol = currentCol;
        
        currentRow = row;
        currentCol = col;
        
        if (isValidPosition(currentCol, currentRow, currentBlock)) {
            for (int i = 0; i < currentBlock->height; i++) {
                for (int j = 0; j < currentBlock->width; j++) {
                    if (currentBlock->shape[i][j] == '#') {
                        int y = row + i;
                        int x = col + j;
                        if (y >= 0 && y < BOX_HEIGHT && x >= 0 && x < BOX_WIDTH) {
                            gameBox[y][x] = '#';
                        }
                    }
                }
            }
            
            currentPlacedBlock++;
            checkAndClearLines();
            
            NextBlock* temp = nextBlockList->next;
            if (temp != NULL) {
                nextBlockList->next = temp->next;
                currentBlock = temp->block;
                free(temp);
                addBlockToNextList();
            } else {
                currentBlock = createRandomBlock();
            }
            
            currentRow = 0;
            currentCol = BOX_WIDTH / 2 - currentBlock->width / 2;
            renderGameBox();
            updateShadowBlock();
            
            if (!isValidPosition(currentCol, currentRow, currentBlock)) {
                gameOverMenu();
            }
        } 
        else {
            currentRow = savedRow;
            currentCol = savedCol;
        }
    }
}

void clearRow() {
    int rowsCleared = 0;

    for (int i = BOX_HEIGHT - 1; i >= 0; i--) {
        bool fullRow = true;

        for (int j = 0; j < BOX_WIDTH; j++) {
            if (gameBox[i][j] != '#') {
                fullRow = false;
                break;
            }
        }

        if (fullRow) {
            rowsCleared++;
            
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOX_WIDTH; j++) {
                    gameBox[k][j] = gameBox[k-1][j];
                }
            }
            
            for (int j = 0; j < BOX_WIDTH; j++) {
                gameBox[0][j] = ' ';
            }
            
            i++;
        }
    }

    if (rowsCleared > 0) {
        currentClearedRow += rowsCleared;
        currentPlayerScore += (rowsCleared * rowsCleared) * (200 + (rowsCleared - 1) * 20) / 2;
    }
}

bool checkGameOver(void) {
    for (int j = 0; j < BOX_WIDTH; j++) {
        if (gameBox[0][j] == '#') {
            return true;
        }
    }
    return false;
}

void gameOverMenu() {
    clearScreen();
    printf("=============================================\n");
    printf("                 GAME OVER                   \n");
    printf("=============================================\n");
    printf("Press enter to continue...");
    getch();
    
    if (strcmp(currentUser.username, "Admin") == 0) {
        Block* current = blockList;
        while (current != NULL) {
            Block* temp = current;
            current = current->next;
            free(temp);
        }
        blockList = NULL;
        
        adminPage();
        return;
    }
    
    clearScreen();
    printf("=============================================\n");
    printf("               GAME STATISTICS               \n");
    printf("=============================================\n");
    printf("Current Statistics:\n");
    printf("Score: %d\n", currentPlayerScore);
    printf("Cleared Row: %d\n", currentClearedRow);
    printf("Placed Block: %d\n", currentPlacedBlock);
    printf("---------------------------------------------\n");
    printf("Previous Statistics:\n");
    printf("Score: %d\n", currentUser.score);
    printf("Cleared Row: %d\n", currentUser.clearedRow);
    printf("Placed Block: %d\n", currentUser.totalBlock);
    printf("---------------------------------------------\n");
    printf("Do you want to save your current score? (y/n): ");
    
    char choice = getch();
    printf("%c\n", choice);
    
    if (choice == 'y') {
        User* user = findUser(currentUser.username);
        if (user != NULL) {
            user->score = currentPlayerScore;
            user->clearedRow = currentClearedRow;
            user->totalBlock = currentPlacedBlock;
            
            currentUser.score = currentPlayerScore;
            currentUser.clearedRow = currentClearedRow;
            currentUser.totalBlock = currentPlacedBlock;
            
            saveUsers();
        }
    }
    
    NextBlock* current = nextBlockList;
    if (current != NULL) {
        NextBlock* start = current;
        current = current->next;
        while (current != start) {
            NextBlock* temp = current;
            current = current->next;
            free(temp->block);
            free(temp);
        }
        free(start);
    }
    nextBlockList = NULL;
    
    homePage();
}

void adminPage() {
    clearScreen();
    printf("=============================================\n");
    printf("                 ADMIN PAGE                  \n");
    printf("=============================================\n");
    printf("1. Play Game\n");
    printf("2. Add New Block\n");
    printf("3. Remove Block\n");
    printf("4. Log Out\n");
    printf("---------------------------------------------\n");
    printf("Choose: ");
    
    int choice;
    scanf("%d", &choice);
    
    switch (choice) {
        case 1:
            playGame();
            break;
            
        case 2:
            addNewBlock();
            break;
            
        case 3:
            removeBlock();
            break;
            
        case 4:
            saveBlocks();
            
            Block* current = blockList;
            while (current != NULL) {
                Block* temp = current;
                current = current->next;
                free(temp);
            }
            blockList = NULL;
            
            homePage();
            break;
            
        default:
            printf("Invalid choice. Please try again.\n");
            printf("Press enter to continue...");
            getchar();
            getchar();
            adminPage();
    }
}

void homePage() {
    clearScreen();
    displayTitle();
    printf("=============================================\n");
    printf("                  HOME PAGE                  \n");
    printf("=============================================\n");
    printf("1. Play\n");
    printf("2. View Player\n");
    printf("3. Exit\n");
    printf("---------------------------------------------\n");
    printf("Choose: ");
    
    int choice;
    scanf("%d", &choice);
    getchar();
    
    switch (choice) {
        case 1:
            clearScreen();
            displayTitle();
            printf("Input username: ");
            char username[MAX_USERNAME];
            fgets(username, MAX_USERNAME, stdin);
            username[strcspn(username, "\n")] = '\0';
            
            if (strcmp(username, "0") == 0) {
                homePage();
                return;
            } else if (strcmp(username, "Admin") == 0) {
                adminPage();
                return;
            }
            
            if (!validateUsername(username)) {
                printf("Press enter to continue...");
                getchar();
                homePage();
                return;
            }
            
            loginOrRegister(username);
            break;
            
        case 2:
            viewPlayers();
            break;
            
        case 3:
            exitProgram();
            break;
            
        default:
            printf("Invalid choice. Please try again.\n");
            printf("Press enter to continue...");
            getchar();
            homePage();
    }
}

void playGame() {
    currentPlayerScore = 0;
    currentClearedRow = 0;
    currentPlacedBlock = 0;
    
    if (blockList == NULL) {
        loadBlocks();
        if (blockList == NULL) {
            clearScreen();
            printf("No blocks available to play game!\n");
            printf("Press enter to continue...");
            getchar();
            getchar();
            homePage();
            return;
        }
    }
    
    initializeGame();
    
    nextBlockList = createNextBlockList();
    
    displayGamePage();
    
    gameLoop();
}

int compareUsers(const void* a, const void* b) {
    User* u1 = *(User**)a;
    User* u2 = *(User**)b;
    return u2->score - u1->score;
}

void viewPlayers() {
    loadUsers();
    
    int page = 0;
    int totalPlayers = 0;
    User* allPlayers[1000];
    
    for (int i = 0; i < HASH_SIZE; i++) {
        User* current = hashTable[i];
        while (current != NULL) {
            if (strcmp(current->username, "Admin") != 0) {
                allPlayers[totalPlayers++] = current;
            }
            current = current->next;
        }
    }
    
    if (totalPlayers == 0) {
        clearScreen();
        displayTitle();
        printf("=============================================\n");
        printf("                 VIEW PLAYER                 \n");
        printf("=============================================\n");
        printf("No players found!\n");
        printf("Press any key to return to home page...");
        getch();
        homePage();
        return;
    }
    
    qsort(allPlayers, totalPlayers, sizeof(User*), compareUsers);
    
    char key;
    do {
        clearScreen();
        displayTitle();
        printf("=============================================\n");
        printf("                 VIEW PLAYER                 \n");
        printf("=============================================\n");
        
        int startIndex = page * MAX_PLAYERS_PER_PAGE;
        int endIndex = startIndex + MAX_PLAYERS_PER_PAGE;
        if (endIndex > totalPlayers) endIndex = totalPlayers;
        
        printf("No. | Username                | Score  | Cleared Row | Total Block\n");
        printf("----+-------------------------+--------+-------------+------------\n");
        
        for (int i = startIndex; i < endIndex; i++) {
            printf("%-4d| %-24s| %-7d| %-12d| %-12d\n", 
                   i + 1, 
                   allPlayers[i]->username,
                   allPlayers[i]->score,
                   allPlayers[i]->clearedRow,
                   allPlayers[i]->totalBlock);
        }
        
        printf("====================================================================\n");
        printf("Page %d/%d\n", page + 1, (totalPlayers + MAX_PLAYERS_PER_PAGE - 1) / MAX_PLAYERS_PER_PAGE);
        printf("Press 'a' for previous page, 'd' for next page, 'q' to quit\n");
        
        key = getch();
        
        if (key == 'd' && endIndex < totalPlayers) {
            page++;
        } else if (key == 'a' && page > 0) {
            page--;
        }
    } while (key != 'q');
    
    homePage();
}

void exitProgram() {
    clearScreen();
    printf("=============================================\n");
    printf("                 CONFIRMATION                \n");
    printf("=============================================\n");
    printf("Are you sure you want to exit? (y/n): ");
    
    char choice;
    choice = getch();
    printf("%c\n", choice);
    
    if (choice == 'y' || choice == 'Y') {
        saveUsers();
        
        freeResources();
        
        exit(0);
    } else {
        homePage();
    }
}

void addNewBlock() {
    clearScreen();
    printf("=============================================\n");
    printf("             ADD NEW BLOCK MENU              \n");
    printf("=============================================\n");
    
    char newBlockShape[BLOCK_SIZE][BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            newBlockShape[i][j] = '.';
        }
    }
    
    int cursorRow = 0;
    int cursorCol = 0;
    bool isEraser = false;
    char key;
    
    do {
        clearScreen();
        printf("=============================================\n");
        printf("             ADD NEW BLOCK MENU              \n");
        printf("=============================================\n");
        printf("Use WASD to move, SPACE to draw, E to toggle eraser, Q to save\n");
        
        printf("Current tool: %s\n", isEraser ? "Eraser" : "Brush");
        
        printf("-------------\n");
        for (int i = 0; i < BLOCK_SIZE; i++) {
            printf("|");
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (i == cursorRow && j == cursorCol) {
                    printf("X");
                } else {
                    printf("%c", newBlockShape[i][j]);
                }
            }
            printf("|\n");
        }
        printf("-------------\n");
        
        key = getch();
        
        switch (key) {
            case 'w':
                if (cursorRow > 0) cursorRow--;
                break;
                
            case 'a':
                if (cursorCol > 0) cursorCol--;
                break;
                
            case 's':
                if (cursorRow < BLOCK_SIZE - 1) cursorRow++;
                break;
                
            case 'd':
                if (cursorCol < BLOCK_SIZE - 1) cursorCol++;
                break;
                
            case 'e':
                isEraser = !isEraser;
                break;
                
            case ' ':
                newBlockShape[cursorRow][cursorCol] = isEraser ? '.' : '#';
                break;
        }
    } while (key != 'q');
    
    bool isEmpty = true;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (newBlockShape[i][j] == '#') {
                isEmpty = false;
                break;
            }
        }
        if (!isEmpty) break;
    }
    
    if (isEmpty) {
        printf("Error: Block cannot be empty!\n");
        printf("Press enter to continue...");
        getchar();
        getchar();
        adminPage();
        return;
    }
    
    int minRow = BLOCK_SIZE, maxRow = 0;
    int minCol = BLOCK_SIZE, maxCol = 0;
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (newBlockShape[i][j] == '#') {
                if (i < minRow) minRow = i;
                if (i > maxRow) maxRow = i;
                if (j < minCol) minCol = j;
                if (j > maxCol) maxCol = j;
            }
        }
    }
    
    int width = maxCol - minCol + 1;
    int height = maxRow - minRow + 1;
    int area = width * height;
    
    Block* newBlock = (Block*)malloc(sizeof(Block));
    if (newBlock == NULL) {
        printf("Memory allocation failed!\n");
        adminPage();
        return;
    }
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            newBlock->shape[i][j] = newBlockShape[i][j];
        }
    }
    
    newBlock->width = width;
    newBlock->height = height;
    newBlock->area = area;
    newBlock->next = NULL;
    newBlock->prev = NULL;
    
    if (blockList == NULL) {
        blockList = newBlock;
    } else {
        Block* current = blockList;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newBlock;
        newBlock->prev = current;
    }
    
    saveBlocks();
    
    adminPage();
}

void removeBlock() {
    if (blockList == NULL) {
        clearScreen();
        printf("No blocks exist to remove!\n");
        printf("Press enter to continue...");
        getchar();
        getchar();
        adminPage();
        return;
    }
    
    Block* current = NULL;
    Block* selected = blockList;
    
    char key;
    do {
        clearScreen();
        printf("=============================================\n");
        printf("             REMOVE BLOCK MENU               \n");
        printf("=============================================\n");
        printf("Use A/D to browse blocks, S to toggle sort, E to delete, Q to quit\n");
        printf("Sorting: %s\n", sortAscending ? "Ascending" : "Descending");
        
        printf("-------------\n");
        for (int i = 0; i < BLOCK_SIZE; i++) {
            printf("|");
            for (int j = 0; j < BLOCK_SIZE; j++) {
                printf("%c", selected->shape[i][j]);
            }
            printf("|\n");
        }
        printf("-------------\n");
        printf("Width: %d, Height: %d, Area: %d\n", selected->width, selected->height, selected->area);
        
        key = getch();
        
        switch (key) {
            case 'a':
                if (selected->prev != NULL) {
                    selected = selected->prev;
                }
                break;
                
            case 'd':
                if (selected->next != NULL) {
                    selected = selected->next;
                }
                break;
                
            case 's':
                sortAscending = !sortAscending;
                
                bool swapped;
                do {
                    swapped = false;
                    current = blockList;
                    
                    while (current->next != NULL) {
                        Block* nextBlock = current->next;
                        
                        bool shouldSwap = false;
                        if (sortAscending) {
                            shouldSwap = current->area > nextBlock->area;
                        } else {
                            shouldSwap = current->area < nextBlock->area;
                        }
                        
                        if (shouldSwap) {
                            Block tempBlock = *current;
                            
                            memcpy(current->shape, nextBlock->shape, sizeof(nextBlock->shape));
                            current->width = nextBlock->width;
                            current->height = nextBlock->height;
                            current->area = nextBlock->area;
                            
                            memcpy(nextBlock->shape, tempBlock.shape, sizeof(tempBlock.shape));
                            nextBlock->width = tempBlock.width;
                            nextBlock->height = tempBlock.height;
                            nextBlock->area = tempBlock.area;
                            
                            swapped = true;
                        }
                        
                        current = current->next;
                    }
                } while (swapped);
                
                selected = blockList;
                break;
                
            case 'e':
                clearScreen();
                printf("Are you sure you want to delete this block? (y/n): ");
                char choice = getch();
                printf("%c\n", choice);
                
                if (choice == 'y') {
                    if (selected == blockList) {
                        blockList = selected->next;
                        if (blockList != NULL) {
                            blockList->prev = NULL;
                        }
                    } else {
                        selected->prev->next = selected->next;
                        if (selected->next != NULL) {
                            selected->next->prev = selected->prev;
                        }
                    }
                    
                    Block* toDelete = selected;
                    
                    if (selected->next != NULL) {
                        selected = selected->next;
                    } else if (selected->prev != NULL) {
                        selected = selected->prev;
                    } else {
                        selected = NULL;
                    }
                    
                    free(toDelete);
                    
                    if (blockList == NULL) {
                        printf("No blocks left to remove!\n");
                        printf("Press enter to continue...");
                        getchar();
                        getchar();
                        adminPage();
                        return;
                    }
                    
                    saveBlocks();
                }
                break;
        }
    } while (key != 'q');
    
    adminPage();
}

void freeResources() {
    for (int i = 0; i < HASH_SIZE; i++) {
        User* current = hashTable[i];
        while (current != NULL) {
            User* temp = current;
            current = current->next;
            free(temp);
        }
        hashTable[i] = NULL;
    }
    
    Block* currentBlock = blockList;
    while (currentBlock != NULL) {
        Block* temp = currentBlock;
        currentBlock = currentBlock->next;
        free(temp);
    }
    blockList = NULL;
    
    if (nextBlockList != NULL) {
        NextBlock* start = nextBlockList;
        NextBlock* current = nextBlockList->next;
        
        while (current != start) {
            NextBlock* temp = current;
            current = current->next;
            if (temp->block != NULL) {
                free(temp->block);
            }
            free(temp);
        }
        
        free(start);
        nextBlockList = NULL;
    }
}

int main() {

    srand(time(NULL));
    
    for(int i = 0; i < HASH_SIZE; i++) {
        hashTable[i] = NULL;
    }
    
    loadUsers();
    
    loadBlocks();
    
    if (blockList == NULL) {
        clearScreen();
        printf("Error: No blocks found in block.txt\n");
        printf("Press any key to continue...");
        getch();
        saveUsers();
        freeResources();
        return 0;
    }
    
    initializeGame();
    
    homePage();

    saveUsers();
    
    saveBlocks();
    
    freeResources();
    
    return 0;
}