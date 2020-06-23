#include "game.h"
#include "..\libraries\SDL2\include\SDL.h"
#include <stdio.h>

// AABBGGRR: NOTE: alpha is "opaqueness" so must be set to 255 to see colors
static const int dark = (0xFF << 24) | (0x00 << 16) | (0x00 << 8) | (0xFF << 0);
static const int light = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0x00 << 0);
static const int cutscene = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);

static bool displayingTilemap = true;

// todo: will need object representation for player eventually (to manage sprites for example)
static const int playerColor = (0xFF << 24) | (0xA0 << 16) | (0x00 << 8) | (0xA0 << 0);

int main(int argc, char *argv[]) {

    struct Engine engine;
    struct GameState game;

    initGameState(&game);
    initEngine(&engine);

    SDL_Surface *cutsceneSurface = DebugMakeRectSurface(engine.screenWidth, engine.screenHeight, cutscene);
    SDL_Texture *cutsceneTexture = SDL_CreateTextureFromSurface(engine.renderer, cutsceneSurface);

    // TODO: should this go into a struct somewhere?
    float tileWidth = (float)engine.screenWidth / game.camera.width;
    float tileHeight = (float)engine.screenHeight / game.camera.height;

    // TEMP: build some other entities for testing
    SDL_Surface *entitySurface = DebugMakeRectSurface(.5 * tileWidth, .5 * tileHeight, cutscene);
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
    SDL_Surface *lightSurface = DebugMakeRectSurface(roundFloatToInt(tileWidth), roundFloatToInt(tileHeight), light);
    SDL_Surface *darkSurface = DebugMakeRectSurface(roundFloatToInt(tileWidth), roundFloatToInt(tileHeight), dark);
    SDL_Texture *lightTexture = SDL_CreateTextureFromSurface(engine.renderer, lightSurface);
    SDL_Texture *darkTexture = SDL_CreateTextureFromSurface(engine.renderer, darkSurface);
    // TODO: make this less messy, right now we need to loop through each tile again to add textures
    for (int i = 0; i < game.world.tilesVert; i++) {
        for (int j = 0; j < game.world.tilesHorz; j++) {
            Tile *tile = &(game.world.tilemap[i][j]);
            if (tile->blocked) {
                tile->texture = darkTexture;
            } else {
                tile->texture = lightTexture;
            }
        }
    }

    // TEMP: add a town for testing
    Town *town = new Town;
    game.world.tilemap[1][1].town = town; 
    town->position = v2{1.5, 1.5};
    town->width = .5;
    town->height = .5;
    town->texture = entityTexture;
    town->id = 3;
    game.entities[2] = town;
    game.numEntities = 3;

    SDL_Surface *playerSurface = DebugMakeRectSurface(game.player()->width * tileWidth, game.player()->height * tileHeight, playerColor);
    SDL_Texture *playerTexture = SDL_CreateTextureFromSurface(engine.renderer, playerSurface);
    game.player()->texture = playerTexture;

    int frameCounter = 0;
    while(true) {
        // main loop
        Uint64 frameStartCounter = SDL_GetPerformanceCounter();

        bool quit = false;

        // handle input
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
                            // Close cutscene window (TEMP)
                            case SDLK_f:
                                displayingTilemap = true;
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
                       break;               }
            }
        }

        if (quit) {
            break;
        }

        // update player position
        // TODO: update by actual frametime not predicted frametime?
        // TODO: improve collision detection
        // strategy: check all 4 walls of all frames in min - max x/y of P0 -> P1 vector
        // solve for time of collision for each wall
        // takes smallest time (min w/ full move as well) and set position to be position 
        // at that time (need epsilon?)
        
        // TODO: make towns distinct objects and game state more extensible
        if (displayingTilemap) {

            // Player -> tilemap collision
            v2 newPlayerPos = game.player()->position + game.player()->speed * engine.frametimeTarget * game.player()->velocity.normalized();
            // NOTE: adding player dimension only to maxes is (maybe) not strictly correct

            float vertExtrude = game.player()->height / 2.0;
            float horzExtrude = game.player()->width / 2.0;

            int minRow = (int)min(game.player()->position.y - vertExtrude, newPlayerPos.y - vertExtrude);
            int maxRow = (int)max(game.player()->position.y + vertExtrude, newPlayerPos.y + vertExtrude);
            int minCol = (int)min(game.player()->position.x - horzExtrude, newPlayerPos.x - horzExtrude);
            int maxCol = (int)max(game.player()->position.x + horzExtrude, newPlayerPos.x + horzExtrude);

            float minT = engine.frametimeTarget;

            for (int i = minRow; i < maxRow + 1; i++) {
                for (int j = minCol; j < maxCol + 1; j++) {
                    if (game.world.tilemap[i][j].blocked) {
                        float collisionT = testCollision(minT, game.player(), v2{(float)j + 0.5f, (float)i + 0.5f}, 1.0f, 1.0f);
                        if (collisionT < minT) {
                            // we collided so we set time to collision time
                            minT = collisionT;
                        }
                    }
                    // check if we hit a town (NOTE: technically this check should go after we determine final minT, but this should be equivalent
                    if (game.world.tilemap[i][j].town != NULL) {
                        Town *town = game.world.tilemap[i][j].town;
                        float collisionT = testCollision(minT, game.player(), town->position, town->width, town->height);
                        if (collisionT < minT && !hasOverlap(game.player()->position, game.player()->width, game.player()->height, town->position, town->width, town->height)) {
                            displayingTilemap = false;
                        }
                    }

                }
            }

            // Player -> entity and entity->player collision
            // brute force check for collision between player and all other entities
            for (int i = 1; i < game.numEntities; i++) {
                // TODO: not technically entirely correct
                float playerEntityCollisionT = testCollision(minT, game.player(), game.entities[i]->position, game.entities[i]->width, game.entities[i]->height);
                float entityPlayerCollisionT = testCollision(engine.frametimeTarget, game.entities[i], game.player()->position, game.player()->width, game.player()->height);
                if ((playerEntityCollisionT < minT || entityPlayerCollisionT < engine.frametimeTarget) && !hasOverlap(game.player()->position, game.player()->width, game.player()->height, game.entities[i]->position, game.entities[i]->width, game.entities[i]->height)) {
                        displayingTilemap = false;
                }
                // move entity
                game.entities[i]->position = game.entities[i]->position + game.entities[i]->speed * engine.frametimeTarget * game.entities[i]->velocity.normalized();
            }

            // move player
            float eps = 0.0005;
            game.player()->position = game.player()->position + game.player()->speed * (minT - eps) * game.player()->velocity.normalized();
 
            // camera will follow player
            game.camera.position = game.player()->position - .5 * v2{game.camera.width, game.camera.height};

            int startingRow = (int)game.camera.position.y;
            int startingCol = (int)game.camera.position.x;
            int endingRow = (int)(game.camera.position.y + game.camera.height);
            int endingCol = (int)(game.camera.position.x + game.camera.width);
            // write to screen
            // TODO: switch i and j
            for (int j = startingRow; j < endingRow+1; j++) {
                for (int i = startingCol; i < endingCol+1; i++) {
                    SDL_Rect destRect;
                    destRect.x = roundFloatToInt(((float)i - game.camera.position.x) * tileWidth);
                    destRect.y = roundFloatToInt(((float)j - game.camera.position.y) * tileHeight);
                    destRect.w = roundFloatToInt(tileWidth);
                    destRect.h = roundFloatToInt(tileHeight);
                    if (game.world.tilemap[j][i].blocked || i > game.world.tilesHorz || i < 0 || j > game.world.tilesVert || j < 0) {
                        SDL_RenderCopy(engine.renderer, darkTexture, NULL, &destRect);
                    } else {
                       SDL_RenderCopy(engine.renderer, lightTexture, NULL, &destRect); 
                    }
                    
                }
            }

            // draw entities; write in decreasing order of index so that player is written last
            for (int i = game.numEntities-1; i >= 0; i--) {
                SDL_Rect destRect;
                destRect.x = roundFloatToInt((game.entities[i]->position.x - game.camera.position.x - (game.entities[i]->width / 2.0)) * tileWidth);
                destRect.y = roundFloatToInt((game.entities[i]->position.y - game.camera.position.y - (game.entities[i]->height / 2.0)) * tileHeight); 
                destRect.w = roundFloatToInt(game.entities[i]->width * tileWidth);
                destRect.h = roundFloatToInt(game.entities[i]->height * tileHeight);
                SDL_RenderCopy(engine.renderer, game.entities[i]->texture, NULL, &destRect);
            }
        } else {
        // display black screen (TEMP)
            SDL_RenderCopy(engine.renderer, cutsceneTexture, NULL, NULL);
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
        // printf("frame %d time: %1.5f\n", frameCounter, (SDL_GetPerformanceCounter() - frameStartCounter) / (float)SDL_GetPerformanceFrequency());
        frameCounter++;
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

    // initialize camera
    game->camera.height = 9.0/2.0;
    game->camera.width = 16.0/2.0;

    // initialize World struct
     int tileTemplate[9][16] = {
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
    game->world.tilesVert = 9;
    game->world.tilesHorz = 16;
    for (int i = 0; i < game->world.tilesVert; i++) {
        for (int j = 0; j < game->world.tilesHorz; j++) {
            Tile tile;
            tile.blocked = tileTemplate[i][j];
            tile.town = NULL;
            game->world.tilemap[i][j] = tile; 
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
