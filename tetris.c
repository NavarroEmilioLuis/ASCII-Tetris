#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>

// Tetris grid size
#define HEIGHT 16
#define WIDTH 10

// Piece struct to hold current piece information
typedef struct
{
    int x;
    int y;
    int size;
    char shape;
}
piece;

// Helper struct for storing coordinates for rotation
typedef struct
{
    int x;
    int y;
}
coordinates;

// Global variables

// Declare game pieces
piece current;
piece next;
// Grid to store the current piece
int curPieceGrid[4][4];
// Grid to store the next piece
int nextPieceGrid[4][4];
// Display variable in string format
char board[HEIGHT * WIDTH];
// Display variable in string format
char nextBoard[4 * 4];
// Engine grid
int grid[HEIGHT][WIDTH];
// Shape names
char shapes[7] = {'O', 'I', 'T', 'S', 'Z', 'J', 'L'};
// Score variable
int score = 0;

int gameOver(void);
void movePiece(char direction);
int collisionX(char direction);
int collisionY(void);
void nextPiece(void);
void newPiece(void);
void updatePieceGrid(int pieceGrid[][4], char shape);
void updateGrid(void);
void rotatePiece(char direction);
void setPiece(void);
void checkLines(void);
void clearLine(int line);
void updateScore(int lines);
void drawGame(WINDOW *board_window, WINDOW *score_window, WINDOW *next_window);

int main(void)
{
    // Ncurses functions: Initialize screen, break characters,
    // prevent echo and invisible cursor (if possible)
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    // Check if the terminal has enough space
    if ((LINES < 20) || (COLS < 50))
    {
        endwin();
        printf("Your terminal needs to be at least 50x20\n");
        return 1;
    }

    // Create game windows
    // LINES and COLS are used to center the windows
    int gameHeight = 20;
    int gameWidth = 50;
    int gameY = (LINES - gameHeight) / 2;
    int gameX = (COLS - gameWidth) / 2;
    int gameBoardY = (LINES - HEIGHT) / 2;
    int gameBoardX = (COLS - WIDTH) / 2;

    // Outer window: Game window
    WINDOW *outer = newwin(gameHeight, gameWidth, gameY, gameX);
    box(outer, 0, 0);

    // Draws control information and score
    mvwprintw(outer, 8, 3, "Use arrow keys");
    mvwprintw(outer, 9, 6, "to move");
    mvwprintw(outer, 11, 4, "Rotate with");
    mvwprintw(outer, 12, 4, "'z' or 'x'");
    mvwprintw(outer, 3, 37, "Score:");
    mvwprintw(outer, 11, 37, "Next:");
    wrefresh(outer);

    // +2 to account for offset
    // Inner window: Border for the board
    WINDOW *inner = newwin(HEIGHT + 2, WIDTH + 2, gameBoardY - 1, gameBoardX);
    box(inner, 0, 0);
    wrefresh(inner);

    // Border for the next window
    WINDOW *nextOut = newwin(6, 6, gameY + 12, gameX + 37);
    box(nextOut, 0, 0);
    wrefresh(nextOut);

    // Next window
    WINDOW *next_window = newwin(4, 4, gameY + 13, gameX + 38);

    // Score window
    WINDOW *score_window = newwin(2, 6, gameY + 4, gameX + 37);
    wprintw(score_window, "%06d", score);
    wrefresh(score_window);

    // +1 to account for offset
    // Board window
    WINDOW *board_window = newwin(HEIGHT, WIDTH, gameBoardY, gameBoardX + 1);

    // Set non blocking input and keypad listener in the board window
    nodelay(board_window, TRUE);
    keypad(board_window, TRUE);

    // Fill grid with void
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            grid[i][j] = 0;
        }
    }

    // Create current and next piece
    nextPiece();
    newPiece();

    // Clock variables to limit game updates
    clock_t last = clock();
    double time_taken = 0;

    // Clock variables to time piece autoscroll
    clock_t t = clock();
    double t_taken = 0;

    // Counter to register the amount of total game updates
    int counter = 0;

    // Use current time as
    // seed for random generator
    srand(time(0));

    // Game Loop
    while (1)
    {
        // Clock variables to compare current time passed since last update
        clock_t now = clock() - last;
        time_taken = ((double)now) / CLOCKS_PER_SEC;

        // Update the game if enough time has passed since last update
        if (time_taken > 0.016)
        {
            // Check for input and update grid accordingly
            int input = wgetch(board_window);

            if (input == KEY_RIGHT)
            {
                movePiece('R');
            }
            else if (input == KEY_LEFT)
            {
                movePiece('L');
            }
            else if (input == KEY_DOWN)
            {
                movePiece('D');
            }
            else if (input == 'X' || input == 'x')
            {
                rotatePiece('A');
            }
            else if (input == 'Z' || input == 'z')
            {
                rotatePiece('B');
            }

            // Clock variables for time passed since last autoscroll
            clock_t t_now = clock() - t;
            t_taken = ((double)t_now) / CLOCKS_PER_SEC;

            // Autoscroll logic
            if (t_taken > 0.5)
            {
                // Check if the piece has space below
                if (collisionY())
                {
                    setPiece();
                    checkLines();
                    newPiece();

                    // Check if the board is topped
                    if (gameOver())
                    {
                        break;
                    }

                    // Reset autoscroll clock
                    t = clock();
                }
                else
                {
                    movePiece('D');
                    t = clock();
                }
            }

            // Update
            updateGrid();

            // Render
            drawGame(board_window, score_window, next_window);

            // Update counter and last update time
            counter++;
            last = clock();
        }
    }

    //End Game
    endwin();
    printf("You lost!\n");
    printf("Frames rendered: %i\n", counter);
    return 0;
}

