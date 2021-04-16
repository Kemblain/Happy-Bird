/********************************************************************
Anthony Nann II
CSC-222-301W - Fall 2018
Final Project
Due December 14, 2018
********************************************************************/

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <time.h>

// Game Constants
const int PLAYER_JUMP = -4;
const int PLAYER_ACCEL = 1;
const int PIPE_SPEED = -10;

const int FPS = 30;
const int FRAME_DELAY = 1000/FPS;

const int GS_SPLASH = 0;
const int GS_RUNNING = 1;
const int GS_GAMEOVER = 2;
const int GS_PAUSED = 3;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int PLAYER_HEIGHT = 61;
const int PLAYER_WIDTH = 72;

const int PIPE_WIDTH = 60;
const int PIPE_HEIGHT = 268;

const int MAX_SPRITE_FRAME = 3;

// SDL Surfaces
SDL_Surface *Backbuffer = NULL;
SDL_Surface *BackgroundImage = NULL;
SDL_Surface *PlayerImage = NULL;
SDL_Surface *UpperPipeImage = NULL;
SDL_Surface *LowerPipeImage = NULL;
SDL_Surface *SplashImage = NULL;
SDL_Surface *GameoverImage = NULL;
SDL_Surface *GamepausedImage = NULL;

// Font
TTF_Font *GameFont = NULL;

// Sound
Mix_Chunk *CollisionSound = NULL;

// Music
Mix_Music *GameMusic = NULL;

// Game Structures
struct Player
{
    SDL_Rect rect;
    int lives;
    int score;
    int velocity;
};

struct PipeSection
{
    SDL_Rect rect;
};

struct Pipe
{
    PipeSection upper;
    PipeSection lower;
};

// Game Variables
Player myPlayer;
Pipe myPipe;
int gameState;
bool hasPassed = false;
int SpriteFrame = 0;
int FrameCounter = 0;

// Function Declarations
SDL_Surface* LoadImage(char* fileName);
void DrawImage(SDL_Surface* image, SDL_Surface* destSurface, int x, int y);
void DrawImageFrame(SDL_Surface* image, SDL_Surface* destSurface, int x, int y, int width, int height, int frame);
void DrawText(SDL_Surface* surface, char* string, int x, int y, TTF_Font* font, Uint8 r, Uint8 g, Uint8 b);
bool ProgramIsRunning();
bool LoadFiles();
void FreeFiles();
bool RectsOverlap(SDL_Rect rect1, SDL_Rect rect2);
bool InitSDL();
bool InitGame();
void UpdatePlayer();
void UpdatePipe(int num);
void UpdateGame(int num);
void DrawGame();
void RunGame(int num);
void FreeGame();
void DrawScreen();
void CheckCollision();
void UpdateSplash();
void UpdateGameOver();
void DrawSplash();
void DrawGameOver();
void DrawGamePaused();

/****************************************************************************************************************************
                MAIN STARTS HERE
****************************************************************************************************************************/
int main(int argc, char *argv[])
{
    if(!InitGame())                                 // If the game doesn't load correctly
    {
        FreeGame();                                 // Release what did load
        return 0;                                   // Exit
    }

    srand(time(NULL));                              // Seed the random number generator

    while(ProgramIsRunning())                       // While the user hasn't quit
    {

        int num = (rand() % 253)+5;                 // Generate a random number

        long int oldTime = SDL_GetTicks();          // Store frame counter to ensure fps is limited correctly
        SDL_FillRect(Backbuffer, NULL, 0);          // Clear the screen
        RunGame(num);                               // Run the logic
        DrawScreen();                               // Draw the new positions

        int frametime = SDL_GetTicks() - oldTime;   // See how long it took since you stored the frame counter
        if (frametime < FRAME_DELAY)                // If it hasn't been long enough
            SDL_Delay(FRAME_DELAY-frametime);       // delay until it has
        SDL_Flip(Backbuffer);                       // update the backbuffer
    }

    FreeGame();                                     // Release everything
    return 0;                                       // Exit
}

/****************************************************************************************************************************
                MAIN ENDS HERE
                Function definitions follow
****************************************************************************************************************************/

//************************************************************
// This function takes a filename and loads the related image*
// processing it and removing the pink background.           *
//************************************************************

SDL_Surface* LoadImage(char* filename)
{
    SDL_Surface* imageLoaded = NULL;
    SDL_Surface* processedImage = NULL;

    imageLoaded = SDL_LoadBMP(filename);

    if(imageLoaded != NULL)
    {
        processedImage = SDL_DisplayFormat(imageLoaded);
        SDL_FreeSurface(imageLoaded);

        if (processedImage != NULL)
        {
            Uint32 colorKey = SDL_MapRGB(processedImage->format, 255, 0, 255);
            SDL_SetColorKey( processedImage, SDL_SRCCOLORKEY, colorKey );
        }
    }

    return processedImage;
}

