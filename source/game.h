#include "..\libraries\SDL2\include\SDL.h"
#include <math.h>

// (Possible) TODO: replace common width/height format w/ a rect struct (containing width, height, v2 center)

SDL_Surface *DebugMakeRectSurface(int w, int h, int color);

struct v2 {
    float x, y;

    inline float magnitude(void);
    inline v2 normalized(void);
};

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



struct Tile {
    bool blocked;
    SDL_Texture *texture;
    Town *town; // NULL if there is no town on the tile
};

struct World {
    int tilesVert;
    int tilesHorz;
    Tile tilemap[9][16]; // 2D array
};

struct Engine {
    int screenWidth;
    int screenHeight;
    SDL_Window *window;
    int refreshRate;
    float frametimeTarget;
    SDL_Renderer *renderer;
};

struct Camera {
    // all in world units
    v2 position;
    float height;
    float width;
};

// TODO
struct GameState {
    // world, camera position, entities, background (?), etc.
    Camera camera;
    World world;
    // an array of entity pointers
    Entity *entities[1000]; // TODO: estimate max number of entities
    int numEntities;
    inline Player *player();
};

void initEngine(Engine *engine);
void initGameState(GameState *game);
float testCollision(float dT, Entity *colliding, v2 testPosition, float testWidth, float testHeight);

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
