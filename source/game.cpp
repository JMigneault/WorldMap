#include "game.h"
#include "..\libraries\SDL2\include\SDL.h"
#include <stdio.h>
// for memset
#include <cstring>

// AABBGGRR: NOTE: alpha is "opaqueness" so must be set to 255 to see colors
static const int dark = (0xFF << 24) | (0x00 << 16) | (0x00 << 8) | (0xFF << 0);
static const int light = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0x00 << 0);
static const int cutscene = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);

static const int highlightColor = (0x55 << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);

// todo: will need object representation for player eventually (to manage sprites for example)
static const int playerColor = (0xFF << 24) | (0xA0 << 16) | (0x00 << 8) | (0xA0 << 0);

// TEMP: eventually should be determined by art assets themselves
// in the meantime set at resolution of a tile for a 16 x 9 tilemap on 1920x1080 display
static const int tileResolution = 120;

// TODO: allow sources to have multiple sinks and detect (and disallow) the mixing of sources
// TODO: allow sources to be special tiles
int main(int argc, char *argv[]) {

    struct GameState game;
    struct Engine engine;

    initGameState(&game);
    initEngine(&engine);

    SDL_Surface *cutsceneSurface = DebugMakeRectSurface(engine.screenWidth, engine.screenHeight, cutscene);
    SDL_Texture *cutsceneTexture = SDL_CreateTextureFromSurface(engine.renderer, cutsceneSurface);

    // TEMP: for making example puzzle
    game.mode = puzzleMode;

    // START TEMP LOADING CODE
    // TEMP: build some other entities for testing
    SDL_Surface *entitySurface = DebugMakeRectSurface(.5 * tileResolution, .5 * tileResolution, cutscene);
    SDL_Texture *entityTexture = SDL_CreateTextureFromSurface(engine.renderer, entitySurface);
    game.entities[1] = new Entity;
    game.entities[1]->id = 2;
    game.entities[1]->position = v2{9, 5};
    game.entities[1]->velocity = v2{-1.0, 0};
    game.entities[1]->width = .5;
    game.entities[1]->height = .5;
    game.entities[1]->speed = .5;
    game.entities[1]->texture = entityTexture;
    game.numEntities = 2;

    // Make textures for background and player
    SDL_Surface *lightSurface = DebugMakeRectSurface(roundFloatToInt(tileResolution), roundFloatToInt(tileResolution), light);
    SDL_Surface *darkSurface = DebugMakeRectSurface(roundFloatToInt(tileResolution), roundFloatToInt(tileResolution), dark);
    SDL_Texture *lightTexture = SDL_CreateTextureFromSurface(engine.renderer, lightSurface);
    SDL_Texture *darkTexture = SDL_CreateTextureFromSurface(engine.renderer, darkSurface);
    // TODO: make this less messy, right now we need to loop through each tile again to add textures
    for (int i = 0; i < game.worldTilemap->tilesVert; i++) {
        for (int j = 0; j < game.worldTilemap->tilesHorz; j++) {
            WorldTile *tile = game.worldTilemap->get(i, j);
            if (tile->blocked) {
                tile->texture = darkTexture;
            } else {
                tile->texture = lightTexture;
            }
        }
    }

    // TEMP: add a town for testing
    Town *town = new Town;
    game.worldTilemap->get(1, 1)->town = town;
    town->position = v2{1.5, 1.5};
    town->width = .5;
    town->height = .5;
    town->texture = entityTexture;
    town->id = 3;
    game.entities[2] = town;
    game.numEntities = 3;

    SDL_Surface *playerSurface = DebugMakeRectSurface(game.player()->width * tileResolution, game.player()->height * tileResolution, playerColor);
    SDL_Texture *playerTexture = SDL_CreateTextureFromSurface(engine.renderer, playerSurface);
    game.player()->texture = playerTexture;

    // TEMP: bind highlight texture
    SDL_Surface *highlightSurface = DebugMakeRectSurface(roundFloatToInt(tileResolution), roundFloatToInt(tileResolution), highlightColor);
    SDL_Texture *highlightTexture = SDL_CreateTextureFromSurface(engine.renderer, highlightSurface);

    // TEMP: initialize example puzzle
    // TODO: make pipes respect the borderSize pixel boundary for tiles
    for (int i = 0; i < game.puzzleTilemap->tilesVert; i++) {
        for (int j = 0; j < game.puzzleTilemap->tilesHorz; j++) {
            int reactorColor = (0xFF << 24) | (0x00 << 16) | (0x00 << 8) | (0xFF << 0);
            int coolerColor = (0xFF << 24) | (0xFF << 16) | (0x00 << 8) | (0x00 << 0);
            int backgroundColor = (0xFF << 24) | (0x55 << 16) | (0x10 << 8) | (0x30 << 0);
            int pipeColor = (0xFF << 24) | (0x00 << 16) | (0xFF << 8) | (0x00 << 0);
            int cappedColor = (0xFF << 24) | (0x10 << 16) | (0x50 << 8) | (0x10 << 0);
            int borderSize = 3;

            PuzzleTile *tile = game.puzzleTilemap->get(i,j);
            // Start w/ black background
            // TODO (?): write border at the end instead of the beginning
            SDL_Surface *surface;
            if (tile->isSpecial) {
                int specialColor;
                if (tile->isCooler) {
                    specialColor = coolerColor;
                } else {
                    specialColor = reactorColor;
                }
                surface = DebugMakeRectSurface(tileResolution, tileResolution, specialColor);
            } else {
                // write background leaving black border
                surface = DebugMakeRectSurface(tileResolution, tileResolution, backgroundColor);
                SDL_Surface *pipeSource = DebugMakeRectSurface(tileResolution / 3.0, tileResolution / 3.0, pipeColor);
                SDL_Surface *cappedSource = DebugMakeRectSurface(tileResolution / 3.0, tileResolution / 3.0, cappedColor);
                int pieceWidth = tileResolution / 3.0;
                int pieceHeight = tileResolution / 3.0;

                SDL_Rect rect;
                // write to middle
                if (tile->existsLeft || tile->existsUp || tile->existsRight || tile->existsDown) {
                    rect.x = pieceWidth;
                    rect.y = pieceHeight;
                    rect.w = pieceWidth;
                    rect.h = pieceHeight;
                    SDL_BlitSurface(pipeSource, NULL, surface, &rect);
                }
                if (tile->existsLeft) {
                    rect.x = borderSize;
                    rect.y = pieceHeight;
                    rect.w = pieceWidth;
                    rect.h = pieceHeight;
                    if (tile->cappedLeft) {
                        SDL_BlitSurface(cappedSource, NULL, surface, &rect);
                    } else {
                        SDL_BlitSurface(pipeSource, NULL, surface, &rect);
                    }
                }
                if (tile->existsUp) {
                    rect.x = pieceHeight;
                    rect.y = borderSize;
                    rect.w = pieceWidth;
                    rect.h = pieceHeight;
                    if (tile->cappedUp) {
                        SDL_BlitSurface(cappedSource, NULL, surface, &rect);
                    } else {
                        SDL_BlitSurface(pipeSource, NULL, surface, &rect);
                    }
                }
                if (tile->existsRight) {
                    rect.x = 2 * pieceWidth; // -1 for off by one (rounding?) error
                    rect.y = pieceHeight;
                    rect.w = pieceWidth;
                    rect.h = pieceHeight;
                    if (tile->cappedRight) {
                        SDL_BlitSurface(cappedSource, NULL, surface, &rect);
                    } else {
                        SDL_BlitSurface(pipeSource, NULL, surface, &rect);
                    }
                }
                if (tile->existsDown) {
                    rect.x = pieceWidth;
                    rect.y = 2 * rect.h;
                    rect.w = pieceWidth;
                    rect.h = pieceHeight;
                   if (tile->cappedDown) {
                        SDL_BlitSurface(cappedSource, NULL, surface, &rect);
                    } else {
                        SDL_BlitSurface(pipeSource, NULL, surface, &rect);
                    }
                }
            }

            // write border
            int borderColor;
            if (!tile->movable) {
                borderColor = 0xFF << 24;
            } else {
                // white
                borderColor = cutscene;
            }
            SDL_Surface *vertBorder = DebugMakeRectSurface(borderSize, tileResolution, borderColor);
            SDL_Surface *horzBorder = DebugMakeRectSurface(tileResolution, borderSize, borderColor);
            // draw vertical borders
            SDL_Rect rect;
            rect.x = 0;
            rect.y = 0;
            rect.w = borderSize;
            rect.h = tileResolution;
            SDL_BlitSurface(vertBorder, NULL, surface, &rect);
            rect.x = tileResolution - borderSize;
            SDL_BlitSurface(vertBorder, NULL, surface, &rect);
            // draw horizontal borders
            rect. x = 0;
            rect.w = tileResolution;
            rect.h = borderSize;
            SDL_BlitSurface(horzBorder, NULL, surface, &rect);
            rect.y = tileResolution - borderSize;
            SDL_BlitSurface(horzBorder, NULL, surface, &rect);

            tile->texture = SDL_CreateTextureFromSurface(engine.renderer, surface);
        }
    }
 
    // END TEMP LOADING CODE

    while(true) {
        // main loop
        Uint64 frameStartCounter = SDL_GetPerformanceCounter();

        bool quit = false;

        // TEMP: institute puzzle checking
        bool checkPuzzle = false;

        // handle input
        // TODO: make input handling generic
        SDL_Event event;
        bool foundEvent = true;
        while (foundEvent) {
            foundEvent = SDL_PollEvent(&event);
            if (foundEvent) {
                switch(event.type) {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    case SDL_KEYDOWN:
                        switch(event.key.keysym.sym) {
                            case SDLK_w:
                                game.player()->velocity.y = -1;
                                break;
                            case SDLK_a:
                                game.player()->velocity.x = -1;
                                break;
                            case SDLK_s:
                                game.player()->velocity.y = 1;
                                break;
                            case SDLK_d:
                                game.player()->velocity.x = 1;
                                break;
                            // TEMP: Close cutscene window
                            case SDLK_f:
                                game.mode = worldMode;
                                break;
                            // TEMP: check puzzle when p is pressed
                            case SDLK_p:
                                checkPuzzle = true;
                                break;
                       }
                       break;
                    case SDL_KEYUP:
                         switch(event.key.keysym.sym) {
                            case SDLK_w:
                                if (game.player()->velocity.y < 0) {
                                    game.player()->velocity.y = 0;
                                }
                                break;
                            case SDLK_a:
                                if (game.player()->velocity.x < 0) {
                                    game.player()->velocity.x = 0;
                                }
                                break;
                            case SDLK_s:
                                if (game.player()->velocity.y > 0) {
                                    game.player()->velocity.y = 0;
                                }
                                break;
                            case SDLK_d:
                                if (game.player()->velocity.x > 0) {
                                    game.player()->velocity.x = 0;
                                }
                                break;
                        }
                        break;              
                    case SDL_MOUSEBUTTONDOWN:
                        switch(event.button.button) {
                            case SDL_BUTTON_LEFT:
                                // select or switch tile
                                if (game.mode == puzzleMode) {
                                    // todo
                                    // TODO (?): camera to world space function 
                                    int col = event.button.x / engine.unitWidth() - engine.activeCamera->position.x;
                                    int row = event.button.y / engine.unitHeight() - engine.activeCamera->position.y;
                                    PuzzleTile *tile = game.puzzleTilemap->get(row, col);
                                    if (tile->movable) {
                                        if (!game.selection.hasSelection) {
                                            game.selection.hasSelection = true;
                                            game.selection.coords = pair{row, col};
                                        } else {
                                            pair first = game.selection.coords;
                                            pair second = pair{row, col};
                                            if (canMoveTo(first, second, game.puzzleTilemap) 
                                                && canMoveTo(second, first, game.puzzleTilemap)) {
                                                game.selection.hasSelection = false;
                                                PuzzleTile temp = *tile;
                                                game.puzzleTilemap->set(row, col, game.puzzleTilemap->get(game.selection.coords.r, game.selection.coords.c));
                                                game.puzzleTilemap->set(game.selection.coords.r, game.selection.coords.c, &temp);
                                            }
                                        }
                                    }
                                }
                                break;
                        }
                        break;
                }
            }
        }

        if (quit) {
            break;
        }

        switch (game.mode) {
            case worldMode:
                engine.activeCamera = &game.worldCamera;
                worldModeUpdate(&game, &engine);
                game.worldTilemap->renderTilemap(&engine);
                drawEntities(&engine, game.entities, game.numEntities);
                break;
            case puzzleMode:
                engine.activeCamera = &game.puzzleCamera;
                game.puzzleTilemap->renderTilemap(&engine);

                // Highlight selected tile
                if (game.selection.hasSelection) {
                    SDL_Rect destRect;
                    destRect.x = roundFloatToInt(((float)game.selection.coords.c - engine.activeCamera->position.x) * engine.unitWidth());
                    destRect.y = roundFloatToInt(((float)game.selection.coords.r - engine.activeCamera->position.y) * engine.unitHeight());
                    destRect.w = roundFloatToInt(engine.unitWidth());
                    destRect.h = roundFloatToInt(engine.unitHeight());
                    SDL_RenderCopy(engine.renderer, highlightTexture, NULL, &destRect);
                }

                if (checkPuzzle) {
                    bool validSolution = true;
                    for (int i = 0; i < game.numSourceSinks; i++) {
                        validSolution = validSolution && hasSourceSinkPath(game.sources[i], game.sinks[i], game.puzzleTilemap);
                    }
                    if (validSolution) {
                        printf("Succesfully hit sink\n");
                    } else {
                        printf("Failed to hit sink\n");
                    }
                }
                break;
        }

        // wait to target framerate
        Uint64 waitStartCounter = SDL_GetPerformanceCounter(); 
        
        // put thread to sleep for the amount of time until the target leaving 2ms to spare
        int waitTimeTargetMS = (int)(1000 * (engine.frametimeTarget - ((double)(waitStartCounter - frameStartCounter) / (double)SDL_GetPerformanceFrequency())) - 2);
        if (waitTimeTargetMS > 0) {
            SDL_Delay(waitTimeTargetMS);
        }

        // display screen and then explicitly wait until we hit the frame target
        Uint64 waitEndCounter = SDL_GetPerformanceCounter();
        Uint64 frameEndCounter = (Uint64)(engine.frametimeTarget * (float)SDL_GetPerformanceFrequency()) + frameStartCounter;
        SDL_RenderPresent(engine.renderer);
        while (SDL_GetPerformanceCounter() < frameEndCounter) { }
    }

    // cleanup allocated memory
    for (int i = 0; i < game.numEntities; i++) {
        delete game.entities[i];
    }

    SDL_FreeSurface(lightSurface);
    SDL_FreeSurface(darkSurface);
    SDL_FreeSurface(playerSurface);
    SDL_FreeSurface(entitySurface);
    SDL_FreeSurface(cutsceneSurface);
    SDL_DestroyTexture(lightTexture);
    SDL_DestroyTexture(darkTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(entityTexture);
    SDL_DestroyTexture(cutsceneTexture);
    SDL_DestroyRenderer(engine.renderer);
    SDL_DestroyWindow(engine.window);
    SDL_Quit();

	return 0;
}

void initEngine(Engine *engine) {
    // initialize Engine struct
    engine->screenWidth = 1920;
    engine->screenHeight = 1080;

    // initialize SDL and make a window
    SDL_Init(SDL_INIT_VIDEO);
    engine->window = SDL_CreateWindow("WorldMap", 100, 100, engine->screenWidth, engine->screenHeight, SDL_WINDOW_FULLSCREEN);
    engine->renderer = SDL_CreateRenderer(engine->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // find refresh rate to set target framerate
    SDL_DisplayMode display;
    
    if (SDL_GetCurrentDisplayMode(0, &display) == 0) {
        engine->refreshRate = display.refresh_rate;
    } else {
        engine->refreshRate = 60;
    }
    engine->frametimeTarget = 1.0 / (float)engine->refreshRate;
}

void initGameState(GameState *game) {

    // initialize cameras
    game->worldCamera.height = 4.5; //9.0;
    game->worldCamera.width = 8.0; //16.0;
    // world camera position should be set to player position before first render anyways
    game->worldCamera.position = v2{0,0};

    game->puzzleCamera.height = 9.0;
    game->puzzleCamera.width = 16.0;
    game->puzzleCamera.position = v2{0,0};

    // initialize World struct
    int worldTemplate[9][16] = {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 0, 1, 0,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 0,  0, 0, 0, 1,  1, 1, 1, 0,  0, 0, 1, 1},

        {1, 1, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 1,  0, 0, 1, 1},

        {1, 1, 0, 0,  0, 0, 1, 0,  0, 0, 1, 0,  0, 0, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 1,  0, 1, 0, 0,  0, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 1,  1, 1, 1, 1},
    };

    // explicitly define tilemap for now
    // TODO: refactor and malloc for tilemap
    Tilemap<WorldTile> *worldTilemap = new Tilemap<WorldTile>;
    // TODO: malloc array
    game->worldTilemap = worldTilemap;
    worldTilemap->tilesVert = 9;
    worldTilemap->tilesHorz = 16;
    worldTilemap->tiles = new WorldTile[worldTilemap->tilesVert * worldTilemap->tilesHorz];
    for (int i = 0; i < worldTilemap->tilesVert; i++) {
        for (int j = 0; j < worldTilemap->tilesHorz; j++) {
            WorldTile *tile = worldTilemap->get(i, j);
            tile->blocked = worldTemplate[i][j];
            tile->town = NULL;
        }
    }

    // Create the player
    Player *player = new Player;
    player->id = 0;
    player->position = v2{8, 4.5};
    player->velocity = v2{0, 0};
    player->width = .5;
    player->height = .5;
    player->speed = 1.5;
    game->entities[0] = player;
    game->numEntities = 1;
    // TODO: free memory
    // TODO: add differentiation between movable tiles and start adding functionality
    int puzzleTemplate[9][16] = {
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 20, 0, 0,    0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 20, 0, 42,  10, 154 + 512, 10, 10},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 20, 0, 0,    0, 20, 0, 0},

        {0, 0, 0, 0,      0, 84 + 256, 0, 0,      1, 1, 10, 10,  3, 3, 10, 10},
        {10, 10, 26 + 512, 10,  10, 286 + 512, 10, 10,  1, 1, 10, 10,  3, 3, 0, 0},
        {0, 0, 20, 0,     0, 20, 0, 0,       0, 0, 0, 0,    20, 0, 0, 0},

        {0, 0, 12 + 512, 10,  10, 94 + 512, 10, 10,  10, 10, 10, 10,  6, 0, 0, 0},
        {0, 0, 0, 0,    0, 276 + 64, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0},
        {0, 0, 0, 0,    0, 0, 0, 0,       0, 0, 0, 0,      0, 0, 0, 0},
    };

    Tilemap<PuzzleTile> *puzzleTilemap = new Tilemap<PuzzleTile>;
    game->puzzleTilemap = puzzleTilemap;
    puzzleTilemap->tilesVert = 9;
    puzzleTilemap->tilesHorz = 16;
    puzzleTilemap->tiles = new PuzzleTile[puzzleTilemap->tilesHorz * puzzleTilemap->tilesVert];

    game->sources = new pair[3];
    game->sources[0] = pair{2, 13};
    game->sources[1] = pair{4, 0};
    game->sources[2] = pair{4, 0};
    game->sinks = new pair[3];
    game->sinks[0] = pair{1, 15};
    game->sinks[1] = pair{4, 7};
    game->sinks[2] = pair{6, 12};
    game->numSourceSinks = 3;

    for (int i = 0; i < puzzleTilemap->tilesVert; i++) {
        for (int j = 0; j < puzzleTilemap->tilesHorz; j++) {
            // Build the tile from its code by explicitly calling its constructor
            PuzzleTile *tile = puzzleTilemap->get(i, j);
            tile->indices = pair{i,j};
            tile->parseCode(puzzleTemplate[i][j]);
        }
    }

    // TEMP: set highlighted tile
    // NOTE: default "no tile selected" square is -1, -1
    game->selection.hasSelection = false;
}