// Checks if game is over and returns 1 or 0 for true/false
// It gets called after setting a new piece
int gameOver(void)
{
    // If the new piece grid space is already occupied
    // it means that the field has topped out
    if (current.y == 0 && collisionY())
    {
        return 1;
    }

    return 0;
}

// Updates the origin point of the piece accordingly
// Takes a char specifying the direction to move
void movePiece(char direction)
{
    if (direction == 'R')
    {
        // Check if there is space available to the right
        if (collisionX('R'))
        {
            return;
        }
        // Move origin to the right
        current.x++;
    }
    else if (direction == 'L')
    {
        // Check if there is space available to the left
        if (collisionX('L'))
        {
            return;
        }
        // Move origin to the left
        current.x--;
    }
    else if (direction == 'D')
    {
        // Check if there is space available below
        if (collisionY())
        {
            return;
        }
        // Move origin down
        current.y++;
    }
}

// Checks for collision on the X axis, returns 1 or 0 for true/false
// Takes a char specifying right or left
int collisionX(char direction)
{
    // Loop through current piece
    for (int i = 0; i < current.size; i++)
    {
        for (int j = 0; j < current.size; j++)
        {
            if (curPieceGrid[i][j] == 2)
            {
                if (direction == 'R')
                {
                    // Check if parameter is out of bounds
                    if (current.x + j + 1 != WIDTH)
                    {
                        // Check if parameter is available
                        if (grid[current.y + i][current.x + j + 1] == 1)
                        {
                            return 1;
                        }
                    }
                    else
                    {
                        return 1;
                    }
                }
                else if (direction == 'L')
                {
                    // Check if parameter is out of bounds
                    if (current.x + j - 1 != -1)
                    {
                        // Check if parameter is available
                        if (grid[current.y + i][current.x + j - 1] == 1)
                        {
                            return 1;
                        }
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
    }

    // No collision
    return 0;
}

// Checks for collision on the Y axis (only down), returns 1 or 0 for true/false
int collisionY(void)
{
    // Loops through the current piece
    for (int i = 0; i < current.size; i++)
    {
        for (int j = 0; j < current.size; j++)
        {
            if (curPieceGrid[i][j] == 2)
            {
                // Check if parameter is inside of bounds
                if (current.y + i + 1 != HEIGHT)
                {
                    // Check if parameter is available
                    if (grid[current.y + i + 1][current.x + j] == 1)
                    {
                        return 1;
                    }
                }
                else
                {
                    return 1;
                }
            }
        }
    }

    // No collision
    return 0;
}

// Randomly creates next piece
void nextPiece(void)
{
    // Get random piece
    int n = (rand() % 7);
    char shape = shapes[n];

    // Set initial values
    next.x = 4;
    next.y = 0;
    next.size = 3;
    next.shape = shape;

    // Overwrite size if the shape is O or I
    if (shape == 'O')
    {
        next.size = 2;
    }
    else if (shape == 'I')
    {
        next.size = 4;
    }

    updatePieceGrid(nextPieceGrid, next.shape);
}

// Updates current piece for a new one
void newPiece(void)
{
    // Set initial values
    current.x = next.x;
    current.y = next.y;
    current.size = next.size;
    current.shape = next.shape;

    updatePieceGrid(curPieceGrid, current.shape);
    nextPiece();
}

void updatePieceGrid(int pieceGrid[][4], char shape)
{
    // Clear previous pieceGrid
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            pieceGrid[i][j] = 0;
        }
    }

    // Set the new piece coordinates
    if (shape == 'O')
    {
        pieceGrid[0][0] = 2;
        pieceGrid[0][1] = 2;
        pieceGrid[1][0] = 2;
        pieceGrid[1][1] = 2;
    }
    else if (shape == 'I')
    {
        pieceGrid[0][1] = 2;
        pieceGrid[1][1] = 2;
        pieceGrid[2][1] = 2;
        pieceGrid[3][1] = 2;
    }
    else if (shape == 'T')
    {
        pieceGrid[0][1] = 2;
        pieceGrid[1][0] = 2;
        pieceGrid[1][1] = 2;
        pieceGrid[1][2] = 2;
    }
    else if (shape == 'S')
    {
        pieceGrid[0][1] = 2;
        pieceGrid[0][2] = 2;
        pieceGrid[1][0] = 2;
        pieceGrid[1][1] = 2;
    }
    else if (shape == 'Z')
    {
        pieceGrid[0][0] = 2;
        pieceGrid[0][1] = 2;
        pieceGrid[1][1] = 2;
        pieceGrid[1][2] = 2;
    }
    else if (shape == 'J')
    {
        pieceGrid[0][1] = 2;
        pieceGrid[1][1] = 2;
        pieceGrid[2][0] = 2;
        pieceGrid[2][1] = 2;
    }
    else if (shape == 'L')
    {
        pieceGrid[0][1] = 2;
        pieceGrid[1][1] = 2;
        pieceGrid[2][1] = 2;
        pieceGrid[2][2] = 2;
    }
}

// Gets current piece stats and updates the game grid
void updateGrid(void)
{
    // Delete last position (searches all board for the active piece)
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (grid[i][j] == 2)
            {
                grid[i][j] = 0;
            }
        }
    }

    // Draw current position
    for (int i = 0; i < current.size; i++)
    {
        for (int j = 0; j < current.size; j++)
        {
            if (curPieceGrid[i][j] == 2)
            {
                grid[current.y + i][current.x + j] = 2;
            }
        }
    }
}

