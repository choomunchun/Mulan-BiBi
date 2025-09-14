#pragma once

#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
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

private:
    // Static variables for state
    static GLUquadric* quadric;
    static float windTime;
    static float cloudTime;
    static int currentWeather;
    
    // Storm-related variables
    static float lightningTimer;
    static float lightningBrightness;
    
    // Siege effect variables
    static float siegeTime;
    static std::vector<Arrow> arrows;
    static std::vector<Spark> sparks;
    static std::vector<Ember> embers;

    // Siege effects update and spawn
    static void updateSiegeEffects(float deltaTime);
    static void spawnSiegeEffects();

    // Helper to draw a single tattered banner
    static void drawTatteredBanner(float x, float y, float z, float height, float width, float r, float g, float b);

    // --- Drawing Sub-routines ---

    // Sky & Atmosphere
    static void drawSkyDome();
    static void drawSunOrMoon();
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
    
    // Siege Battle Effects
    static void drawSiegeEffects();
    static void drawCatapultStones();
    static void drawSiegeLadders();
    
    // Wildlife
    static void drawBirds();
    
    // Weather effects
    static void drawRain();
};