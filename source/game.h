#include "..\libraries\SDL2\include\SDL.h"
#include <math.h>
#include <cstring>

// (Possible) TODO: replace common width/height format w/ a rect struct (containing width, height, v2 center)

SDL_Surface *DebugMakeRectSurface(int w, int h, int color);

struct v2 {
    float x, y;

    inline float magnitude(void);
    inline v2 normalized(void);
};

struct pair {
    int r, c;

};

inline bool operator==(pair x, pair y) {
    return x.r == y.r && x.c == y.c;
}

struct Entity {
    int id;
    v2 position;
    v2 velocity;
    float speed;
    float width;
    float height;
    SDL_Texture *texture;
    // more physics?
    // animator
    // TODO
};

struct Town : public Entity {
    // TODO
};

struct Player : public Entity {
    // TODO
};

struct Camera {
    // all in world units
    v2 position;
    float height;
    float width;
};

struct Engine {
    int screenWidth;
    int screenHeight;
    SDL_Window *window;
    int refreshRate;
    float frametimeTarget;
    SDL_Renderer *renderer;
    Camera *activeCamera;

    inline float unitWidth();
    inline float unitHeight();
};

inline float Engine::unitWidth(void) {
    return screenWidth / activeCamera->width;
}

inline float Engine::unitHeight(void) {
    return screenHeight / activeCamera->height;
}

struct Tile {
    SDL_Texture *texture;
};

template <class T>
struct Tilemap {
    // TODO: what is the most efficient/common sense representation here (flywheel?)
    // 1d array of Tiles with 2d array indexing
    T *tiles;
    int tilesVert;
    int tilesHorz;

    inline T *Tilemap::get(int row, int col) {
        return &tiles[row * tilesHorz + col];
    }

    inline void Tilemap::set(int row, int col, T *newTile) {
        tiles[row * tilesHorz + col] = *newTile;
    }

    void Tilemap::renderTilemap(Engine *engine);
};

template <class T>
void Tilemap<T>::renderTilemap(Engine *engine) {
    Camera *camera = engine->activeCamera;
    int startingRow = (int)(camera->position.y);
    int startingCol = (int)(camera->position.x);
    // add 1 so that the cast rounds up
    int endingRow = (int)(camera->position.y + camera->height + 1);
    int endingCol = (int)(camera->position.x + camera->width + 1);
    // write to screen
    for (int i = startingRow; i < endingRow; i++) {
        for (int j = startingCol; j < endingCol; j++) {
            SDL_Rect destRect;
            destRect.x = roundFloatToInt(((float)j - camera->position.x) * engine->unitWidth());
            destRect.y = roundFloatToInt(((float)i - camera->position.y) * engine->unitHeight());
            destRect.w = roundFloatToInt(engine->unitWidth());
            destRect.h = roundFloatToInt(engine->unitHeight());
            if (j < tilesHorz && j >= 0 && i < tilesVert && i >= 0) {
                SDL_RenderCopy(engine->renderer, get(i, j)->texture, NULL, &destRect);
            } else {
                // TODO: display blackness for out of bounds
                SDL_RenderCopy(engine->renderer, get(0, 0)->texture, NULL, &destRect);
            }
        }
    }
}

struct WorldTile : public Tile { 
    bool blocked;
    Town *town; // NULL if there is no town on the tile
};

struct PuzzleTile : public Tile {
    int code;
    pair indices;

    bool movable;

    bool isSpecial;
    bool isCooler;
    bool isReactor;

    bool existsLeft;
    bool existsUp;
    bool existsRight;
    bool existsDown;
    bool cappedLeft;
    bool cappedUp;
    bool cappedRight;
    bool cappedDown;

    
    PuzzleTile(void) {    }
    PuzzleTile(pair tileIndices, int tileCode) {
        indices = tileIndices;
        parseCode(tileCode);
    }

    // TODO: design of this could be improved. Potentially provide a pure function for this use.
    void parseCode(int tileCode);

    bool openLeft(void) {
        return existsLeft && !cappedLeft;
    }

    bool openUp(void) {
        return existsUp && !cappedUp;
    }

    bool openRight(void) {
        return existsRight && !cappedRight;
    }

    bool openDown(void) {
        return existsDown && !cappedDown;
    }
};