void rotatePiece(char direction)
{
    coordinates old[4];
    int count = 0;

    // Check if the piece origin is already out of bounds
    if (current.x < 0 || current.x + current.size > WIDTH)
    {
        return;
    }

    // Check if rotation is possible (needs to have the whole box available)
    for (int i = 0; i < current.size; i++)
    {
        for (int j = 0; j < current.size; j++)
        {
            if (grid[current.y + i][current.x + j] == 1)
            {
                return;
            }
            // Save the coordinates to track the rotation
            else if (curPieceGrid[i][j] == 2)
            {
                old[count].y = i;
                old[count].x = j;
                count++;
            }
        }
    }

    // Update the curPieceGrid to rotate the piece to be redrawn
    curPieceGrid[old[0].y][old[0].x] = 0;
    curPieceGrid[old[1].y][old[1].x] = 0;
    curPieceGrid[old[2].y][old[2].x] = 0;
    curPieceGrid[old[3].y][old[3].x] = 0;

    // Direction A for clock-wise rotation (Y2 = X1; X2 = N - Y1)
    if (direction == 'A')
    {
        curPieceGrid[old[0].x][current.size - 1 - old[0].y] = 2;
        curPieceGrid[old[1].x][current.size - 1 - old[1].y] = 2;
        curPieceGrid[old[2].x][current.size - 1 - old[2].y] = 2;
        curPieceGrid[old[3].x][current.size - 1 - old[3].y] = 2;
    }
    // Direction B for counter-clock-wise rotation (Y2 = N - X1; X2 = Y1)
    else if (direction == 'B')
    {
        curPieceGrid[current.size - 1 - old[0].x][old[0].y] = 2;
        curPieceGrid[current.size - 1 - old[1].x][old[1].y] = 2;
        curPieceGrid[current.size - 1 - old[2].x][old[2].y] = 2;
        curPieceGrid[current.size - 1 - old[3].x][old[3].y] = 2;
    }
}