// Returns the minimum time between inputted dT and the time to one of the 4 walls of the object
// we are testing for collision against. If return value < dT we collided with a the object.
float testCollision(float dT, Entity *colliding, v2 testPosition, float testWidth, float testHeight) {
    // TODO: clean up this code; make a generic edge checking function and check each edge w/ 4 function calls
    // TODO: could instead return a "collision" struct with dT and other info (or NULL if no collision)
    v2 velVector = colliding->speed * colliding->velocity.normalized();
    // minkowski collision says we can add the radius of our colliding object to our object and then
    // do raycasting
    float lBound = testPosition.x - (testWidth + colliding->width) / 2.0;
    float rBound = testPosition.x + (testWidth + colliding->width) / 2.0;
    float tBound = testPosition.y - (testHeight + colliding->height) / 2.0;
    float bBound = testPosition.y + (testHeight + colliding->height) / 2.0;
    float minT = dT;
    // left vert edge
    if (velVector.x != 0) {
        float x1T = (lBound - colliding->position.x) / velVector.x;
        if (x1T > 0 && colliding->position.y + x1T * velVector.y >= tBound && colliding->position.y + x1T * velVector.y <= bBound) {
            minT = x1T; 
        }
    }
    // right vert edge
    if (velVector.x != 0) {
        float x2T = (rBound - colliding->position.x) / velVector.x;
        if (x2T > 0 && colliding->position.y + x2T * velVector.y >= tBound && colliding->position.y + x2T * velVector.y <= bBound) {
            minT = min(minT, x2T); 
        }
    }
    // top horz edge
    if (velVector.y != 0) {
        float y1T = (tBound - colliding->position.y) / velVector.y;
        if (y1T > 0 && colliding->position.x + y1T * velVector.x >= lBound && colliding->position.x + y1T * velVector.x <= rBound) {
            minT = min(minT, y1T); 
        }
    }
    // bottom horz edge
    if (velVector.y != 0) {
        float y2T = (bBound - colliding->position.y) / velVector.y;
        if (y2T > 0 && colliding->position.x + y2T * velVector.x >= lBound && colliding->position.x + y2T * velVector.x <= rBound) {
            minT = min(minT, y2T); 
        }
    }
    return minT;

}