//************************************************************
// This function draws a static image                        *
//************************************************************

void DrawImage(SDL_Surface* image, SDL_Surface* destSurface, int x, int y)
{
    SDL_Rect destRect;
    destRect.x = x;
    destRect.y = y;

    SDL_BlitSurface( image, NULL, destSurface, &destRect);
}

//************************************************************
// This function draws an animated image from a spritesheet  *
//************************************************************
void DrawImageFrame(SDL_Surface* image, SDL_Surface* destSurface, int x, int y, int width, int height, int frame)
{
    SDL_Rect destRect;
    destRect.x = x;
    destRect.y = y;

    int columns = image->w/width;

    SDL_Rect sourceRect;
    sourceRect.y = (frame/columns)*height;
    sourceRect.x = (frame%columns)*width;
    sourceRect.w = width;
    sourceRect.h = height;

    SDL_BlitSurface( image, &sourceRect, destSurface, &destRect);
}

//************************************************************
// This function writes text                                 *
//************************************************************

void DrawText(SDL_Surface* surface, char* string, int x, int y, TTF_Font* font, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Surface* renderedText = NULL;

    SDL_Color color;

    color.r = r;
    color.g = g;
    color.b = b;

    renderedText = TTF_RenderText_Solid( font, string, color );

    SDL_Rect pos;

    pos.x = x;
    pos.y = y;

    SDL_BlitSurface( renderedText, NULL, surface, &pos );
    SDL_FreeSurface(renderedText);
}

//************************************************************
// This function checks if the user has quit, and if not     *
// updates the game state variable based on what the user    *
// has otherwise done                                        *
//************************************************************

bool ProgramIsRunning()
{
    SDL_Event event;

    bool running = true;

    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_QUIT)
            running = false;

        if( event.type == SDL_KEYDOWN )
        {
            if(event.key.keysym.sym == SDLK_ESCAPE)
            {
                 if(gameState == GS_RUNNING)
                    gameState = GS_PAUSED;
                else if(gameState == GS_PAUSED)
                    gameState = GS_RUNNING;
            }
        }
    }

    return running;
}

//************************************************************
// This function loads all the requisite files and verifies  *
// that they have loaded correctly                           *
//************************************************************

bool LoadFiles()
{
    // Images
    BackgroundImage = LoadImage("Graphics/background.bmp");
    PlayerImage = LoadImage("Graphics/player.bmp");
    UpperPipeImage = LoadImage("Graphics/upperpipe.bmp");
    LowerPipeImage = LoadImage("Graphics/lowerpipe.bmp");
    GameoverImage = LoadImage("Graphics/gameover.bmp");
    GamepausedImage = LoadImage("Graphics/gamepaused.bmp");
    SplashImage = LoadImage("Graphics/splash.bmp");

    if (BackgroundImage == NULL)
        return false;
    if (PlayerImage == NULL)
        return false;
    if (UpperPipeImage == NULL)
        return false;
    if (LowerPipeImage == NULL)
        return false;
    if (GameoverImage == NULL)
        return false;
    if (GamepausedImage == NULL)
        return false;
    if (SplashImage == NULL)
        return false;

    // Font
    GameFont = TTF_OpenFont("Graphics/MotorwerkOblique.ttf", 40);

    if (GameFont == NULL)
        return false;


    // Sounds
    CollisionSound = Mix_LoadWAV("Audio/collision.wav");
    GameMusic = Mix_LoadMUS("Audio/song.mp3");

    if(CollisionSound == NULL)
        return false;
    if(GameMusic == NULL)
        return false;

    return true;
}

//************************************************************
// This function releases all the loaded files               *
//************************************************************

void FreeFiles()
{
    SDL_FreeSurface(BackgroundImage);
    SDL_FreeSurface(PlayerImage);
    SDL_FreeSurface(UpperPipeImage);
    SDL_FreeSurface(LowerPipeImage);
    SDL_FreeSurface(GameoverImage);
    SDL_FreeSurface(GamepausedImage);
    SDL_FreeSurface(SplashImage);

    TTF_CloseFont(GameFont);

    Mix_FreeChunk(CollisionSound);
    Mix_FreeMusic(GameMusic);
}

//*************************************************************
// This function checks if two SDL_Rect objects overlap at all*
//*************************************************************

bool RectsOverlap(SDL_Rect rect1, SDL_Rect rect2)
{
    if(rect1.x >= rect2.x+rect2.w)
        return false;

    if(rect1.y >= rect2.y+rect2.h)
        return false;

    if(rect2.x >= rect1.x+rect1.w)
        return false;

    if(rect2.y >= rect1.y+rect1.h)
        return false;

    return true;
}

