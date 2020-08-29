#include "game.h"
#include <stdio.h>
#include <cstring>

// TODO LIST OF TASKS TODO
// TODO: - Finish granular level design (fill out small blocks) 
// TODO: - Add progress text (0/4 puzzles completed - squares change color on puzzle completion) [SDLTTF for font rendering]
// TODO: - Fix player animation
// TODO: - Clarify Puzzle goals, add pipe sprites, etc.
// TODO END TASK LIST TODO

// TODO: allow sources to have multiple sinks and detect (and disallow) the mixing of sources
// TODO: allow sources to be special tiles
// TODO: fix memory management!

// add Congratualations text!

enum Direction {LEFT, UP, RIGHT, DOWN};

static bool leftPressed = false;
static bool upPressed = false;
static bool rightPressed = false;
static bool downPressed = false;

static bool walking = false;

// TEMP: global vars for animating
static int frameCounter = 0;

// TODO: add player animation control
int main(int argc, char *argv[]) {

    printf("STARTING\n");
    struct GameState game;
    struct Engine engine;

    initEngine(&engine);
    initGameState(&game, &engine);

    while(true) {
        // main loop
        Uint64 frameStartCounter = SDL_GetPerformanceCounter();

        bool quit = false;

        // TEMP: institute puzzle checking
        bool checkPuzzle = false;

        // handle input
        // TODO: make input handling generic; overhaul input system
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
                            // TEMP: just remember/lookup animation indices
                            case SDLK_a:
                                leftPressed = true;
                                break;
                            case SDLK_w:
                                upPressed = true;
                                break;
                            case SDLK_d:
                                rightPressed = true;
                                break;
                            case SDLK_s:
                                downPressed = true;
                                break;
                            // TEMP: Close puzzle window
                            case SDLK_ESCAPE:
                                game.mode = worldMode;
                                break;
/* DEBUG FUNCTIONALITY
                            // TEMP: check puzzle when p is pressed
                            case SDLK_p:
                                checkPuzzle = true;
                                break;
                            // TEMP: test puzzles
                            case SDLK_1:
                                game.currentPuzzle = 0;
                                break;
                            case SDLK_2:
                                game.currentPuzzle = 1;
                                break;
                            case SDLK_3:
                                game.currentPuzzle = 2;
                                break;
                            case SDLK_4:
                                game.currentPuzzle = 3;
                                break;
*/
                       }
                       break;
 
                    case SDL_KEYUP:
                         switch(event.key.keysym.sym) {
                            case SDLK_w:
                                upPressed = false;
                                break;
                            case SDLK_a:
                                leftPressed = false;
                                break;
                            case SDLK_s:
                                downPressed = false;
                                break;
                            case SDLK_d:
                                rightPressed = false;
                                break;
                        }
                        break;              
 
                  case SDL_MOUSEBUTTONDOWN:
                        switch(event.button.button) {
                            case SDL_BUTTON_LEFT:
                                // select or switch tile
                                if (game.mode == puzzleMode && !game.puzzles[game.currentPuzzle].solved) {
                                    // todo
                                    // TODO (?): camera to world space function 
                                    int col = event.button.x / engine.unitWidth() - engine.activeCamera->position.x;
                                    int row = event.button.y / engine.unitHeight() - engine.activeCamera->position.y;
                                    PuzzleTile *tile = game.puzzles[game.currentPuzzle].tilemap->get(row, col);
                                    if (tile->movable) {
                                        if (!game.selection.hasSelection) {
                                            game.selection.hasSelection = true;
                                            game.selection.coords = pair{row, col};
                                        } else {
                                            pair first = game.selection.coords;
                                            pair second = pair{row, col};
                                            if (canMoveTo(first, second, game.puzzles[game.currentPuzzle].tilemap) 
                                                && canMoveTo(second, first, game.puzzles[game.currentPuzzle].tilemap)) {
                                                game.selection.hasSelection = false;
                                                PuzzleTile temp = *tile;
                                                game.puzzles[game.currentPuzzle].tilemap->set(row, col, game.puzzles[game.currentPuzzle].tilemap->get(game.selection.coords.r, game.selection.coords.c));
                                                game.puzzles[game.currentPuzzle].tilemap->set(game.selection.coords.r, game.selection.coords.c, &temp);
                                                checkPuzzle = true;
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

                // Update animations
                for (int i = 0; i < game.numEntities; i++) {
                    // TEMP: manually advance animation
                    Animation *animation = &game.entities[i]->animator.animations[game.entities[i]->animator.currentAnimation];
                    if (game.entities[i]->animator.isAnimating) {
                        if (animation->currentFrame == animation->framesPerImage) {
                            animation->currentSprite = (animation->currentSprite + 1) % animation->numSprites;
                            animation->currentFrame = 1;
                        }
                        animation->currentFrame++;
                    }
                }
                
                drawEntities(&engine, game.entities, game.numEntities);
                renderSentences(&engine, game.worldSentences, game.numWorldSentences);
                break;

            case puzzleMode:
                engine.activeCamera = &game.puzzleCamera;
                game.puzzles[game.currentPuzzle].tilemap->renderTilemap(&engine);

                // TEMP: highlight selected tile
                if (game.selection.hasSelection && !game.puzzles[game.currentPuzzle].solved) {
                    SDL_Rect destRect;
                    destRect.x = roundFloatToInt(((float)game.selection.coords.c - engine.activeCamera->position.x) * engine.unitWidth());
                    destRect.y = roundFloatToInt(((float)game.selection.coords.r - engine.activeCamera->position.y) * engine.unitHeight());
                    destRect.w = roundFloatToInt(engine.unitWidth());
                    destRect.h = roundFloatToInt(engine.unitHeight());
                    SDL_RenderCopy(engine.renderer, game.highlightTexture, NULL, &destRect);
                }

                if (checkPuzzle) {
                    bool validSolution = true;
                    for (int i = 0; i < game.puzzles[game.currentPuzzle].numGoals; i++) {
                        validSolution = validSolution && hasSourceSinkPath(game.puzzles[game.currentPuzzle].goals[i], game.puzzles[game.currentPuzzle].tilemap);
                    }
                    // TEMP: write to console
                    // TODO: gameplay consequences of solving puzzle
                    if (validSolution) {
                        game.puzzles[game.currentPuzzle].solved = true;
                        game.numPuzzlesSolved++;
                        char worldStr[25];
                        sprintf(worldStr, "Puzzles Solved %d/4", game.numPuzzlesSolved);
                        game.worldSentences[0] = buildSentence(&engine, game.font, fontColor, worldStr, 10, 0, 1.0);
                        SDL_Color finalColor = {255, 255, 255};
                        game.puzzles[game.currentPuzzle].sentence = buildSentence(&engine, game.font, finalColor, "Puzzle Solved!", roundFloatToInt(engine.screenWidth / 2.0), roundFloatToInt(engine.screenHeight / 2.0), 7.0);
                        SDL_Rect *destRect = &game.puzzles[game.currentPuzzle].sentence->destRect;
                        float scale = game.puzzles[game.currentPuzzle].sentence->scale;
                        destRect->x -= roundFloatToInt(destRect->w / 2.0 * scale);
                        destRect->y -= roundFloatToInt(destRect->h / 2.0 * scale);
                        printf("Succesfully hit sink\n");
                        if (game.numPuzzlesSolved == NUMPUZZLES) {
                            game.numWorldSentences++;
                            game.worldSentences[0]->scale *= 2.0;
                            Sentence *sent = buildSentence(&engine, game.font, fontColor, "Congratulations!", roundFloatToInt(engine.screenWidth / 2.0), roundFloatToInt(engine.screenHeight / 2.0), 7.0);
                            game.worldSentences[game.numWorldSentences-1] = sent;
                            sent->destRect.x -= roundFloatToInt(sent->destRect.w / 2.0 * scale);
                            sent->destRect.y -= roundFloatToInt(sent->destRect.h / 2.0 * scale);
                        }
                    } else {
                        printf("Failed to hit sink\n");
                    }
                }
                game.puzzleSentences[0] = game.puzzles[game.currentPuzzle].sentence;
                renderSentences(&engine, game.puzzleSentences, game.numPuzzleSentences);
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

        // TEMP: for animating player
        frameCounter++;
    }

    // cleanup allocated memory
    for (int i = 0; i < game.numEntities; i++) {
        delete game.entities[i];
    }

    // memory cleanup
    // TODO: fix/make more comprehensive
    SDL_DestroyRenderer(engine.renderer);
    SDL_DestroyWindow(engine.window);
    SDL_Quit();

	return 0;
}

void initEngine(Engine *engine) {
    // initialize Engine struct
    engine->screenWidth = SCREENWIDTH;
    engine->screenHeight = SCREENHEIGHT;

    // initialize SDL and make a window
    SDL_Init(SDL_INIT_VIDEO);
    engine->window = SDL_CreateWindow("WorldMap", 0, 0, engine->screenWidth, engine->screenHeight, SDL_WINDOW_FULLSCREEN);
    engine->renderer = SDL_CreateRenderer(engine->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // enable transparency rendering
    SDL_SetRenderDrawBlendMode(engine->renderer, SDL_BLENDMODE_BLEND);

    // find refresh rate to set target framerate
    SDL_DisplayMode display;
    
    if (SDL_GetCurrentDisplayMode(0, &display) == 0) {
        engine->refreshRate = display.refresh_rate;
    } else {
        engine->refreshRate = 60;
    }
    engine->frametimeTarget = 1.0 / (float)engine->refreshRate;
}

void initGameState(GameState *game, Engine *engine) {

    // initialize cameras
    game->worldCamera.height = WORLDCAMERAHEIGHT;
    game->worldCamera.width = WORLDCAMERAWIDTH;
    // world camera position should be set to player position before first render anyways
    // position is top left corner of camera (double check this)
    game->worldCamera.position = v2{};

    game->puzzleCamera.height = PUZZLECAMERAHEIGHT;
    game->puzzleCamera.width = PUZZLECAMERAWIDTH;
    game->puzzleCamera.position = v2{0,0};

    // Make world tilemap
    Tilemap<WorldTile> *worldTilemap = new Tilemap<WorldTile>;

    game->worldTilemap = worldTilemap;
    worldTilemap->tilesVert = WORLDHEIGHT;
    worldTilemap->tilesHorz = WORLDWIDTH;
    worldTilemap->tiles = new WorldTile[worldTilemap->tilesVert * worldTilemap->tilesHorz];

    // Make textures for background and player
    // Textures are reused (1 texture for dark, 1 for light tiles)
    SDL_Surface *lightSurface = DebugMakeRectSurface(TILERES, TILERES, light);
    SDL_Surface *darkSurface = DebugMakeRectSurface(TILERES, TILERES, dark);
    SDL_Texture *lightTexture = SDL_CreateTextureFromSurface(engine->renderer, lightSurface);
    SDL_Texture *darkTexture = SDL_CreateTextureFromSurface(engine->renderer, darkSurface);

    SDL_FreeSurface(lightSurface);
    SDL_FreeSurface(darkSurface);

    // use explicitly defined world template to build worldTilemap
    for (int i = 0; i < worldTilemap->tilesVert; i++) {
        for (int j = 0; j < worldTilemap->tilesHorz; j++) {
            WorldTile *tile = worldTilemap->get(i, j);
            tile->blocked = worldTemplate[i][j];
            tile->town = NULL;
            if (tile->blocked) {
                tile->texture = darkTexture;
            } else {
                tile->texture = lightTexture;
            }
        }
    }

    // Create the player
    Player *player = new Player;

    initAnimator(engine->renderer, &player->animator, NUMPLAYERANIMATIONS, PLAYERANIMATIONSIZES, PLAYERBMPS, PLAYERFRAMESPERSPRITE, PLAYERPIXELSPERUNIT, PLAYERPIXELCENTERSX, PLAYERPIXELCENTERSY);  

    player->position = v2{PLAYERPOSX, PLAYERPOSY};
    player->velocity = v2{0, 0};
    player->speed = PLAYERSPEED;

    player->width = PLAYERWIDTH;
    player->height = PLAYERHEIGHT;

    player->id = 0;
    game->entities[0] = player;
    game->numEntities = 1;

    // TEMP: build textures for towns
    SDL_Surface *townSurface = DebugMakeRectSurface(.5 * TILERES, .5 * TILERES, cutscene);
    SDL_Texture *townTexture = SDL_CreateTextureFromSurface(engine->renderer, townSurface);
    SDL_FreeSurface(townSurface);

    // Create 4 towns
    // TODO: first town has disappeared (sometimes)
    for (int i = 0; i < 4; i++) {
        Town *town = new Town;

        town->animator.currentAnimation = 0;
        town->animator.isAnimating = false;

        Animation *animation = new Animation;
        town->animator.animations = animation;

        animation->sprites = new Sprite;
        animation->numSprites = 1;
        animation->currentSprite = 0;
        animation->framesPerImage = 15;

        animation->sprites[0].texture = townTexture;
        animation->sprites[0].width = TOWNWIDTH;
        animation->sprites[0].height = TOWNHEIGHT;
        animation->sprites[0].center = v2{TOWNWIDTH / 2, TOWNHEIGHT / 2};

        town->width = TOWNWIDTH;
        town->height = TOWNHEIGHT;
        // TEMP:
        town->position = v2{ALLTOWNPOSX[i], ALLTOWNPOSY[i]};
        town->speed = 0.0;
        town->velocity = v2{0, 0};
        town->puzzleNumber = i;
        town->id = i + 1;
        game->entities[town->id] = town;
        game->numEntities++;
    }

    // TODO: free memory
    Puzzle *puzzles = new Puzzle[NUMPUZZLES];
    // TEMP:
    game->puzzles = puzzles;
    game->currentPuzzle = -1;

    // Change when more puzzles are added
    initPuzzle(&(puzzles[0]), puzzleOneTemplate, puzzleOneSourceRows, puzzleOneSourceCols, puzzleOneSinkRows, puzzleOneSinkCols, puzzleOneNumGoals); 
    initPuzzle(&(puzzles[1]), puzzleTwoTemplate, puzzleTwoSourceRows, puzzleTwoSourceCols, puzzleTwoSinkRows, puzzleTwoSinkCols, puzzleTwoNumGoals); 
    initPuzzle(&(puzzles[2]), puzzleThreeTemplate, puzzleThreeSourceRows, puzzleThreeSourceCols, puzzleThreeSinkRows, puzzleThreeSinkCols, puzzleThreeNumGoals); 
    initPuzzle(&(puzzles[3]), puzzleFourTemplate, puzzleFourSourceRows, puzzleFourSourceCols, puzzleFourSinkRows, puzzleFourSinkCols, puzzleFourNumGoals); 

    // TEMP: bind highlight texture
    SDL_Surface *highlightSurface = DebugMakeRectSurface(TILERES, TILERES, highlightColor);
    SDL_Texture *highlightTexture = SDL_CreateTextureFromSurface(engine->renderer, highlightSurface);
    SDL_FreeSurface(highlightSurface);

    game->highlightTexture = highlightTexture;
    // NOTE: default "no tile selected" square is -1, -1
    game->selection.hasSelection = false;

    // TEMP: bind debug textures to puzzles
    DebugBuildTilemapTextures(game->puzzles[0].tilemap, engine);
    DebugBuildTilemapTextures(game->puzzles[1].tilemap, engine);
    DebugBuildTilemapTextures(game->puzzles[2].tilemap, engine);
    DebugBuildTilemapTextures(game->puzzles[3].tilemap, engine);

    // build sentences
    TTF_Init();

    game->numWorldSentences = numWorldSentences;
    game->numPuzzleSentences = numPuzzleSentences;

    char fp[MAXFILEPATHLENGTH];
    strcpy(fp, TLPATH);
    strcat(fp, fontPath);
 
    TTF_Font *font = TTF_OpenFont(fp, fontSize);
    game->font = font;

    for (int i = 0; i < numWorldSentences; i++) {
        game->worldSentences[i] = buildSentence(engine, font, fontColor, worldSentences[i], worldSentencesX[i], worldSentencesY[i], worldSentencesScale[i]);
    }

    for (int i = 1; i < numPuzzleSentences; i++) {
        game->puzzleSentences[i] = buildSentence(engine, font, fontColor, puzzleSentences[i - 1], puzzleSentencesX[i - 1], puzzleSentencesY[i - 1], puzzleSentencesScale[i - 1]);
    }

    for (int i = 0; i < NUMPUZZLES; i++) {
        // TODO: fix constants
        game->puzzles[i].sentence = buildSentence(engine, font, fontColor, puzzleUnsolvedText, puzzleUnsolvedXStart, puzzleUnsolvedYStart, 1.0);
    }

    // TEMP: start game in world
    game->mode = worldMode;
}

void initAnimator(SDL_Renderer *renderer, Animator *animator, int numAnimations, const int *animationSizes, const char **bmps, const int framesPerSprite, float pixelsPerUnit, const int *pixelCentersX, const int *pixelCentersY) {
    animator->animations = new Animation[numAnimations];
    // tracks position in 1d bmps array representing 2d info
    int spriteOffset = 0;
    for (int i = 0; i < numAnimations; i++) {
        Animation *animation = &animator->animations[i];
        animation->numSprites = animationSizes[i];
        animation->framesPerImage = framesPerSprite;
        animation->sprites = new Sprite[animation->numSprites];
        for (int j = 0; j < animation->numSprites; j++) {
            int sprIndex = spriteOffset + j;
            // Load Texture
            char fp[MAXFILEPATHLENGTH];
            strcpy(fp, TLPATH);
            strcat(fp, bmps[sprIndex]);
            printf("%s\n", fp);
            SDL_Surface *surface = SDL_LoadBMP(fp);
            if (surface == NULL) {
                printf(SDL_GetError());
            }
            animation->sprites[j].texture = SDL_CreateTextureFromSurface(renderer, surface);
            animation->sprites[j].width = surface->w / pixelsPerUnit;
            animation->sprites[j].height = surface->h / pixelsPerUnit;
            animation->sprites[j].center = v2{pixelCentersX[sprIndex] / pixelsPerUnit, pixelCentersY[sprIndex] / pixelsPerUnit};

            SDL_FreeSurface(surface);
        }
        spriteOffset += animationSizes[i];
    }

}
 

void initPuzzle(Puzzle *puzzle, const int puzzleTemplate[PUZZLEHEIGHT][PUZZLEWIDTH], const int *puzzleSourceRows, const int *puzzleSourceCols, const int *puzzleSinkRows, const int *puzzleSinkCols, const int numGoals) {
    puzzle->tilemap = new Tilemap<PuzzleTile>;
    puzzle->tilemap->tilesVert = PUZZLEHEIGHT;
    puzzle->tilemap->tilesHorz = PUZZLEWIDTH;
    puzzle->tilemap->tiles = new PuzzleTile[puzzle->tilemap->tilesHorz * puzzle->tilemap->tilesVert];
    for (int i = 0; i < puzzle->tilemap->tilesVert; i++) {
        for (int j = 0; j < puzzle->tilemap->tilesHorz; j++) {
            // Build the tile from its code by explicitly calling its constructor
            PuzzleTile *tile = puzzle->tilemap->get(i, j);
            tile->indices = pair{i,j};
            tile->parseCode(puzzleTemplate[i][j]);
        }
    }

    puzzle->numGoals = numGoals;
    puzzle->goals = new Goal[numGoals];
    for (int i = 0; i < numGoals; i++) {
        puzzle->goals[i].source = pair{puzzleSourceRows[i], puzzleSourceCols[i]};
        puzzle->goals[i].sink = pair{puzzleSinkRows[i], puzzleSinkCols[i]};
    }
};

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

void DebugBuildTilemapTextures(Tilemap<PuzzleTile> *puzzleTilemap, Engine *engine) {
    for (int i = 0; i < puzzleTilemap->tilesVert; i++) {
        for (int j = 0; j < puzzleTilemap->tilesHorz; j++) {
            int reactorColor = (0xFF << 24) | (0x00 << 16) | (0x00 << 8) | (0xFF << 0);
            int coolerColor = (0xFF << 24) | (0xFF << 16) | (0x00 << 8) | (0x00 << 0);
            int backgroundColor = (0xFF << 24) | (0x55 << 16) | (0x10 << 8) | (0x30 << 0);
            int pipeColor = (0xFF << 24) | (0x00 << 16) | (0xFF << 8) | (0x00 << 0);
            int cappedColor = (0xFF << 24) | (0x10 << 16) | (0x50 << 8) | (0x10 << 0);
            int borderSize = 3;

            PuzzleTile *tile = puzzleTilemap->get(i,j);
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
                surface = DebugMakeRectSurface(TILERES, TILERES, specialColor);
            } else {
                // write background leaving black border
                surface = DebugMakeRectSurface(TILERES, TILERES, backgroundColor);
                SDL_Surface *pipeSource = DebugMakeRectSurface(TILERES / 3.0, TILERES / 3.0, pipeColor);
                SDL_Surface *cappedSource = DebugMakeRectSurface(TILERES / 3.0, TILERES / 3.0, cappedColor);
                int pieceWidth = TILERES / 3.0;
                int pieceHeight = TILERES / 3.0;

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
            SDL_Surface *vertBorder = DebugMakeRectSurface(borderSize, TILERES, borderColor);
            SDL_Surface *horzBorder = DebugMakeRectSurface(TILERES, borderSize, borderColor);
            // draw vertical borders
            SDL_Rect rect;
            rect.x = 0;
            rect.y = 0;
            rect.w = borderSize;
            rect.h = TILERES;
            SDL_BlitSurface(vertBorder, NULL, surface, &rect);
            rect.x = TILERES - borderSize;
            SDL_BlitSurface(vertBorder, NULL, surface, &rect);
            // draw horizontal borders
            rect. x = 0;
            rect.w = TILERES;
            rect.h = borderSize;
            SDL_BlitSurface(horzBorder, NULL, surface, &rect);
            rect.y = TILERES - borderSize;
            SDL_BlitSurface(horzBorder, NULL, surface, &rect);

            tile->texture = SDL_CreateTextureFromSurface(engine->renderer, surface);
        }
    }

}

void worldModeUpdate(GameState *game, Engine *engine) {

    int hasOver = hasOverlap(game->player()->position, game->player()->width, game->player()->height, game->entities[1]->position, game->entities[1]->width, game->entities[1]->height);
    //printf("hasOverlap: %d\n", hasOver);

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
                float collisionT = testCollision(minT, game->player(), v2{j + 0.5f, i + 0.5f}, 1.0f, 1.0f);
                if (collisionT < minT) {
                    // we collided so we set time to collision time
                    minT = collisionT;
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
        if (game->entities[i]->puzzleNumber > -1
            && (playerEntityCollisionT < minT || entityPlayerCollisionT < engine->frametimeTarget)
            && !hasOverlap(game->player()->position, game->player()->width, game->player()->height, game->entities[i]->position, game->entities[i]->width, game->entities[i]->height)) {
                game->mode = puzzleMode;
                game->currentPuzzle = game->entities[i]->puzzleNumber;
        }
        // move entity
        game->entities[i]->position = game->entities[i]->position + game->entities[i]->speed * engine->frametimeTarget * game->entities[i]->velocity.normalized();
    }

    // move player
    float eps = 0.0005;
    game->player()->position = game->player()->position + game->player()->speed * (minT - eps) * game->player()->velocity.normalized();

    // camera will follow player
    game->worldCamera.position = game->player()->position - .5 * v2{game->worldCamera.width, game->worldCamera.height};

    // set animations
    if (!(leftPressed || upPressed || rightPressed || downPressed)) {
        walking = false;
        game->player()->velocity = v2{0,0};
        game->player()->animator.stopAnimating();
    } else {
        walking = true;
        game->player()->animator.isAnimating = true;

        // left > right
        if (leftPressed) {
            game->player()->velocity.x = -1;
            game->player()->animator.currentAnimation = LEFT;
        } else {
            if (rightPressed) {
                game->player()->velocity.x = 1;
                game->player()->animator.currentAnimation = RIGHT;
            } else {
                game->player()->velocity.x = 0;
            }
        }

        // up > down
        if (upPressed) {
            game->player()->velocity.y = -1;
            game->player()->animator.currentAnimation = UP;
        } else {
            if (downPressed) {
                game->player()->velocity.y = 1;
                game->player()->animator.currentAnimation = DOWN;
            } else {
                game->player()->velocity.y = 0;
            }
        }
    }
}

void drawEntities(Engine *engine, Entity **entities, int numEntities) {
    Camera *camera = engine->activeCamera;
    // draw entities; write in decreasing order of index so that player is written last
    for (int i = numEntities-1; i >= 0; i--) {
        // Draw to screen
        SDL_Rect destRect;
        Sprite *sprite = entities[i]->animator.currentSprite();
        destRect.x = roundFloatToInt((entities[i]->position.x - camera->position.x - sprite->center.x) * engine->unitWidth());
        destRect.y = roundFloatToInt((entities[i]->position.y - camera->position.y - sprite->center.y) * engine->unitHeight()); 
        destRect.w = roundFloatToInt(sprite->width * engine->unitWidth());
        destRect.h = roundFloatToInt(sprite->height * engine->unitHeight());
        SDL_RenderCopy(engine->renderer, sprite->texture, NULL, &destRect);
    }
}

Sentence *buildSentence(Engine *engine, TTF_Font *font, SDL_Color color, const char *text, int x, int y, float scale) {
    SDL_Surface *sentenceSurface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *sentenceTexture = SDL_CreateTextureFromSurface(engine->renderer, sentenceSurface);

    SDL_Rect destRect;
    destRect.h = sentenceSurface->h;
    destRect.w = sentenceSurface->w;
    destRect.x = x;
    destRect.y = y;
    Sentence *sentence = new Sentence;
    sentence->texture = sentenceTexture;
    sentence->destRect = destRect;
    sentence->scale = scale;

    SDL_FreeSurface(sentenceSurface);

    return sentence;
}

// TODO: decide interface for this
void renderSentences(Engine *engine, Sentence **sentences, int numSentences) {

    for (int i = 0; i < numSentences; i++) {
        SDL_Rect scaledDestRect = sentences[i]->destRect;
        scaledDestRect.w = roundFloatToInt(scaledDestRect.w * sentences[i]->scale);
        scaledDestRect.h = roundFloatToInt(scaledDestRect.h * sentences[i]->scale);

        SDL_RenderCopy(engine->renderer, sentences[i]->texture, NULL, &scaledDestRect);
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
            toRet = toRet && (dest.c - 1 >= 0 && tilemap->get(dest.r, dest.c - 1)->existsRight);
        }
    } else {
        // neighbor openRight
        toRet = toRet && !(dest.c - 1 >= 0 && tilemap->get(dest.r, dest.c - 1)->openRight());
    }

    if (tile->existsUp) {
        if (!tile->cappedUp) {
            toRet = toRet && (dest.r - 1 >= 0 && tilemap->get(dest.r - 1, dest.c)->existsDown);
        }
    } else {
        toRet = toRet && !(dest.r - 1 >= 0 && tilemap->get(dest.r - 1, dest.c)->openDown());
    }

    if (tile->existsRight) {
        if (!tile->cappedRight) {
            toRet = toRet && (dest.c + 1 >= 0 && tilemap->get(dest.r, dest.c + 1)->existsLeft);
        }
    } else {
        toRet = toRet && !(dest.c + 1 >= 0 && tilemap->get(dest.r, dest.c + 1)->openLeft());
    }

    if (tile->existsDown) {
        if (!tile->cappedDown) {
            toRet = toRet && (dest.r + 1 > 0 && tilemap->get(dest.r + 1, dest.c)->existsUp);
        }
    } else {
        toRet = toRet && !(dest.r + 1 > 0 && tilemap->get(dest.r + 1, dest.c)->openUp());
    }

    return toRet;

}

bool hasSourceSinkPath(Goal goal, Tilemap<PuzzleTile> *puzzlemap) {

    // row size for 2d indexing
    int rs = puzzlemap->tilesHorz;

    // use iterative DFS with a stack
    pair *stack = new pair[puzzlemap->tilesVert * puzzlemap->tilesHorz];
    // initialize stack
    stack[0] = goal.source;
    int currentIndex = 0;

    // use a 2d array of bools to avoid infinite DFS loop when graph has cycles
    bool *visited = new bool[puzzlemap->tilesVert * puzzlemap->tilesHorz];
    // zero out the array except for the source
    std::memset(visited, 0, puzzlemap->tilesVert * puzzlemap->tilesHorz);
    visited[goal.source.r * rs + goal.source.c] = true;

    bool hitSink = false;

    while (currentIndex >= 0 && !hitSink) {
        pair currentPair;
        PuzzleTile *currentTile;
        currentPair = stack[currentIndex];
        currentTile = puzzlemap->get(currentPair.r, currentPair.c);
        currentIndex--;
        // left
        if (currentTile->openLeft() && currentPair.c - 1 >= 0 && !visited[currentPair.r * rs + currentPair.c - 1]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r, currentPair.c - 1);
            if (nextTile->isSpecial || nextTile->openRight()) {
                visited[currentPair.r * rs + currentPair.c - 1] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || goal.sink == nextTile->indices;
            }
            
        }
        // up
        if (currentTile->openUp() && currentPair.r - 1 >= 0 && !visited[(currentPair.r - 1) * rs + currentPair.c]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r - 1, currentPair.c);
            if (nextTile->isSpecial || nextTile->openDown()) {
                visited[(currentPair.r - 1) * rs + currentPair.c] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || goal.sink == nextTile->indices;
            }
        }
        // right
        if (currentTile->openRight() && currentPair.c + 1 >= 0 && !visited[currentPair.r * rs + currentPair.c + 1]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r, currentPair.c + 1);
            if (nextTile->isSpecial || nextTile->openLeft()) {
                visited[currentPair.r * rs + currentPair.c + 1] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || goal.sink == nextTile->indices;
            }
        }
        // down
        if (currentTile->openDown() && currentPair.r + 1 >= 0 && !visited[(currentPair.r + 1) * rs + currentPair.c]) {
            PuzzleTile *nextTile = puzzlemap->get(currentPair.r + 1, currentPair.c);
            if (nextTile->isSpecial || nextTile->openUp()) {
                visited[(currentPair.r + 1) * rs + currentPair.c] = true;
                currentIndex++;
                stack[currentIndex] = nextTile->indices;
                hitSink = hitSink || goal.sink == nextTile->indices;
            }
        }
    }

    // cleanup
    delete [] visited;
    delete [] stack;

    return hitSink;
   
}