// debug function that makes a surface for a colored (AABBGGRR) rectangle
SDL_Surface *DebugMakeRectSurface(int w, int h, int color) {
    SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 32,
                                                 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    SDL_LockSurface(surface);
    for (int i = 0; i < w * h; i++) {
        *((int *)(surface->pixels) + i) = color;
    }
    SDL_UnlockSurface(surface);

    return surface; 
}


void worldModeUpdate(GameState *game, Engine *engine) {

        // update player position
        // TODO: update by actual frametime not predicted frametime?
        // TODO: improve collision detection
        // strategy: check all 4 walls of all frames in min - max x/y of P0 -> P1 vector
        // solve for time of collision for each wall
        // takes smallest time (min w/ full move as well) and set position to be position 
        // at that time (need epsilon?)

        // Player -> tilemap collision
        v2 newPlayerPos = game->player()->position + game->player()->speed * engine->frametimeTarget * game->player()->velocity.normalized();
        // NOTE: adding player dimension only to maxes is (maybe) not strictly correct

        float vertExtrude = game->player()->height / 2.0;
        float horzExtrude = game->player()->width / 2.0;

        int minRow = (int)min(game->player()->position.y - vertExtrude, newPlayerPos.y - vertExtrude);
        int maxRow = (int)max(game->player()->position.y + vertExtrude, newPlayerPos.y + vertExtrude);
        int minCol = (int)min(game->player()->position.x - horzExtrude, newPlayerPos.x - horzExtrude);
        int maxCol = (int)max(game->player()->position.x + horzExtrude, newPlayerPos.x + horzExtrude);

        float minT = engine->frametimeTarget;

        for (int i = minRow; i < maxRow + 1; i++) {
            for (int j = minCol; j < maxCol + 1; j++) {
                if (game->worldTilemap->get(i, j)->blocked) {
                    float collisionT = testCollision(minT, game->player(), v2{(float)j + 0.5f, (float)i + 0.5f}, 1.0f, 1.0f);
                    if (collisionT < minT) {
                        // we collided so we set time to collision time
                        minT = collisionT;
                    }
                }
                // check if we hit a town (NOTE: technically this check should go after we determine final minT, but this should be equivalent
                if ((game->worldTilemap->get(i, j))->town != NULL) {
                    Town *town = game->worldTilemap->get(i, j)->town;
                    float collisionT = testCollision(minT, game->player(), town->position, town->width, town->height);
                    if (collisionT < minT && !hasOverlap(game->player()->position, game->player()->width, game->player()->height, town->position, town->width, town->height)) {
                        game->mode = puzzleMode;
                    }
                }

            }
        }

        // Player -> entity and entity->player collision
        // brute force check for collision between player and all other entities
        for (int i = 1; i < game->numEntities; i++) {
            // TODO: not technically entirely correct
            float playerEntityCollisionT = testCollision(minT, game->player(), game->entities[i]->position, game->entities[i]->width, game->entities[i]->height);
            float entityPlayerCollisionT = testCollision(engine->frametimeTarget, game->entities[i], game->player()->position, game->player()->width, game->player()->height);
            if ((playerEntityCollisionT < minT || entityPlayerCollisionT < engine->frametimeTarget) && !hasOverlap(game->player()->position, game->player()->width, game->player()->height, game->entities[i]->position, game->entities[i]->width, game->entities[i]->height)) {
                    game->mode = puzzleMode;
            }
            // move entity
            game->entities[i]->position = game->entities[i]->position + game->entities[i]->speed * engine->frametimeTarget * game->entities[i]->velocity.normalized();
        }

        // move player
        float eps = 0.0005;
        game->player()->position = game->player()->position + game->player()->speed * (minT - eps) * game->player()->velocity.normalized();

        // camera will follow player
        game->worldCamera.position = game->player()->position - .5 * v2{game->worldCamera.width, game->worldCamera.height};
}