//************************************************************
// This function loads the portions of SDL that are used     *
//************************************************************

bool InitSDL()
{
    if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
        return false;

    //Init audio subsystem
    if(Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 2048 ) == -1)
    {
        return false;
    }

    //Init TTF subsystem
    if(TTF_Init() == -1)
    {
        return false;
    }

    //Generate screen
    Backbuffer = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE );

    //Error check Backbuffer
    if(Backbuffer == NULL)
        return false;

    return true;
}

//************************************************************
// This function loads the SDL and the files, sets up all of *
// the game variables, and then opens the window             *
//************************************************************

bool InitGame()
{
    if(!InitSDL())
        return false;

    if(!LoadFiles())
        return false;

    myPlayer.score = 0;
    myPlayer.lives = 5;
    myPlayer.velocity = -10;
    myPlayer.rect.x = (SCREEN_WIDTH/3);
    myPlayer.rect.y = (SCREEN_HEIGHT/2) - 25;
    myPlayer.rect.h = PLAYER_HEIGHT;
    myPlayer.rect.w = PLAYER_WIDTH;
    myPipe.upper.rect.h = PIPE_HEIGHT;
    myPipe.upper.rect.w = PIPE_WIDTH;
    myPipe.lower.rect.h = PIPE_HEIGHT;
    myPipe.lower.rect.w = PIPE_WIDTH;
    myPipe.lower.rect.x = SCREEN_WIDTH;
    myPipe.upper.rect.x = SCREEN_WIDTH;
    myPipe.lower.rect.y = SCREEN_HEIGHT - 100;
    myPipe.upper.rect.y = myPipe.lower.rect.y - (300 + PIPE_HEIGHT);

    SDL_WM_SetCaption("Happy Bird", NULL);
    Mix_PlayMusic(GameMusic, -1);
    gameState = GS_SPLASH;
    return true;
}

//************************************************************
// This function updates the players position and handles the*
// sprite animation.                                         *
//************************************************************

void UpdatePlayer()
{
    Uint8 *keys = SDL_GetKeyState(NULL);

    FrameCounter++;

    if(FrameCounter > 2)
    {
        FrameCounter = 0;
        SpriteFrame++;
    }

    if(SpriteFrame > MAX_SPRITE_FRAME)
        SpriteFrame = 0;

    if(keys[SDLK_SPACE])
        myPlayer.velocity += PLAYER_JUMP;

    myPlayer.velocity += PLAYER_ACCEL;

    myPlayer.rect.y += myPlayer.velocity;
}

//************************************************************
// This function handles the pipes movement and resetting,   *
// and resets the hasPassed variable that is used in keeping *
// score                                                     *
//************************************************************

void UpdatePipe(int num)
{

    myPipe.upper.rect.x += PIPE_SPEED;
    myPipe.lower.rect.x += PIPE_SPEED;

    if(myPipe.upper.rect.x + PIPE_WIDTH <= 0)
    {
        hasPassed = false;

        myPipe.lower.rect.x = SCREEN_WIDTH;
        myPipe.upper.rect.x = SCREEN_WIDTH;

        myPipe.upper.rect.y = -num;
        myPipe.lower.rect.y = myPipe.upper.rect.y + 320 + 268;
    }
}

//************************************************************
// This function calls the updater for the player, the pipe, *
// checks if there is a collision, and then changes the game *
// state if the player has lost all their lives              *
//************************************************************

void UpdateGame(int num)
{
    UpdatePlayer();
    UpdatePipe(num);
    CheckCollision();

    if(myPlayer.lives <= 0)
        gameState = GS_GAMEOVER;
}

//************************************************************
// This function handles which updating function to call     *
// based on the gamestate variable                           *
//************************************************************

void RunGame(int num)
{
    switch(gameState)
    {
        case GS_SPLASH:
            UpdateSplash();
            break;
        case GS_RUNNING:
            UpdateGame(num);
            break;
        case GS_GAMEOVER:
            UpdateGameOver();
            break;
        default:
            break;
    }
}

//************************************************************
// This function handles which draw functions to call based  *
// on the gamestate variable                                 *
//************************************************************

void DrawScreen()
{
    switch(gameState)
    {
    case GS_SPLASH:
        DrawSplash();
        break;
    case GS_RUNNING:
        DrawGame();
        break;
    case GS_GAMEOVER:
        DrawGameOver();
        break;
    case GS_PAUSED:
        DrawGamePaused();
        break;
    default:
        break;
    }
}

//************************************************************
// This function releases everything the SDL is using        *
//************************************************************

void FreeGame()
{
    Mix_HaltMusic();
    FreeFiles();
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
}

