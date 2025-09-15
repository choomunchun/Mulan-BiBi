#pragma once

#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Weather conditions
enum WeatherType {
    WEATHER_CLEAR = 0,
    WEATHER_FOG = 1,
    WEATHER_RAIN = 2,
    WEATHER_STORM = 3
};

// Siege effect structures
struct Vec3 {
    float x, y, z;
};

struct Arrow {
    Vec3 position;
    Vec3 velocity;
    float lifetime;
};

struct Spark {
    Vec3 position;
    Vec3 velocity;
    float lifetime;
};

struct Ember {
    Vec3 position;
    Vec3 velocity;
    float lifetime;
};

class BackgroundRenderer {
public:
    // Initialization and cleanup
    static void init();
    static void cleanup();

    // Update animations
    static void update(float deltaTime);

    // Main rendering function
    static void render();

    // Weather control
    static void setWeather(int weather);
    static int getWeather() { return currentWeather; }

    // Audio control
    static void playWarSound();
    static void stopWarSound();
    static void setWarSoundVolume(float volume); // 0.0f to 1.0f
    
    // Audio state access
    static bool warSoundPlaying;
    static float warSoundVolume;
    
    // Collision debug access
    static bool collisionDebugMode;
    
    // Collision detection system
    static bool checkCollision(float x, float z, float radius = 1.0f);
    static bool checkCollisionWithCastles(float x, float z, float radius);
    static bool checkCollisionWithObjects(float x, float z, float radius);
    static void drawCollisionBoxes(); // Debug visualization

private:
    // Static variables for state
    static GLUquadric* quadric;
    static float windTime;
    static float cloudTime;
    static int currentWeather;
    
    // Storm-related variables
    static float lightningTimer;
    static float lightningBrightness;
    
    // Audio-related variables
    static bool warSoundInitialized;
    
    // Collision detection data
    struct CollisionBox {
        float minX, maxX, minZ, maxZ;
        float height;
        const char* name; // For debugging
    };
    static std::vector<CollisionBox> collisionBoxes;
    
    // Siege effect variables
    static float siegeTime;
    static float dayNightTime; // Time for day/night cycle
    static std::vector<Arrow> arrows;
    static std::vector<Spark> sparks;
    static std::vector<Ember> embers;
    
    // Texture system
    static GLuint textures[50]; // Array to hold texture IDs
    static bool texturesLoaded;
    
    // Texture loading and management
    static GLuint loadBMPTexture(const char* filename);
    static void loadAllTextures();
    static void loadTextureOnDemand(int textureIndex);
    static void bindTexture(int textureIndex);
    
    // Texture indices for easy access
    enum TextureIndex {
        TEX_BATTLEFIELD = 0,
        TEX_SKY = 1,
        TEX_SUN = 2,
        TEX_MOON = 3,
        TEX_CLOUDS = 4,
        TEX_MOUNTAIN = 5,
        TEX_FOREST = 6,
        TEX_GRASS = 7,
        TEX_CASTLE = 8,
        TEX_RAIN = 9,
        TEX_STORM = 10,
        TEX_LIGHTNING = 11,
        TEX_FIRE = 12,
        TEX_SMOKE = 13,
        TEX_DEBRIS = 14,
        TEX_BIRD = 15,
        TEX_HORSES = 16,
        TEX_SCORCH = 17,
        TEX_BANNER = 18,
        TEX_MIXED_FOREST = 19,
        TEX_ARROWS = 20,
        TEX_BATTERING_RAMS = 21,
        TEX_CATAPULT_STONES = 22,
        TEX_FALLEN_WARRIOR = 23,
        TEX_KNIGHT_FORMATIONS = 24,
        TEX_DRUMS = 25,
        TEX_CAMPFIRE = 26,
        TEX_TREBUCHET = 27,
        TEX_STARS = 28,
        TEX_TREES = 29,
        TEX_HORSE_CAVALRY = 30,
        TEX_ARCHER_FORMATIONS = 31,
        TEX_WOOD = 32,
        TEX_BLOOD_STAINS = 33,
        TEX_ARMOR = 34,
        TEX_SKIN = 35,
        TEX_HELMET = 36,
        TEX_SABATONS = 37
    };

    // Siege effects update and spawn
    static void updateSiegeEffects(float deltaTime);
    static void spawnSiegeEffects();

    // Helper to draw a single tattered banner
    static void drawTatteredBanner(float x, float y, float z, float height, float width, float r, float g, float b);
    
    // Collision detection helpers
    static void initializeCollisionBoxes();
    static bool pointInBox(float x, float z, const CollisionBox& box);
    static bool circleBoxCollision(float cx, float cz, float radius, const CollisionBox& box);

    // --- Drawing Sub-routines ---

    // Sky & Atmosphere
    static void drawSkyDome();
    static void drawSunOrMoon();
    static void drawStars();
    static void drawClouds();
    static void setupFog();
    static void drawLightning();

    // Terrain
    static void drawMountains();
    static void drawBattlefield();

    // Structures
    static void drawChineseFortress();
    static void drawWesternCastle();

    // Vegetation
    static void drawTrees();
    static void drawGrass();

    // Battle Aftermath
    static void drawWeaponsAndDebris();
    static void drawBanners();
    static void drawFires();
    static void drawSmokePlumes();
    static void drawFallenWarriors();
    static void drawWarHorses();
    
    // Siege Battle Effects
    static void drawSiegeEffects();
    static void drawCatapultStones();
    static void drawSiegeLadders();
    static void drawSiegeTowers();
    static void drawTrebuchets();
    static void drawBatteringRams();
    static void drawArrowVolleys();
    static void drawCampfires();
    static void drawWeaponRacks();
    
    // Chinese War Elements
    static void drawChineseWarDrums();
    static void drawChineseDragonBanners();
    static void drawChineseFireLances();
    
    // Western War Elements
    static void drawWesternKnightFormations();
    static void drawWesternCrossbowSquads();
    static void drawWesternWarHorns();
    
    // Wildlife
    static void drawBirds();
    
    // Weather effects
    static void drawRain();
    
    // Expanded War Scene Elements
    static void drawMassiveArmy();
    static void drawArcherFormations();
    static void drawCavalryCharges();
    static void drawTrebuchetBattery();
    static void drawWarDrums();
    static void drawBatteringRamAssault();
    static void drawArrowVolley();
    static void drawStarField();
    static void drawForestTrees();
};