void drawEntities(Engine *engine, Entity **entities, int numEntities) {
    Camera *camera = engine->activeCamera;
    // draw entities; write in decreasing order of index so that player is written last
    for (int i = numEntities-1; i >= 0; i--) {
        SDL_Rect destRect;
        destRect.x = roundFloatToInt((entities[i]->position.x - camera->position.x - (entities[i]->width / 2.0)) * engine->unitWidth());
        destRect.y = roundFloatToInt((entities[i]->position.y - camera->position.y - (entities[i]->height / 2.0)) * engine->unitHeight()); 
        destRect.w = roundFloatToInt(entities[i]->width * engine->unitWidth());
        destRect.h = roundFloatToInt(entities[i]->height * engine->unitHeight());
        SDL_RenderCopy(engine->renderer, entities[i]->texture, NULL, &destRect);
    }
}

// TODO: fix condition. Capped directions are optional and existence requirement should be bidirectional!
bool canMoveTo(pair dest, pair source, Tilemap<PuzzleTile> *tilemap) {
   // allowed to left
    bool toRet = true;
    PuzzleTile *tile = tilemap->get(source.r, source.c);
    // if tile l is open then lefttile r must exist
    // if lefttile r is open the tile l must exist
    // cases: tile l open and tile r 
    // TODO: make these cases cleaner
    if (tile->existsLeft) {
        if (!tile->cappedLeft) {
            // tile openLeft
            toRet = toRet && (dest.c - 1 > 0 && tilemap->get(dest.r, dest.c - 1)->existsRight);
        }
    } else {
        // neighbor openRight
        toRet = toRet && !(dest.c - 1 > 0 && tilemap->get(dest.r, dest.c - 1)->openRight());
    }

    if (tile->existsUp) {
        if (!tile->cappedUp) {
            toRet = toRet && (dest.c - 1 > 0 && tilemap->get(dest.r - 1, dest.c)->existsDown);
        }
    } else {
        toRet = toRet && !(dest.c - 1 > 0 && tilemap->get(dest.r - 1, dest.c)->openDown());
    }

    if (tile->existsRight) {
        if (!tile->cappedRight) {
            toRet = toRet && (dest.c - 1 > 0 && tilemap->get(dest.r, dest.c + 1)->existsLeft);
        }
    } else {
        toRet = toRet && !(dest.c - 1 > 0 && tilemap->get(dest.r, dest.c + 1)->openLeft());
    }

    if (tile->existsDown) {
        if (!tile->cappedDown) {
            toRet = toRet && (dest.c - 1 > 0 && tilemap->get(dest.r + 1, dest.c)->existsUp);
        }
    } else {
        toRet = toRet && !(dest.c - 1 > 0 && tilemap->get(dest.r + 1, dest.c)->openUp());
    }

    return toRet;

}