//************************************************************
// This function resets some game variables for a reset game *
//************************************************************

void UpdateSplash()
{
    Uint8 *keys = SDL_GetKeyState(NULL);

    if(keys[SDLK_RETURN])
    {
        myPlayer.score = 0;
        myPlayer.lives = 5;
        myPlayer.velocity = -10;
        myPlayer.rect.x = (SCREEN_WIDTH/3);
        myPlayer.rect.y = (SCREEN_HEIGHT/2) - 25;
        myPipe.upper.rect.x = SCREEN_WIDTH;
        myPipe.lower.rect.x = SCREEN_WIDTH;
        gameState = GS_RUNNING;
    }
}

//************************************************************
// This function allows the user to exit the game over screen*
//************************************************************

void UpdateGameOver()
{
    Uint8 *keys = SDL_GetKeyState(NULL);

    if(keys[SDLK_SPACE])
    {
        gameState = GS_SPLASH;
    }
}

//************************************************************
// This function draws the new game screen                   *
//************************************************************

void DrawSplash()
{
    DrawImage(SplashImage, Backbuffer, 0, 0);
}

//************************************************************
// This function draws the game over screen                  *
//************************************************************

void DrawGameOver()
{
    DrawGame();
    DrawImage(GameoverImage, Backbuffer, 0, 0);
}

//************************************************************
// This function draws the game paused screen                *
//************************************************************

void DrawGamePaused()
{
    DrawGame();
    DrawImage(GamepausedImage, Backbuffer, 0, 0);
}

//************************************************************
// This function draws the screen while the game is active,  *
// including the player, the pipe, and the textboxes         *
//************************************************************

void DrawGame()
{
    DrawImage(BackgroundImage, Backbuffer, 0, 0);
    DrawImageFrame(PlayerImage, Backbuffer, myPlayer.rect.x, myPlayer.rect.y, PLAYER_WIDTH, PLAYER_HEIGHT, SpriteFrame);
    DrawImage(UpperPipeImage, Backbuffer, myPipe.upper.rect.x, myPipe.upper.rect.y);
    DrawImage(LowerPipeImage, Backbuffer, myPipe.lower.rect.x, myPipe.lower.rect.y);

    char livesText[64];
    char scoreText[64];

    sprintf(livesText, "Lives: %d", myPlayer.lives);
    sprintf(scoreText, "Score: %d", myPlayer.score);

    DrawText(Backbuffer, livesText, 40, 40, GameFont, 0, 0, 0);
    DrawText(Backbuffer, scoreText, 40, 65, GameFont, 0, 0, 0);
}

//************************************************************
// This function checks if the player has collided with the  *
// ceiling, the ground, or the pipe, and then adds to the    *
// player score when the pipe passes the player              *
//************************************************************

void CheckCollision()
{
    if(myPlayer.rect.y <=0)
    {
        myPlayer.rect.y = 0;
        myPlayer.velocity = 0;
    }

    if (myPlayer.rect.y >= (SCREEN_HEIGHT - PLAYER_HEIGHT))
    {
        Mix_PlayChannel(-1, CollisionSound, 0);
        myPlayer.lives--;
        myPlayer.velocity = -10;
        myPlayer.rect.y = (SCREEN_HEIGHT - PLAYER_HEIGHT)/2;
        myPlayer.score--;
        myPipe.upper.rect.x = SCREEN_WIDTH;
        myPipe.lower.rect.x = SCREEN_WIDTH;
    }

    if (RectsOverlap(myPlayer.rect, myPipe.lower.rect))
    {
        Mix_PlayChannel(-1, CollisionSound, 0);
        myPlayer.lives--;
        myPlayer.velocity = -10;
        myPlayer.rect.y = (SCREEN_HEIGHT - PLAYER_HEIGHT)/2;
        myPlayer.score--;
        myPipe.upper.rect.x = SCREEN_WIDTH;
        myPipe.lower.rect.x = SCREEN_WIDTH;
    }

    if (RectsOverlap(myPlayer.rect, myPipe.upper.rect))
    {
        Mix_PlayChannel(-1, CollisionSound, 0);
        myPlayer.lives--;
        myPlayer.velocity = -10;
        myPlayer.rect.y = (SCREEN_HEIGHT - PLAYER_HEIGHT)/2;
        myPlayer.score--;
        myPipe.upper.rect.x = SCREEN_WIDTH;
        myPipe.lower.rect.x = SCREEN_WIDTH;
    }

    if(!hasPassed)
    {
        if (myPlayer.rect.x > (myPipe.upper.rect.x + myPipe.upper.rect.w))
        {
            hasPassed = true;
            myPlayer.score++;
        }
    }
}