// Adds the current piece to the field
void setPiece(void)
{
    // Loop through the curPieceGrid
    for (int i = 0; i < current.size; i++)
    {
        for (int j = 0; j < current.size; j++)
        {
            // Add the piece to the field
            if (curPieceGrid[i][j] == 2)
            {
                grid[current.y + i][current.x + j] = 1;
            }
        }
    }
}

// Check if the line is cleared
void checkLines(void)
{
    int counter = 0;
    int lines = 0;

    // Loop all the possible cleared lines
    for (int i = 0; i < current.size; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (grid[current.y + i][j] == 1)
            {
                counter++;
            }
        }

        // If a line is completely filled, call clearLine with current line
        if (counter == WIDTH)
        {
            clearLine(current.y + i);
            lines++;
        }

        // Reset counter for each row
        counter = 0;
    }
    if (lines > 0)
    {
        updateScore(lines);
    }
}

// Deletes cleared line and updates the grid
void clearLine(int line)
{
    // Loop backwards from the line to be cleared to the top
    for (int i = line; i >= 0; i--)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (grid[i][j] == 1)
            {
                // Clear the line
                if (i == line)
                {
                    grid[i][j] = 0;
                }
                // Else, move every set piece 1 row down
                else
                {
                    grid[i][j] = 0;
                    grid[i + 1][j] = 1;
                }
            }
        }
    }
}

// Reads number of lines cleared and updates score
void updateScore(int lines)
{
    // Add points to current score
    if (lines == 1)
    {
        score += 40;
    }
    else if (lines == 2)
    {
        score += 100;
    }
    else if (lines == 3)
    {
        score += 300;
    }
    else if (lines == 4)
    {
        score += 1200;
    }

    // Top out score maximum display
    if (score > 999999)
    {
        score = 999999;
    }
}

// Creates the display
void drawGame(WINDOW *board_window, WINDOW *score_window, WINDOW *next_window)
{
    // Clear the windows
    werase(board_window);
    werase(score_window);
    werase(next_window);

    // Loop through all the grid to create the board display as a string
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            // If value isn't 0, it must be the current piece or field
            // Update the board string to display the game
            if (grid[i][j] != 0)
            {
                board[i * WIDTH + j] = '#';
            }
            else
            {
                board[i * WIDTH + j] = ' ';
            }
        }
    }

    // Draw board
    waddstr(board_window, board);
    wrefresh(board_window);

    // Draw score
    wprintw(score_window, "%06d", score);
    wrefresh(score_window);

    // Loop through the next piece to create the next board display as a string
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (nextPieceGrid[i][j] == 2)
            {
                nextBoard[i * 4 + j] = '#';
            }
            else
            {
                nextBoard[i * 4 + j] = ' ';
            }
        }
    }

    // Draw next piece
    waddstr(next_window, nextBoard);
    wrefresh(next_window);
}