void PuzzleTile::parseCode(int tileCode) {
    code = tileCode;
    // tenth bit
    movable = (1 << 9) & code;

    isSpecial = 1 & code;
    if (isSpecial) {
        isCooler = (1 << 1) & code;
        isReactor = !isCooler;
        existsLeft = false;
        cappedLeft = false;
        existsUp = false;
        cappedUp = false;
        existsRight = false;
        cappedRight = false;
        existsDown = false;
        cappedDown = false;
    } else {
        existsLeft = (1 << 1) & code;
        existsUp = (1 << 2) & code;
        existsRight = (1 << 3) & code;
        existsDown = (1 << 4) & code;
        cappedLeft = (1 << 5) & code;
        cappedUp = (1 << 6) & code;
        cappedRight = (1 << 7) & code;
        cappedDown = (1 << 8) & code;
        isCooler = false;
        isReactor = false;
    }
}

enum GameMode { worldMode, puzzleMode };

struct TileSelection {
    bool hasSelection;
    pair coords;
};

// TODO
struct GameState {
    GameMode mode;
    Camera worldCamera;
    Camera puzzleCamera;
    Tilemap<WorldTile> *worldTilemap;
    Tilemap<PuzzleTile> *puzzleTilemap;
    TileSelection selection;
    // an array of entity pointers
    Entity *entities[1000]; // TODO: estimate max number of entities
    int numEntities;
    inline Player *player();
};

void initEngine(Engine *engine);
void initGameState(GameState *game);
float testCollision(float dT, Entity *colliding, v2 testPosition, float testWidth, float testHeight);
void renderTilemap(Engine *engine, Tilemap<Tile> *tilemap);
void worldModeUpdate(GameState *game, Engine *engine);
void drawEntities(Engine *engine, Entity **entities, int numEntities);
bool canMoveTo(pair dest, pair source, Tilemap<PuzzleTile> *tilemap);

// NOTE: this provides an alias to reference player (entity 0)
inline Player *GameState::player(void) {
    return (Player *)entities[0];
}

// TODO: decide if this actually makes things simpler?
struct Input {
    bool quit;
    bool downW;
    bool upW;
    bool downA;
    bool upA; 
    bool downS;
    bool upS;
    bool downD;
    bool upD;    
};

// vector operations
inline v2 operator-(v2 a) {
    return v2{-a.x, -a.y};
}

inline v2 operator+(v2 a, v2 b) {
    return v2{a.x + b.x, a.y + b.y};
}

inline v2 operator-(v2 a, v2 b) {
    return v2{a.x - b.x, a.y - b.y};
}

inline v2 operator*(float a, v2 b) {
    return v2{a * b.x, a * b.y};
}

inline v2 operator+=(v2 a, v2 b) {
    return v2{a.x + b.x, a.y + b.y};
}

inline v2 operator-=(v2 a, v2 b) {
    return v2{a.x - b.x, a.y - b.y};
}

inline float dot(v2 a, v2 b) {
    return a.x * b.x + a.y * b.y;
}

inline float v2::magnitude(void) {
   return sqrt(dot(*this, *this));
}

inline v2 v2::normalized(void) {
    float mag = magnitude();
    if (mag == 0.0) {
        return v2{0, 0}; 
    } else {
        return (1.0 / mag) * v2{x, y};
    }
}

// math operations
inline float roundFloatToInt(float x) {
    return (int) (x + .5);
}

inline float min(float x, float y) {
    return (x < y) ? x : y;
}

inline float max(float x, float y) {
    return (x > y) ? x : y;
}

inline bool pointInRect(v2 point, v2 pos, float width, float height) {
    return (point.x > pos.x - width/2
            && point.x < pos.x + width/2 
            && point.y > pos.y - width/2 
            && point.y < pos.y + width/2);
}

// misc operations
inline bool hasOverlap(v2 pos1, float width1, float height1, v2 pos2, float width2, float height2) {
    return (pointInRect(pos1 + .5 * v2{width1, height1}, pos2, width2, height2)
            || pointInRect(pos1 + .5 * v2{width1, -height1}, pos2, width2, height2)
            || pointInRect(pos1 + .5 * v2{-width1, height1}, pos2, width2, height2)
            || pointInRect(pos1 + .5 * v2{-width1, -height1}, pos2, width2, height2)
    );
}