bool hasSourceSinkPath(pair source, pair sink, Tilemap<PuzzleTile> *puzzlemap) {

    // row size for 2d indexing
    int rs = puzzlemap->tilesHorz;

    // use iterative DFS with a stack
    pair *stack = new pair[puzzlemap->tilesVert * puzzlemap->tilesHorz];
    // initialize stack
    stack[0] = source;
    int currentIndex = 0;

    // use a 2d array of bools to avoid infinite DFS loop when graph has cycles
    bool *visited = new bool[puzzlemap->tilesVert * puzzlemap->tilesHorz];
    // zero out the array except for the source
    std::memset(visited, 0, puzzlemap->tilesVert * puzzlemap->tilesHorz);
    visited[source.r * rs + source.c] = true;

    bool hitSink = false;

    while (currentIndex >= 0 && !hitSink) {
        pair currentPair;
        PuzzleTile *currentTile;
        currentPair = stack[currentIndex];
        currentTile = puzzlemap->get(currentPair.r, currentPair.c);
        currentIndex--;
        // left
        if (currentTile->openLeft() && currentPair.c - 1 > 0 && !visited[currentPair.r * rs + currentPair.c - 1]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r, currentPair.c - 1);
            if (nextTile->isSpecial || nextTile->openRight()) {
                visited[currentPair.r * rs + currentPair.c - 1] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || sink == nextTile->indices;
            }
            
        }
        // up
        if (currentTile->openUp() && currentPair.r - 1 > 0 && !visited[(currentPair.r - 1) * rs + currentPair.c]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r - 1, currentPair.c);
            if (nextTile->isSpecial || nextTile->openDown()) {
                visited[(currentPair.r - 1) * rs + currentPair.c] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || sink == nextTile->indices;
            }
        }
        // right
        if (currentTile->openRight() && currentPair.c + 1 > 0 && !visited[currentPair.r * rs + currentPair.c + 1]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r, currentPair.c + 1);
            if (nextTile->isSpecial || nextTile->openLeft()) {
                visited[currentPair.r * rs + currentPair.c + 1] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || sink == nextTile->indices;
            }
        }
        // down
        if (currentTile->openDown() && currentPair.r + 1 > 0 && !visited[(currentPair.r + 1) * rs + currentPair.c]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r + 1, currentPair.c);
            if (nextTile->isSpecial || nextTile->openUp()) {
                visited[(currentPair.r + 1) * rs + currentPair.c] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || sink == nextTile->indices;
            }
        }
    }

    // cleanup
    delete visited;
    delete stack;

    return hitSink;
   
}
