#include "background.h"
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Force ANSI version for this specific function call
#undef LoadImage
#define LoadImage LoadImageA

// Initialize static variables
GLUquadric* BackgroundRenderer::quadric = nullptr;
float BackgroundRenderer::windTime = 0.0f;
float BackgroundRenderer::cloudTime = 0.0f;
int BackgroundRenderer::currentWeather = WEATHER_CLEAR;
float BackgroundRenderer::lightningTimer = 0.0f;
float BackgroundRenderer::lightningBrightness = 0.0f;
float BackgroundRenderer::siegeTime = 0.0f;
float BackgroundRenderer::dayNightTime = 0.0f;
std::vector<Spark> BackgroundRenderer::sparks;
std::vector<Arrow> BackgroundRenderer::arrows;
std::vector<Ember> BackgroundRenderer::embers;

// Audio system variables
bool BackgroundRenderer::warSoundPlaying = false;
bool BackgroundRenderer::warSoundInitialized = false;
float BackgroundRenderer::warSoundVolume = 0.7f;

// Collision detection variables
std::vector<BackgroundRenderer::CollisionBox> BackgroundRenderer::collisionBoxes;
bool BackgroundRenderer::collisionDebugMode = false;

// Texture system variables
GLuint BackgroundRenderer::textures[50];
bool BackgroundRenderer::texturesLoaded = false;


void BackgroundRenderer::init() {
    printf("Initializing BackgroundRenderer...\n");
    
    quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluQuadricTexture(quadric, GL_TRUE);

    // Setup basic lighting for a more 3D feel
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Set light properties - low sun for dramatic shadows
    GLfloat light_pos[] = { 50.0f, 30.0f, -100.0f, 0.0f }; // Directional light from far away
    GLfloat ambient[] = { 0.3f, 0.3f, 0.35f, 1.0f };
    GLfloat diffuse[] = { 0.8f, 0.8f, 0.7f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    // Seed random generator for effects like fire and grass
    srand(time(NULL));
    
    // Load all textures
    printf("Loading all textures...\n");
    loadAllTextures();
    
    // Initialize audio system
    printf("Initializing audio system...\n");
    warSoundInitialized = true;
    warSoundPlaying = false;
    
    // Initialize collision detection system
    printf("Initializing collision detection...\n");
    initializeCollisionBoxes();
    
    printf("BackgroundRenderer initialization complete.\n");
}

void BackgroundRenderer::cleanup() {
    if (quadric) {
        gluDeleteQuadric(quadric);
        quadric = nullptr;
    }
    
    // Clean up textures
    if (texturesLoaded) {
        glDeleteTextures(50, textures);
        texturesLoaded = false;
    }
    
    // Stop any playing audio
    if (warSoundPlaying) {
        stopWarSound();
    }
    
    warSoundInitialized = false;
}

void BackgroundRenderer::update(float deltaTime) {
    windTime += deltaTime * 0.8f;
    cloudTime += deltaTime * 0.15f;
    siegeTime += deltaTime;
    dayNightTime += deltaTime; // Update day/night cycle

    // Update lightning for storms
    if (currentWeather == WEATHER_STORM) {
        lightningTimer -= deltaTime;
        if (lightningTimer <= 0.0f) {
            // More intense and varied lightning strikes
            lightningBrightness = 0.7f + (rand() % 30) / 100.0f; // Random brightness 0.7-1.0
            lightningTimer = 1.0f + (rand() % 40) / 10.0f;       // Next strike in 1-5 seconds (more frequent)
        }
        // Faster fade out for more dramatic flicker
        lightningBrightness -= deltaTime * 6.0f;
        if (lightningBrightness < 0.0f) lightningBrightness = 0.0f;
    } else {
        lightningBrightness = 0.0f;
    }

    // Update siege effects
    updateSiegeEffects(deltaTime);
}

// ===== SIEGE EFFECTS UPDATE =====
void BackgroundRenderer::updateSiegeEffects(float deltaTime) {
    // Update flying arrows
    for (auto it = arrows.begin(); it != arrows.end();) {
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;
        it->velocity.y -= 9.8f * deltaTime; // Gravity
        it->lifetime -= deltaTime;
        
        if (it->lifetime <= 0.0f || it->position.y < 0.0f) {
            it = arrows.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update sparks from metalwork
    for (auto it = sparks.begin(); it != sparks.end();) {
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;
        it->velocity.y -= 5.0f * deltaTime; // Light gravity
        it->lifetime -= deltaTime;
        
        if (it->lifetime <= 0.0f) {
            it = sparks.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update fire embers
    for (auto it = embers.begin(); it != embers.end();) {
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;
        it->velocity.y += 2.0f * deltaTime; // Rising with heat
        it->lifetime -= deltaTime;
        
        if (it->lifetime <= 0.0f) {
            it = embers.erase(it);
        } else {
            ++it;
        }
    }
    
    // Spawn new effects periodically
    if (fmod(siegeTime, 0.5f) < deltaTime) {
        spawnSiegeEffects();
    }
}

void BackgroundRenderer::spawnSiegeEffects() {
    // Spawn arrows during siege
    if (arrows.size() < 20) {
        Arrow arrow;
        arrow.position = {-80.0f + (rand() % 160), 25.0f + (rand() % 15), -60.0f + (rand() % 20)};
        arrow.velocity = {15.0f + (rand() % 10), -5.0f + (rand() % 5), 10.0f + (rand() % 5)};
        arrow.lifetime = 3.0f + (rand() % 200) / 100.0f;
        arrows.push_back(arrow);
    }
    
    // Spawn sparks from metalwork
    if (sparks.size() < 50) {
        for (int i = 0; i < 5; i++) {
            Spark spark;
            spark.position = {-40.0f + (rand() % 80), 8.0f + (rand() % 5), -35.0f + (rand() % 10)};
            spark.velocity = {(rand() % 10 - 5) * 0.5f, (float)(rand() % 8), (rand() % 10 - 5) * 0.5f};
            spark.lifetime = 0.5f + (rand() % 100) / 200.0f;
            sparks.push_back(spark);
        }
    }
    
    // Spawn fire embers
    if (embers.size() < 30) {
        for (int i = 0; i < 3; i++) {
            Ember ember;
            ember.position = {15.0f + (rand() % 10), 2.0f, -8.0f + (rand() % 4)};
            ember.velocity = {(rand() % 6 - 3) * 0.3f, 2.0f + (rand() % 3), (rand() % 6 - 3) * 0.3f};
            ember.lifetime = 2.0f + (rand() % 150) / 100.0f;
            embers.push_back(ember);
        }
    }
}

// ===== TEXTURE SYSTEM =====
GLuint BackgroundRenderer::loadBMPTexture(const char* filename) {
    printf("Loading texture: %s\n", filename);
    
    GLuint textureID = 0;
    HBITMAP hBMP = NULL;
    BITMAP BMP;

    // Load bitmap using the same method as the working example
    hBMP = (HBITMAP)LoadImage(GetModuleHandle(NULL), filename, IMAGE_BITMAP, 0, 0, 
                              LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    // If loading failed, show detailed error information
    if (!hBMP) {
        DWORD error = GetLastError();
        printf("ERROR: Could not load texture file '%s'. Error code: %lu\n", filename, error);
        
        // Create a simple colored texture as fallback instead of returning 0
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Create a 2x2 white texture as fallback
        unsigned char whitePixels[12] = {255,255,255, 255,255,255, 255,255,255, 255,255,255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        return textureID;
    }

    printf("Successfully loaded texture: %s\n", filename);

    // Get bitmap information
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    GetObject(hBMP, sizeof(BMP), &BMP);

    // Generate and bind texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters for better performance
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Load texture data directly from bitmap bits (more efficient)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BMP.bmWidth, BMP.bmHeight, 0, 
                 GL_BGR_EXT, GL_UNSIGNED_BYTE, BMP.bmBits);

    // Clean up GDI object immediately to reduce memory usage
    DeleteObject(hBMP);
    
    return textureID;
}

void BackgroundRenderer::loadAllTextures() {
    if (texturesLoaded) return;
    
    printf("Loading essential textures...\n");
    
    // Only load the most essential textures at startup to reduce memory usage
    // Other textures will be loaded on-demand when needed
    textures[TEX_BATTLEFIELD] = loadBMPTexture("texturess/Battlefield Terrain.bmp");
    textures[TEX_SKY] = loadBMPTexture("texturess/sky.bmp");
    textures[TEX_SUN] = loadBMPTexture("texturess/sun.bmp");
    textures[TEX_MOON] = loadBMPTexture("texturess/moon.bmp");
    textures[TEX_MOUNTAIN] = loadBMPTexture("texturess/mountain.bmp");
    textures[TEX_CASTLE] = loadBMPTexture("texturess/castle.bmp");
    
    // Initialize other texture slots to 0 (will be loaded on-demand)
    textures[TEX_CLOUDS] = 0;
    textures[TEX_FOREST] = 0;
    textures[TEX_GRASS] = 0;
    textures[TEX_RAIN] = 0;
    textures[TEX_STORM] = 0;
    textures[TEX_LIGHTNING] = 0;
    textures[TEX_FIRE] = 0;
    textures[TEX_SMOKE] = 0;
    textures[TEX_DEBRIS] = 0;
    textures[TEX_BIRD] = 0;
    textures[TEX_HORSES] = 0;
    textures[TEX_SCORCH] = 0;
    textures[TEX_BANNER] = 0;
    textures[TEX_MIXED_FOREST] = 0;
    textures[TEX_ARROWS] = 0;
    textures[TEX_BATTERING_RAMS] = 0;
    textures[TEX_CATAPULT_STONES] = 0;
    textures[TEX_FALLEN_WARRIOR] = 0;
    textures[TEX_KNIGHT_FORMATIONS] = 0;
    textures[TEX_DRUMS] = 0;
    textures[TEX_CAMPFIRE] = 0;
    textures[TEX_TREBUCHET] = 0;
    textures[TEX_STARS] = 0;
    textures[TEX_TREES] = 0;
    textures[TEX_HORSE_CAVALRY] = 0;
    textures[TEX_ARCHER_FORMATIONS] = 0;
    textures[TEX_WOOD] = 0;
    textures[TEX_BLOOD_STAINS] = 0;
    textures[TEX_ARMOR] = 0;
    textures[TEX_SKIN] = 0;
    textures[TEX_HELMET] = 0;
    textures[TEX_SABATONS] = 0;
    
    texturesLoaded = true;
    printf("Essential textures loaded successfully.\n");
}

// Load texture on-demand to reduce memory usage
void BackgroundRenderer::loadTextureOnDemand(int textureIndex) {
    if (textureIndex < 0 || textureIndex >= 50 || textures[textureIndex] != 0) {
        return; // Already loaded or invalid index
    }
    
    const char* filenames[] = {
        "texturess/Battlefield Terrain.bmp",  // TEX_BATTLEFIELD (0)
        "texturess/sky.bmp",                  // TEX_SKY (1)
        "texturess/sun.bmp",                  // TEX_SUN (2)
        "texturess/moon.bmp",                 // TEX_MOON (3)
        "texturess/clouds.bmp",               // TEX_CLOUDS (4)
        "texturess/mountain.bmp",             // TEX_MOUNTAIN (5)
        "texturess/forest.bmp",               // TEX_FOREST (6)
        "texturess/grass.bmp",                // TEX_GRASS (7)
        "texturess/castle.bmp",               // TEX_CASTLE (8)
        "texturess/rain.bmp",                 // TEX_RAIN (9)
        "texturess/storm.bmp",                // TEX_STORM (10)
        "texturess/lightning.bmp",            // TEX_LIGHTNING (11)
        "texturess/fire.bmp",                 // TEX_FIRE (12)
        "texturess/smoke.bmp",                // TEX_SMOKE (13)
        "texturess/debris.bmp",               // TEX_DEBRIS (14)
        "texturess/bird.bmp",                 // TEX_BIRD (15)
        "texturess/horses.bmp",               // TEX_HORSES (16)
        "texturess/Scorch Marks.bmp",         // TEX_SCORCH (17)
        "texturess/tattered banner.bmp",      // TEX_BANNER (18)
        "texturess/mixed forest.bmp",         // TEX_MIXED_FOREST (19)
        "texturess/arrows.bmp",               // TEX_ARROWS (20)
        "texturess/battering rams.bmp",       // TEX_BATTERING_RAMS (21)
        "texturess/catapult stones.bmp",      // TEX_CATAPULT_STONES (22)
        "texturess/fallen warrior.bmp",       // TEX_FALLEN_WARRIOR (23)
        "texturess/knight formations.bmp",    // TEX_KNIGHT_FORMATIONS (24)
        "texturess/drums.bmp",                // TEX_DRUMS (25)
        "texturess/campfire.bmp",             // TEX_CAMPFIRE (26)
        "texturess/trebuchet.bmp",            // TEX_TREBUCHET (27)
        "texturess/stars.bmp",                // TEX_STARS (28)
        "texturess/trees.bmp",                // TEX_TREES (29)
        "texturess/horse cavalry.bmp",        // TEX_HORSE_CAVALRY (30)
        "texturess/archer formations.bmp",    // TEX_ARCHER_FORMATIONS (31)
        "texturess/wood.bmp",                 // TEX_WOOD (32)
        "texturess/blood stains.bmp",         // TEX_BLOOD_STAINS (33)
        "texturess/armor.bmp",                // TEX_ARMOR (34)
        "texturess/skin.bmp",                 // TEX_SKIN (35)
        "texturess/helmet.bmp",               // TEX_HELMET (36)
        "texturess/sabatons.bmp"              // TEX_SABATONS (37)
    };
    
    textures[textureIndex] = loadBMPTexture(filenames[textureIndex]);
}

void BackgroundRenderer::bindTexture(int textureIndex) {
    if (texturesLoaded && textureIndex >= 0 && textureIndex < 50) {
        // Load texture on-demand if not already loaded
        if (textures[textureIndex] == 0) {
            loadTextureOnDemand(textureIndex);
        }
        
        if (textures[textureIndex] != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textures[textureIndex]);
        }
    }
}

// ===== SKY & ATMOSPHERE =====
void BackgroundRenderer::drawSkyDome() {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    
    // Apply sky texture
    bindTexture(TEX_SKY);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Smooth textured sky dome
    glBegin(GL_QUAD_STRIP);
    
    // Create smooth horizontal bands instead of radial rays
    for (int level = 0; level < 10; level++) {
        float y1 = 80.0f - level * 9.0f;      // Top level
        float y2 = 80.0f - (level + 1) * 9.0f; // Bottom level
        
        // Texture coordinates for vertical gradient
        float topV = (float)level / 10.0f;
        float bottomV = (float)(level + 1) / 10.0f;
        
        // Create circular bands around the sky
        for (int i = 0; i <= 36; i++) { // More segments for smoother appearance
            float angle = i * M_PI / 18.0f;  // 36 segments = 360/10 degrees each
            float radius = 200.0f;
            float u = (float)i / 36.0f; // Texture U coordinate
            
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            
            // Top vertex with texture coordinates
            glTexCoord2f(u, topV);
            glVertex3f(x, y1, z);
            
            // Bottom vertex with texture coordinates
            glTexCoord2f(u, bottomV);
            glVertex3f(x, y2, z);
        }
    }
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void BackgroundRenderer::drawSunOrMoon() {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // If stormy, no sun/moon visible
    if (currentWeather == WEATHER_STORM || currentWeather == WEATHER_RAIN) {
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glPopMatrix();
        return;
    }

    // 20-second cycle: first 10 seconds = day (sun), next 10 seconds = night (moon)
    float cycleTime = fmod(dayNightTime, 20.0f);
    bool isDay = (cycleTime < 10.0f);
    
    // Position sun/moon high in the sky - make sure it's visible
    glTranslatef(0.0f, 80.0f, -120.0f); // Higher and more centered
    
    if (isDay) {
        // DAY TIME - SUN (make it more visible)
        // Outer atmospheric glow
        glColor4f(1.0f, 0.9f, 0.7f, 0.15f);
        gluSphere(quadric, 12.0f, 16, 12);
        
        // Sun corona
        glColor4f(1.0f, 0.85f, 0.6f, 0.3f);
        gluSphere(quadric, 8.0f, 18, 14);
        
        // Main sun disc with texture - use a textured quad facing the camera
        bindTexture(TEX_SUN);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-6.0f, -6.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(6.0f, -6.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(6.0f, 6.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-6.0f, 6.0f, 0.0f);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        
        // Inner bright core for additional glow effect
        glColor4f(1.0f, 1.0f, 0.9f, 0.4f);
        gluSphere(quadric, 2.0f, 16, 12);
    } else {
        // NIGHT TIME - MOON
        // Moon outer glow
        glColor4f(0.8f, 0.8f, 1.0f, 0.12f);
        gluSphere(quadric, 8.0f, 16, 12);
        
        // Moon halo
        glColor4f(0.7f, 0.7f, 0.9f, 0.2f);
        gluSphere(quadric, 6.0f, 18, 14);
        
        // Main moon disc with texture - use a textured quad facing the camera
        bindTexture(TEX_MOON);
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-5.0f, -5.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(5.0f, -5.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(5.0f, 5.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-5.0f, 5.0f, 0.0f);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void BackgroundRenderer::drawStars() {
    // Only draw stars during night time
    float cycleTime = fmod(dayNightTime, 20.0f);
    bool isNight = (cycleTime >= 10.0f);
    
    if (!isNight || currentWeather == WEATHER_STORM || currentWeather == WEATHER_RAIN) {
        return;
    }
    
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw stars as small bright points scattered across the sky
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glPointSize(3.0f);
    
    glBegin(GL_POINTS);
    
    // Generate consistent star positions using fixed seed patterns
    for (int i = 0; i < 50; i++) {
        // Use trigonometric functions to create consistent star positions
        float x = 150.0f * cos(i * 2.37f) * sin(i * 1.73f);
        float y = 40.0f + 30.0f * sin(i * 1.91f);
        float z = -120.0f + 50.0f * cos(i * 2.11f);
        
        // Add subtle twinkling effect
        float twinkle = 0.8f + 0.2f * sin(windTime * 3.0f + i * 0.7f);
        glColor4f(twinkle, twinkle, 1.0f, 0.7f);
        
        glVertex3f(x, y, z);
    }
    
    // Add some brighter stars
    glPointSize(4.0f);
    for (int i = 0; i < 15; i++) {
        float x = 180.0f * cos(i * 3.14f) * sin(i * 2.41f);
        float y = 50.0f + 25.0f * sin(i * 2.67f);
        float z = -130.0f + 40.0f * cos(i * 3.33f);
        
        // Brighter twinkling
        float twinkle = 0.9f + 0.1f * sin(windTime * 2.0f + i * 0.5f);
        glColor4f(twinkle, twinkle, 1.0f, 0.9f);
        
        glVertex3f(x, y, z);
    }
    
    glEnd();
    
    glPointSize(1.0f); // Reset point size
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void BackgroundRenderer::drawClouds() {
    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING); // Clouds are self-illuminated for simplicity
    
    // Apply cloud texture
    bindTexture(TEX_CLOUDS);
    
    // High altitude clouds layer with texture
    glPushMatrix();
    glTranslatef(0.0f, 45.0f, -120.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f); // Use white to show texture properly
    for (int high = 0; high < 6; high++) {
        glPushMatrix();
        float x = -120.0f + high * 40.0f + sin(cloudTime * 0.3f + high) * 20.0f;
        float z = cos(cloudTime * 0.2f + high) * 25.0f;
        glTranslatef(x, 0.0f, z);
        
        // Draw cloud as textured quad
        float size = 15.0f + sin(cloudTime + high) * 3.0f;
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -2.0f, -size*0.5f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(size, -2.0f, -size*0.5f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(size, 2.0f, size*0.5f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, 2.0f, size*0.5f);
        glEnd();
        
        glPopMatrix();
    }
    glPopMatrix();
    
    // Main cloud layer - more dramatic formations with texture
    glTranslatef(0.0f, 25.0f, -60.0f);

    float r, g, b, alpha;
    if (currentWeather == WEATHER_RAIN || currentWeather == WEATHER_STORM) {
        // Much darker and more dramatic storm clouds
        r = 0.2f + lightningBrightness * 0.8f; // Flash brighter during lightning
        g = 0.2f + lightningBrightness * 0.8f; 
        b = 0.3f + lightningBrightness * 0.7f; // Slight blue tint
        alpha = 0.95f; // Very dense storm clouds
    } else {
        r = 1.0f; g = 1.0f; b = 1.0f; alpha = 0.7f; // Bright white clouds
    }
    
    // Large cumulus clouds with texture
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        float x = -100.0f + i * 25.0f + sin(cloudTime * 0.8f + i) * 12.0f;
        float y = sin(cloudTime * 0.4f + i * 0.9f) * 6.0f;
        float z = cos(cloudTime * 0.25f + i) * 20.0f;
        glTranslatef(fmod(x + cloudTime * 15.0f, 250.0f) - 125.0f, y, z);
        
        // Cloud color variation for realism
        float cloudVariation = sin(cloudTime + i * 1.3f) * 0.1f;
        glColor4f(r + cloudVariation, g + cloudVariation, b + cloudVariation, alpha);
        
        // Main cloud body as textured billboards
        float size = 8.0f + sin(cloudTime + i) * 2.0f;
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -size*0.4f, -size*0.6f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(size, -size*0.4f, -size*0.6f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(size, size*0.4f, size*0.6f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, size*0.4f, size*0.6f);
        glEnd();
        glTranslatef(4.0f, 3.0f, -3.0f);
        gluSphere(quadric, 5.0f, 10, 6);
        
        // Additional cloud detail for storm clouds
        if (currentWeather == WEATHER_STORM || currentWeather == WEATHER_RAIN) {
            glColor4f(r * 0.7f, g * 0.7f, b * 0.7f, alpha * 1.1f);
            glTranslatef(2.0f, -4.0f, 1.0f);
            gluSphere(quadric, 4.0f, 8, 6);
            glTranslatef(-8.0f, -2.0f, 0.0f);
            gluSphere(quadric, 3.5f, 8, 6);
        }
        
        glPopMatrix();
    }
    
    // Additional storm cloud layer for dramatic effect
    if (currentWeather == WEATHER_STORM) {
        glTranslatef(0.0f, -5.0f, 10.0f);
        
        // Apply storm texture for realistic storm clouds
        bindTexture(TEX_STORM);
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f); // White to show storm texture clearly
        
        for (int storm = 0; storm < 12; storm++) {
            glPushMatrix();
            float x = -150.0f + storm * 25.0f + sin(cloudTime * 1.2f + storm) * 15.0f;
            float y = sin(cloudTime * 0.6f + storm * 1.1f) * 3.0f;
            float z = cos(cloudTime * 0.4f + storm) * 12.0f;
            glTranslatef(x, y, z);
            
            // Large, menacing storm clouds with texture
            float stormSize = 10.0f + sin(cloudTime * 0.8f + storm) * 3.0f;
            
            // Draw storm clouds as textured quads
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-stormSize, -stormSize*0.5f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(stormSize, -stormSize*0.5f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(stormSize, stormSize*0.5f, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-stormSize, stormSize*0.5f, 0.0f);
            glEnd();
            
            // Wispy extensions with storm texture
            glTranslatef(8.0f, -2.0f, 0.0f);
            float wispySize = stormSize * 0.6f;
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-wispySize, -wispySize*0.5f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(wispySize, -wispySize*0.5f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(wispySize, wispySize*0.5f, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-wispySize, wispySize*0.5f, 0.0f);
            glEnd();
            
            glTranslatef(-16.0f, -1.0f, 3.0f);
            wispySize = stormSize * 0.7f;
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-wispySize, -wispySize*0.5f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(wispySize, -wispySize*0.5f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(wispySize, wispySize*0.5f, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-wispySize, wispySize*0.5f, 0.0f);
            glEnd();
            
            glPopMatrix();
        }
        glDisable(GL_TEXTURE_2D);
    }
    
    // Distant cloud layer for depth
    glTranslatef(0.0f, 10.0f, -40.0f);
    glColor4f(r * 0.6f, g * 0.6f, b * 0.6f, alpha * 0.5f); // Fainter distant clouds
    
    for (int distant = 0; distant < 15; distant++) {
        glPushMatrix();
        float x = -200.0f + distant * 27.0f + sin(cloudTime * 0.5f + distant) * 25.0f;
        float y = sin(cloudTime * 0.3f + distant * 0.7f) * 8.0f;
        float z = cos(cloudTime * 0.2f + distant) * 30.0f;
        glTranslatef(x, y, z);
        
        // Smaller, more scattered distant clouds
        gluSphere(quadric, 4.0f + sin(cloudTime + distant) * 1.0f, 8, 6);
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glPopMatrix();
}

void BackgroundRenderer::setupFog() {
    if (currentWeather == WEATHER_FOG || currentWeather == WEATHER_STORM) {
        glEnable(GL_FOG);
        GLfloat fogColor[4];
        if (currentWeather == WEATHER_STORM) {
            fogColor[0] = 0.1f; fogColor[1] = 0.1f; fogColor[2] = 0.15f; fogColor[3] = 1.0f;
        } else {
            fogColor[0] = 0.5f; fogColor[1] = 0.5f; fogColor[2] = 0.6f; fogColor[3] = 1.0f;
        }
        glFogi(GL_FOG_MODE, GL_EXP2);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_DENSITY, 0.015f);
        glHint(GL_FOG_HINT, GL_NICEST);
    } else {
        glDisable(GL_FOG);
    }
}

void BackgroundRenderer::drawLightning() {
    if (lightningBrightness <= 0.0f) return;

    // Apply lightning texture for dramatic effect
    bindTexture(TEX_LIGHTNING);

    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for bright lightning
    
    // Set up a 2D projection to draw screen effects
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw multiple lightning flashes for more dramatic effect
    float flashIntensity = lightningBrightness;
    
    // Main lightning flash (full screen)
    glColor4f(1.0f, 1.0f, 1.0f, flashIntensity * 0.8f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1, 0);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1, 1);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0, 1);
    glEnd();
    
    // Secondary flicker effect with blue tint
    glColor4f(0.8f, 0.9f, 1.0f, flashIntensity * 0.4f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0.7f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1, 0.7f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1, 1);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0, 1);
    glEnd();
    
    // Multiple lightning bolt streaks in 3D space
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Draw 3D lightning bolts across the sky
    glPushMatrix();
    glTranslatef(0.0f, 60.0f, -100.0f);
    
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        float offsetX = (i - 2) * 40.0f + sin(siegeTime * 3.0f + i) * 20.0f;
        glTranslatef(offsetX, 0.0f, 0.0f);
        
        // Lightning bolt intensity varies per bolt
        float boltIntensity = flashIntensity * (0.6f + (i % 3) * 0.2f);
        glColor4f(1.0f, 1.0f, 1.0f, boltIntensity);
        
        // Jagged lightning bolt path
        glLineWidth(3.0f + flashIntensity * 5.0f);
        glBegin(GL_LINE_STRIP);
        
        float y = 0.0f;
        for (int segment = 0; segment < 12; segment++) {
            float x = sin(segment * 0.5f + i) * 8.0f + (rand() % 10 - 5);
            y -= 8.0f + (rand() % 8);
            float z = (rand() % 20) - 10;
            glVertex3f(x, y, z);
        }
        glEnd();
        glLineWidth(1.0f);
        
        glPopMatrix();
    }
    
    // Restore states
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glPopMatrix();
}

// ===== TERRAIN RENDERING =====
void BackgroundRenderer::drawMountains() {
    glPushMatrix();
    
    // Layered mountain ranges using GL_TRIANGLES for dramatic peaks
    // Far mountains - ink wash style silhouettes
    glTranslatef(0.0f, 0.0f, -150.0f);
    glColor3f(0.15f, 0.15f, 0.25f); // Deep blue-purple silhouette
    
    glBegin(GL_TRIANGLES);
    // Create jagged mountain silhouette
    for (int i = 0; i < 12; i++) {
        float x1 = -120.0f + i * 20.0f;
        float x2 = x1 + 15.0f;
        float x3 = x1 + 25.0f;
        float height1 = 30.0f + sin(i * 1.2f) * 15.0f;
        float height2 = 25.0f + sin(i * 1.5f) * 12.0f;
        
        // Mountain peak triangle
        glVertex3f(x1, 0.0f, 0.0f);
        glVertex3f(x2, height1, -5.0f);
        glVertex3f(x3, 0.0f, 0.0f);
        
        // Secondary peak
        glVertex3f(x2, 0.0f, 0.0f);
        glVertex3f(x2 + 8.0f, height2, -3.0f);
        glVertex3f(x3, 0.0f, 0.0f);
    }
    glEnd();
    
    // Mid-range mountains with more detail and texture
    glTranslatef(0.0f, 0.0f, 60.0f);
    bindTexture(TEX_MOUNTAIN);
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture
    
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 20; i++) {
        float x = -100.0f + i * 10.0f;
        float height = 20.0f + sin(i * 0.8f) * 12.0f + cos(i * 1.3f) * 8.0f;
        float u = (float)i / 20.0f; // Texture U coordinate
        
        glTexCoord2f(u, 0.0f);
        glVertex3f(x, 0.0f, 0.0f);
        glTexCoord2f(u, 1.0f);
        glVertex3f(x, height, -10.0f + sin(i) * 3.0f);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    
    // Foreground dramatic peaks - the mountain pass
    glTranslatef(0.0f, 0.0f, 80.0f);
    glColor3f(0.4f, 0.35f, 0.3f); // Rocky brown-grey
    
    // Left side of the pass
    glBegin(GL_TRIANGLES);
    glVertex3f(-60.0f, 0.0f, 0.0f);
    glVertex3f(-30.0f, 45.0f, -15.0f);
    glVertex3f(-15.0f, 15.0f, -8.0f);
    
    glVertex3f(-30.0f, 45.0f, -15.0f);
    glVertex3f(-10.0f, 25.0f, -12.0f);
    glVertex3f(-15.0f, 15.0f, -8.0f);
    
    // Right side of the pass
    glVertex3f(15.0f, 15.0f, -8.0f);
    glVertex3f(30.0f, 45.0f, -15.0f);
    glVertex3f(60.0f, 0.0f, 0.0f);
    
    glVertex3f(15.0f, 15.0f, -8.0f);
    glVertex3f(10.0f, 25.0f, -12.0f);
    glVertex3f(30.0f, 45.0f, -15.0f);
    
    // Rocky cliff faces using GL_QUADS
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.35f, 0.3f, 0.25f); // Darker cliff face
    
    // Left cliff face
    glVertex3f(-30.0f, 45.0f, -15.0f);
    glVertex3f(-25.0f, 42.0f, -8.0f);
    glVertex3f(-20.0f, 20.0f, -5.0f);
    glVertex3f(-15.0f, 15.0f, -8.0f);
    
    // Right cliff face
    glVertex3f(15.0f, 15.0f, -8.0f);
    glVertex3f(20.0f, 20.0f, -5.0f);
    glVertex3f(25.0f, 42.0f, -8.0f);
    glVertex3f(30.0f, 45.0f, -15.0f);
    glEnd();
    
    glPopMatrix();
}

void BackgroundRenderer::drawBattlefield() {
    // Apply battlefield terrain texture
    bindTexture(TEX_BATTLEFIELD);
    
    // Realistic muddy battlefield terrain with varied elevations
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture properly
    for (int j = -50; j < 50; j++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = -50; i <= 50; i++) {
            float x1 = i * 2.0f;
            float z1 = j * 2.0f;
            float x2 = i * 2.0f;
            float z2 = (j + 1) * 2.0f;
            
            // More complex height variation for realistic battlefield
            float baseHeight1 = sin(x1 * 0.05f) * cos(z1 * 0.07f) * 2.0f;
            float mudVariation1 = sin(x1 * 0.25f + z1 * 0.35f) * 0.8f; // Mud ruts and puddles
            float battleWear1 = cos(x1 * 0.15f + z1 * 0.1f) * 0.5f; // Battle trampling
            float y1 = baseHeight1 + mudVariation1 + battleWear1;
            
            float baseHeight2 = sin(x2 * 0.05f) * cos(z2 * 0.07f) * 2.0f;
            float mudVariation2 = sin(x2 * 0.25f + z2 * 0.35f) * 0.8f;
            float battleWear2 = cos(x2 * 0.15f + z2 * 0.1f) * 0.5f;
            float y2 = baseHeight2 + mudVariation2 + battleWear2;
            
            // Texture coordinates (repeat across terrain)
            float u1 = (x1 + 100.0f) / 20.0f;
            float v1 = (z1 + 100.0f) / 20.0f;
            float u2 = (x2 + 100.0f) / 20.0f;
            float v2 = (z2 + 100.0f) / 20.0f;
            
            // Add subtle color variations to enhance texture
            float colorVar1 = (y1 + 3.0f) * 0.02f;
            float bloodStain1 = (sin(x1 * 0.35f) + cos(z1 * 0.3f)) > 1.5f ? -0.1f : 0.0f;
            float ashStain1 = (sin(x1 * 0.6f) + cos(z1 * 0.45f)) > 1.7f ? -0.05f : 0.0f;
            
            float colorVar2 = (y2 + 3.0f) * 0.02f;
            float bloodStain2 = (sin(x2 * 0.35f) + cos(z2 * 0.3f)) > 1.5f ? -0.1f : 0.0f;
            float ashStain2 = (sin(x2 * 0.6f) + cos(z2 * 0.45f)) > 1.7f ? -0.05f : 0.0f;

            glNormal3f(0.0f, 1.0f, 0.0f);
            
            glColor3f(1.0f - colorVar1 + bloodStain1 + ashStain1, 
                     1.0f - colorVar1 * 0.5f + bloodStain1 * 0.5f + ashStain1, 
                     1.0f - colorVar1 * 0.3f + ashStain1);
            glTexCoord2f(u1, v1);
            glVertex3f(x1, y1, z1);
            
            glColor3f(1.0f - colorVar2 + bloodStain2 + ashStain2, 
                     1.0f - colorVar2 * 0.5f + bloodStain2 * 0.5f + ashStain2, 
                     1.0f - colorVar2 * 0.3f + ashStain2);
            glTexCoord2f(u2, v2);
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }

    glDisable(GL_TEXTURE_2D); // Disable texture for other elements

    // Add blood stains texture overlays on battlefield terrain
    bindTexture(TEX_BLOOD_STAINS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.8f, 0.2f, 0.1f, 0.7f); // Dark red blood color with transparency
    glDisable(GL_LIGHTING);
    
    // Major blood stain areas from fierce battles
    glBegin(GL_QUADS);
    
    // Large blood pool from major combat
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-25.0f, 0.12f, -10.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-10.0f, 0.12f, -12.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-8.0f, 0.12f, 3.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-23.0f, 0.12f, 5.0f);
    
    // Blood stains near fallen warriors
    glTexCoord2f(0.0f, 0.0f); glVertex3f(12.0f, 0.11f, -18.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(28.0f, 0.11f, -20.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(30.0f, 0.11f, -5.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(14.0f, 0.11f, -3.0f);
    
    // Scattered blood stains from arrow wounds and sword fights
    for (int bloodSpot = 0; bloodSpot < 15; bloodSpot++) {
        float bx = -40.0f + bloodSpot * 5.5f + sin(bloodSpot * 1.8f) * 8.0f;
        float bz = -30.0f + bloodSpot * 4.0f + cos(bloodSpot * 1.4f) * 12.0f;
        float size = 2.0f + sin(bloodSpot * 0.9f) * 1.2f;
        
        // Vary blood stain intensity
        float intensity = 0.5f + (bloodSpot % 3) * 0.15f;
        glColor4f(0.7f + intensity * 0.2f, 0.1f + intensity * 0.1f, 0.05f, 0.6f + intensity * 0.2f);
        
        glTexCoord2f(0.0f, 0.0f); glVertex3f(bx - size, 0.08f + bloodSpot * 0.002f, bz - size * 0.8f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(bx + size * 1.2f, 0.08f + bloodSpot * 0.002f, bz - size * 0.6f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(bx + size, 0.08f + bloodSpot * 0.002f, bz + size * 1.1f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(bx - size * 0.9f, 0.08f + bloodSpot * 0.002f, bz + size * 0.9f);
    }
    
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);

    // Multiple realistic battle craters of varying sizes
    glColor3f(0.12f, 0.08f, 0.04f); // Very dark crater soil
    
    // Large catapult impact crater
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-15.0f, -1.5f, 10.0f); // Deeper center
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * M_PI / 20.0f;
        float radius = 8.0f + sin(angle * 4.0f) * 2.0f; // Irregular crater edge
        float rimHeight = 0.4f + cos(angle * 6.0f) * 0.3f; // Raised rim from impact
        glVertex3f(radius * cos(angle) - 15.0f, rimHeight, radius * sin(angle) + 10.0f);
    }
    glEnd();
    
    // Medium explosion crater
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(22.0f, -1.0f, -8.0f);
    for (int i = 0; i <= 16; i++) {
        float angle = i * 2.0f * M_PI / 16.0f;
        float radius = 5.0f + sin(angle * 3.0f) * 1.0f;
        glVertex3f(radius * cos(angle) + 22.0f, 0.15f, radius * sin(angle) - 8.0f);
    }
    glEnd();
    
    // Small artillery impact craters
    for (int crater = 0; crater < 8; crater++) {
        float cx = -35.0f + crater * 12.0f + sin(crater * 2.1f) * 6.0f;
        float cz = -25.0f + crater * 6.0f + cos(crater * 1.7f) * 10.0f;
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(cx, -0.6f, cz);
        for (int i = 0; i <= 12; i++) {
            float angle = i * 2.0f * M_PI / 12.0f;
            float radius = 2.5f + sin(angle * 5.0f) * 0.6f;
            glVertex3f(radius * cos(angle) + cx, 0.08f, radius * sin(angle) + cz);
        }
        glEnd();
    }

    // Defensive trenches and earthworks
    glColor3f(0.18f, 0.12f, 0.08f);
    glBegin(GL_QUADS);
    // Main defensive trench
    for (int i = -40; i <= 40; i += 3) {
        float trenchDepth = -1.2f + sin(i * 0.15f) * 0.3f; // Varying trench depth
        float trenchWidth = 4.0f + cos(i * 0.1f) * 0.8f;
        
        glVertex3f(i, 0.0f, -6.0f);
        glVertex3f(i + 3, 0.0f, -6.0f);
        glVertex3f(i + 3, trenchDepth, -6.0f + trenchWidth * 0.5f);
        glVertex3f(i, trenchDepth, -6.0f + trenchWidth * 0.5f);
        
        // Trench back wall
        glVertex3f(i, trenchDepth, -6.0f + trenchWidth * 0.5f);
        glVertex3f(i + 3, trenchDepth, -6.0f + trenchWidth * 0.5f);
        glVertex3f(i + 3, -0.2f, -6.0f + trenchWidth);
        glVertex3f(i, -0.2f, -6.0f + trenchWidth);
    }
    glEnd();

    // Extensive scorch marks and burn patterns
    bindTexture(TEX_SCORCH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    
    // Larger, more realistic scorch marks with texture coordinates
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-30.0f, 0.1f, -18.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-18.0f, 0.1f, -20.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-15.0f, 0.1f, -8.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-27.0f, 0.1f, -6.0f);
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(15.0f, 0.1f, -15.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(32.0f, 0.1f, -18.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(35.0f, 0.1f, -5.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(18.0f, 0.1f, -2.0f);
    
    // Additional burn patches from Greek fire and flaming arrows
    for (int burn = 0; burn < 12; burn++) {
        float bx = -50.0f + burn * 8.0f + sin(burn * 1.5f) * 6.0f;
        float bz = -30.0f + burn * 5.0f + cos(burn * 1.2f) * 8.0f;
        float size = 3.0f + sin(burn) * 1.5f;
        
        glTexCoord2f(0.0f, 0.0f); glVertex3f(bx - size, 0.05f, bz - size);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(bx + size, 0.05f, bz - size * 0.7f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(bx + size * 0.8f, 0.05f, bz + size);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(bx - size * 0.7f, 0.05f, bz + size * 0.8f);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    
    // Add Chinese war elements
    drawChineseWarDrums();
    drawChineseDragonBanners(); 
    drawChineseFireLances();
    
    // Add Western war elements
    drawWesternKnightFormations();
    drawWesternCrossbowSquads();
    drawWesternWarHorns();
}

// ===== STRUCTURE RENDERING =====
void BackgroundRenderer::drawChineseFortress() {
    glPushMatrix();
    glTranslatef(-45.0f, 0.0f, -35.0f);
    glScalef(1.5f, 1.5f, 1.5f);
    
    // Apply castle texture to fortress walls
    bindTexture(TEX_CASTLE);
    glColor3f(0.8f, 0.6f, 0.5f); // Warm color to blend with texture
    glBegin(GL_QUADS);
    
    // Front wall with traditional Chinese architecture and texture
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-15.0f, 0.0f, 8.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(15.0f, 0.0f, 8.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(15.0f, 12.0f, 8.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-15.0f, 12.0f, 8.0f);
    
    // Side walls with texture
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-15.0f, 0.0f, -8.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-15.0f, 0.0f, 8.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-15.0f, 12.0f, 8.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-15.0f, 12.0f, -8.0f);
    
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(15.0f, 0.0f, 8.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(15.0f, 0.0f, -8.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(15.0f, 12.0f, -8.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(15.0f, 12.0f, 8.0f);
    
    // Back wall with texture
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(15.0f, 0.0f, -8.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-15.0f, 0.0f, -8.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-15.0f, 12.0f, -8.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(15.0f, 12.0f, -8.0f);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    
    // Traditional Chinese curved roof using GL_TRIANGLE_FAN
    glColor3f(0.6f, 0.1f, 0.1f); // Traditional red roof
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 18.0f, 0.0f); // Peak
    
    for (int i = 0; i <= 16; i++) {
        float angle = i * 2.0f * M_PI / 16.0f;
        float x = 18.0f * cos(angle);
        float z = 18.0f * sin(angle);
        float y = 12.0f + 2.0f * sin(angle * 2.0f); // Curved eaves
        glVertex3f(x, y, z);
    }
    glEnd();
    
    // Corner towers using gluCylinder for realism
    glColor3f(0.35f, 0.2f, 0.15f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        float x = (i % 2 == 0) ? -18.0f : 18.0f;
        float z = (i < 2) ? -10.0f : 10.0f;
        glTranslatef(x, 0.0f, z);
        gluCylinder(quadric, 2.0f, 1.8f, 15.0f, 12, 1);
        
        // Tower roof
        glTranslatef(0.0f, 15.0f, 0.0f);
        glColor3f(0.6f, 0.1f, 0.1f);
        gluCylinder(quadric, 2.5f, 0.0f, 4.0f, 8, 1);
        glColor3f(0.35f, 0.2f, 0.15f);
        glPopMatrix();
    }
    
    // Damage effects - broken wall section using GL_TRIANGLES
    glColor3f(0.3f, 0.15f, 0.1f); // Darker damaged color
    glBegin(GL_TRIANGLES);
    // Breach in front wall
    glVertex3f(-5.0f, 0.0f, 8.1f);
    glVertex3f(5.0f, 0.0f, 8.1f);
    glVertex3f(2.0f, 6.0f, 8.1f);
    
    glVertex3f(-5.0f, 0.0f, 8.1f);
    glVertex3f(2.0f, 6.0f, 8.1f);
    glVertex3f(-3.0f, 4.0f, 8.1f);
    glEnd();
    
    // Traditional Chinese decorative elements using GL_LINES
    glColor3f(0.8f, 0.6f, 0.2f); // Gold decorative trim
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    // Horizontal decorative lines
    for (int i = 0; i < 3; i++) {
        float y = 4.0f + i * 3.0f;
        glVertex3f(-14.0f, y, 8.1f); glVertex3f(14.0f, y, 8.1f);
    }
    // Vertical dividers
    for (int i = -2; i <= 2; i++) {
        float x = i * 6.0f;
        glVertex3f(x, 0.0f, 8.1f); glVertex3f(x, 12.0f, 8.1f);
    }
    glEnd();
    
    glPopMatrix();
}

void BackgroundRenderer::drawWesternCastle() {
    glPushMatrix();
    glTranslatef(50.0f, 0.0f, -45.0f);
    
    // Main keep using GL_POLYGON for irregular shape
    glColor3f(0.35f, 0.35f, 0.4f); // Stone grey
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, 1.0f);
    // Irregular fortress base
    glVertex3f(-8.0f, 0.0f, 0.0f);
    glVertex3f(-6.0f, 0.0f, 0.0f);
    glVertex3f(-4.0f, 2.0f, 0.0f);
    glVertex3f(4.0f, 2.0f, 0.0f);
    glVertex3f(6.0f, 0.0f, 0.0f);
    glVertex3f(8.0f, 0.0f, 0.0f);
    glVertex3f(8.0f, 20.0f, 0.0f);
    glVertex3f(-8.0f, 20.0f, 0.0f);
    glEnd();
    
    // Main cylindrical keep using gluCylinder
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -8.0f);
    gluCylinder(quadric, 7.0f, 6.5f, 18.0f, 16, 1);
    glPopMatrix();
    
    // Battlements using GL_QUADS
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_QUADS);
    for (int i = 0; i < 24; i += 2) { // Every other merlon
        float angle1 = i * 2.0f * M_PI / 24.0f;
        float angle2 = (i + 1) * 2.0f * M_PI / 24.0f;
        float x1 = 7.2f * cos(angle1); float z1 = 7.2f * sin(angle1) - 8.0f;
        float x2 = 7.2f * cos(angle2); float z2 = 7.2f * sin(angle2) - 8.0f;
        
        glNormal3f(cos(angle1), 0.0f, sin(angle1));
        glVertex3f(x1, 18.0f, z1); glVertex3f(x2, 18.0f, z2);
        glVertex3f(x2, 20.0f, z2); glVertex3f(x1, 20.0f, z1);
    }
    glEnd();
    
    // Corner towers in various states of damage
    // Intact tower
    glPushMatrix();
    glTranslatef(12.0f, 0.0f, -12.0f);
    glColor3f(0.32f, 0.32f, 0.37f);
    gluCylinder(quadric, 3.0f, 2.7f, 22.0f, 12, 1);
    
    // Tower cap
    glTranslatef(0.0f, 22.0f, 0.0f);
    glColor3f(0.25f, 0.25f, 0.3f);
    gluCylinder(quadric, 3.5f, 0.0f, 6.0f, 12, 1);
    glPopMatrix();
    
    // Damaged tower using GL_TRIANGLE_STRIP
    glPushMatrix();
    glTranslatef(-12.0f, 0.0f, -8.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 8; i++) { // Half circle only
        float angle = i * M_PI / 8.0f;
        float x = 2.8f * cos(angle);
        float z = 2.8f * sin(angle);
        float height = 15.0f - fabs(i - 4.0f) * 1.5f; // Jagged damage
        
        glVertex3f(x, 0.0f, z);
        glVertex3f(x, height, z);
    }
    glEnd();
    glPopMatrix();
    
    // Siege damage - collapsed wall section using GL_TRIANGLES
    glColor3f(0.25f, 0.25f, 0.3f); // Darker rubble color
    glBegin(GL_TRIANGLES);
    // Rubble pile
    glVertex3f(-6.0f, 0.0f, 2.0f);
    glVertex3f(-2.0f, 3.0f, 3.0f);
    glVertex3f(2.0f, 0.0f, 2.0f);
    
    glVertex3f(-4.0f, 0.0f, 4.0f);
    glVertex3f(0.0f, 2.5f, 4.0f);
    glVertex3f(4.0f, 0.0f, 4.0f);
    glEnd();
    
    // Arrow slits using GL_LINES
    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 8; i++) {
        float angle = i * 2.0f * M_PI / 8.0f;
        float x = 6.8f * cos(angle);
        float z = 6.8f * sin(angle) - 8.0f;
        float y = 8.0f + i * 1.5f;
        
        glVertex3f(x, y, z);
        glVertex3f(x * 1.1f, y + 1.0f, z * 1.1f);
    }
    glEnd();
    
    glPopMatrix();
}

// ===== SIEGE BATTLE EFFECTS =====
void BackgroundRenderer::drawSiegeEffects() {
    // Draw flying arrows using GL_LINES
    glColor3f(0.4f, 0.3f, 0.2f); // Dark wood shaft
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (const auto& arrow : arrows) {
        glVertex3f(arrow.position.x, arrow.position.y, arrow.position.z);
        glVertex3f(arrow.position.x - arrow.velocity.x * 0.1f, 
                  arrow.position.y - arrow.velocity.y * 0.1f, 
                  arrow.position.z - arrow.velocity.z * 0.1f);
    }
    glEnd();
    
    // Arrow heads using GL_TRIANGLES
    glColor3f(0.3f, 0.3f, 0.3f); // Steel arrowheads
    glBegin(GL_TRIANGLES);
    for (const auto& arrow : arrows) {
        float length = 0.3f;
        glVertex3f(arrow.position.x, arrow.position.y, arrow.position.z);
        glVertex3f(arrow.position.x - length, arrow.position.y - 0.1f, arrow.position.z);
        glVertex3f(arrow.position.x - length, arrow.position.y + 0.1f, arrow.position.z);
    }
    glEnd();
    
    // Sparks from metalwork using GL_POINTS
    glColor3f(1.0f, 0.8f, 0.2f); // Bright yellow-orange sparks
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (const auto& spark : sparks) {
        glVertex3f(spark.position.x, spark.position.y, spark.position.z);
    }
    glEnd();
    
    // Fire embers using GL_POINTS with larger size
    glColor3f(1.0f, 0.4f, 0.1f); // Orange-red embers
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (const auto& ember : embers) {
        glVertex3f(ember.position.x, ember.position.y, ember.position.z);
    }
    glEnd();
    
    // Reset point size
    glPointSize(1.0f);
}

void BackgroundRenderer::drawCatapultStones() {
    // Large siege stones in flight using textured spheres for realism
    bindTexture(TEX_MOUNTAIN); // Use mountain texture for stone appearance
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture
    
    // Animate several stones
    for (int i = 0; i < 3; i++) {
        glPushMatrix();
        float time_offset = siegeTime + i * 2.0f;
        float arc_progress = fmod(time_offset * 0.3f, 1.0f);
        
        float x = -70.0f + arc_progress * 140.0f; // Across the battlefield
        float z = -30.0f + sin(arc_progress * M_PI) * 20.0f;
        float y = arc_progress * (1.0f - arc_progress) * 40.0f; // Parabolic arc
        
        glTranslatef(x, y, z);
        gluSphere(quadric, 1.2f + i * 0.3f, 12, 8);
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}

void BackgroundRenderer::drawSiegeLadders() {
    // Siege ladders against the walls using textured quads
    bindTexture(TEX_CASTLE); // Use castle texture for wooden appearance
    glColor3f(0.8f, 0.6f, 0.4f); // Warm wood color to blend with texture
    
    // Ladder against Chinese fortress
    glPushMatrix();
    glTranslatef(-42.0f, 0.0f, -27.0f);
    glRotatef(-75.0f, 1.0f, 0.0f, 0.0f); // Leaning against wall
    
    glBegin(GL_QUADS);
    // Left rail with texture coordinates
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, 0.0f); 
    glTexCoord2f(0.1f, 0.0f); glVertex3f(-0.8f, 0.0f, 0.0f);
    glTexCoord2f(0.1f, 1.0f); glVertex3f(-0.8f, 20.0f, 0.0f); 
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 20.0f, 0.0f);
    
    // Right rail with texture coordinates
    glTexCoord2f(0.9f, 0.0f); glVertex3f(0.8f, 0.0f, 0.0f); 
    glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 20.0f, 0.0f); 
    glTexCoord2f(0.9f, 1.0f); glVertex3f(0.8f, 20.0f, 0.0f);
    
    // Rungs with texture coordinates
    for (int i = 1; i < 15; i++) {
        float y = i * 1.3f;
        float v1 = y / 20.0f;
        float v2 = (y + 0.2f) / 20.0f;
        
        glTexCoord2f(0.0f, v1); glVertex3f(-1.0f, y, 0.0f); 
        glTexCoord2f(1.0f, v1); glVertex3f(1.0f, y, 0.0f);
        glTexCoord2f(1.0f, v2); glVertex3f(1.0f, y + 0.2f, 0.0f); 
        glTexCoord2f(0.0f, v2); glVertex3f(-1.0f, y + 0.2f, 0.0f);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void BackgroundRenderer::drawSiegeTowers() {
    // Massive siege towers approaching the walls with castle texture
    bindTexture(TEX_CASTLE); // Use castle texture for wooden structure
    glColor3f(0.7f, 0.55f, 0.4f); // Weathered wood color to blend with texture
    
    // Chinese siege tower
    glPushMatrix();
    glTranslatef(-25.0f, 0.0f, -15.0f);
    
    // Tower base - wider for stability with texture coordinates
    glBegin(GL_QUADS);
    for (int level = 0; level < 4; level++) {
        float baseY = level * 6.0f;
        float width = 6.0f - level * 0.4f; // Tapering tower
        float height = 6.0f;
        
        // Front face with texture coordinates
        glTexCoord2f(0.0f, (float)level / 4.0f); glVertex3f(-width, baseY, width);
        glTexCoord2f(1.0f, (float)level / 4.0f); glVertex3f(width, baseY, width);
        glTexCoord2f(1.0f, (float)(level + 1) / 4.0f); glVertex3f(width, baseY + height, width);
        glTexCoord2f(0.0f, (float)(level + 1) / 4.0f); glVertex3f(-width, baseY + height, width);
        
        // Side faces with texture coordinates
        glTexCoord2f(0.0f, (float)level / 4.0f); glVertex3f(-width, baseY, -width);
        glTexCoord2f(1.0f, (float)level / 4.0f); glVertex3f(-width, baseY, width);
        glTexCoord2f(1.0f, (float)(level + 1) / 4.0f); glVertex3f(-width, baseY + height, width);
        glTexCoord2f(0.0f, (float)(level + 1) / 4.0f); glVertex3f(-width, baseY + height, -width);
        
        glTexCoord2f(0.0f, (float)level / 4.0f); glVertex3f(width, baseY, width);
        glTexCoord2f(1.0f, (float)level / 4.0f); glVertex3f(width, baseY, -width);
        glTexCoord2f(1.0f, (float)(level + 1) / 4.0f); glVertex3f(width, baseY + height, -width);
        glTexCoord2f(0.0f, (float)(level + 1) / 4.0f); glVertex3f(width, baseY + height, width);
        
        // Back face with texture coordinates
        glTexCoord2f(0.0f, (float)level / 4.0f); glVertex3f(width, baseY, -width);
        glTexCoord2f(1.0f, (float)level / 4.0f); glVertex3f(-width, baseY, -width);
        glTexCoord2f(1.0f, (float)(level + 1) / 4.0f); glVertex3f(-width, baseY + height, -width);
        glTexCoord2f(0.0f, (float)(level + 1) / 4.0f); glVertex3f(width, baseY + height, -width);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    
    // Wooden supports and framework
    glColor3f(0.25f, 0.2f, 0.15f);
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    // Vertical supports
    for (int support = 0; support < 4; support++) {
        float x = (support % 2 == 0) ? -4.0f : 4.0f;
        float z = (support < 2) ? 4.0f : -4.0f;
        glVertex3f(x, 0.0f, z);
        glVertex3f(x, 20.0f, z);
    }
    // Horizontal braces
    for (int level = 1; level < 4; level++) {
        float y = level * 6.0f;
        glVertex3f(-4.0f, y, -4.0f); glVertex3f(4.0f, y, -4.0f);
        glVertex3f(-4.0f, y, 4.0f); glVertex3f(4.0f, y, 4.0f);
        glVertex3f(-4.0f, y, -4.0f); glVertex3f(-4.0f, y, 4.0f);
        glVertex3f(4.0f, y, -4.0f); glVertex3f(4.0f, y, 4.0f);
    }
    glEnd();
    
    // Wheels (large wooden wheels)
    glColor3f(0.35f, 0.28f, 0.2f);
    for (int wheel = 0; wheel < 4; wheel++) {
        glPushMatrix();
        float wx = (wheel % 2 == 0) ? -5.0f : 5.0f;
        float wz = (wheel < 2) ? 5.0f : -5.0f;
        glTranslatef(wx, 1.5f, wz);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadric, 1.5f, 1.5f, 0.8f, 8, 1);
        // Wheel spokes
        glColor3f(0.3f, 0.23f, 0.18f);
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        for (int spoke = 0; spoke < 8; spoke++) {
            float angle = spoke * M_PI / 4.0f;
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(cos(angle) * 1.3f, sin(angle) * 1.3f, 0.0f);
        }
        glEnd();
        glPopMatrix();
    }
    
    glPopMatrix();
    
    // Western siege tower - different design
    glPushMatrix();
    glTranslatef(35.0f, 0.0f, -20.0f);
    glColor3f(0.28f, 0.23f, 0.18f);
    
    // More angular European design
    glBegin(GL_QUADS);
    // Main tower structure
    glVertex3f(-4.0f, 0.0f, 4.0f); glVertex3f(4.0f, 0.0f, 4.0f);
    glVertex3f(4.0f, 18.0f, 4.0f); glVertex3f(-4.0f, 18.0f, 4.0f);
    
    glVertex3f(-4.0f, 0.0f, -4.0f); glVertex3f(-4.0f, 0.0f, 4.0f);
    glVertex3f(-4.0f, 18.0f, 4.0f); glVertex3f(-4.0f, 18.0f, -4.0f);
    
    glVertex3f(4.0f, 0.0f, 4.0f); glVertex3f(4.0f, 0.0f, -4.0f);
    glVertex3f(4.0f, 18.0f, -4.0f); glVertex3f(4.0f, 18.0f, 4.0f);
    
    glVertex3f(4.0f, 0.0f, -4.0f); glVertex3f(-4.0f, 0.0f, -4.0f);
    glVertex3f(-4.0f, 18.0f, -4.0f); glVertex3f(4.0f, 18.0f, -4.0f);
    glEnd();
    
    glPopMatrix();
}

void BackgroundRenderer::drawTrebuchets() {
    // Large trebuchets for long-range siege warfare
    glColor3f(0.3f, 0.2f, 0.15f); // Dark siege engine wood
    
    // Chinese trebuchet
    glPushMatrix();
    glTranslatef(-50.0f, 0.0f, 25.0f);
    
    // Base framework - massive wooden structure
    glBegin(GL_QUADS);
    // Base platform
    glVertex3f(-8.0f, 0.0f, -8.0f); glVertex3f(8.0f, 0.0f, -8.0f);
    glVertex3f(8.0f, 1.0f, 8.0f); glVertex3f(-8.0f, 1.0f, 8.0f);
    glEnd();
    
    // A-frame supports
    glColor3f(0.25f, 0.18f, 0.12f);
    glLineWidth(8.0f);
    glBegin(GL_LINES);
    // Left A-frame
    glVertex3f(-6.0f, 1.0f, -2.0f); glVertex3f(-3.0f, 12.0f, 0.0f);
    glVertex3f(-6.0f, 1.0f, 2.0f); glVertex3f(-3.0f, 12.0f, 0.0f);
    glVertex3f(-6.0f, 1.0f, -2.0f); glVertex3f(-6.0f, 1.0f, 2.0f);
    
    // Right A-frame
    glVertex3f(6.0f, 1.0f, -2.0f); glVertex3f(3.0f, 12.0f, 0.0f);
    glVertex3f(6.0f, 1.0f, 2.0f); glVertex3f(3.0f, 12.0f, 0.0f);
    glVertex3f(6.0f, 1.0f, -2.0f); glVertex3f(6.0f, 1.0f, 2.0f);
    
    // Cross beam (fulcrum)
    glVertex3f(-3.0f, 12.0f, 0.0f); glVertex3f(3.0f, 12.0f, 0.0f);
    glEnd();
    
    // Throwing arm
    float armAngle = 45.0f + sin(siegeTime * 0.3f) * 15.0f; // Animated loading/firing
    glPushMatrix();
    glTranslatef(0.0f, 12.0f, 0.0f);
    glRotatef(armAngle, 0.0f, 0.0f, 1.0f);
    
    // Long throwing arm
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 15.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, -8.0f, 0.0f); // Counter-weight arm
    glEnd();
    
    // Counter-weight (heavy stone box)
    glPushMatrix();
    glTranslatef(0.0f, -10.0f, 0.0f);
    glColor3f(0.4f, 0.4f, 0.45f); // Stone weight
    glScalef(2.0f, 2.0f, 2.0f);
    gluSphere(quadric, 1.0f, 8, 6);
    glPopMatrix();
    
    // Projectile at end of arm
    glPushMatrix();
    glTranslatef(0.0f, 15.0f, 0.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    gluSphere(quadric, 0.8f, 6, 4);
    glPopMatrix();
    
    glPopMatrix(); // End throwing arm
    glPopMatrix(); // End trebuchet
    
    // Western trebuchet - slightly different position and design
    glPushMatrix();
    glTranslatef(45.0f, 0.0f, 30.0f);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f); // Facing opposite direction
    
    // Similar structure but smaller
    glColor3f(0.28f, 0.2f, 0.14f);
    glScalef(0.8f, 0.8f, 0.8f);
    
    // Base and supports (similar to Chinese but smaller)
    glBegin(GL_QUADS);
    glVertex3f(-6.0f, 0.0f, -6.0f); glVertex3f(6.0f, 0.0f, -6.0f);
    glVertex3f(6.0f, 0.8f, 6.0f); glVertex3f(-6.0f, 0.8f, 6.0f);
    glEnd();
    
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    // A-frame supports
    glVertex3f(-4.0f, 0.8f, -1.5f); glVertex3f(-2.0f, 10.0f, 0.0f);
    glVertex3f(-4.0f, 0.8f, 1.5f); glVertex3f(-2.0f, 10.0f, 0.0f);
    glVertex3f(4.0f, 0.8f, -1.5f); glVertex3f(2.0f, 10.0f, 0.0f);
    glVertex3f(4.0f, 0.8f, 1.5f); glVertex3f(2.0f, 10.0f, 0.0f);
    glVertex3f(-2.0f, 10.0f, 0.0f); glVertex3f(2.0f, 10.0f, 0.0f);
    glEnd();
    
    glPopMatrix();
}

void BackgroundRenderer::drawBatteringRams() {
    // Massive battering rams for breaching gates
    glColor3f(0.25f, 0.2f, 0.15f); // Heavy siege wood
    
    // Chinese battering ram
    glPushMatrix();
    glTranslatef(-30.0f, 0.0f, -8.0f);
    
    // Protective housing (tortoise/mantlet)
    glColor3f(0.3f, 0.25f, 0.2f);
    glBegin(GL_TRIANGLES);
    // Sloped roof to deflect arrows and stones
    glVertex3f(-8.0f, 2.0f, -3.0f); glVertex3f(8.0f, 2.0f, -3.0f); glVertex3f(0.0f, 6.0f, 0.0f);
    glVertex3f(-8.0f, 2.0f, 3.0f); glVertex3f(-8.0f, 2.0f, -3.0f); glVertex3f(0.0f, 6.0f, 0.0f);
    glVertex3f(8.0f, 2.0f, -3.0f); glVertex3f(8.0f, 2.0f, 3.0f); glVertex3f(0.0f, 6.0f, 0.0f);
    glVertex3f(8.0f, 2.0f, 3.0f); glVertex3f(-8.0f, 2.0f, 3.0f); glVertex3f(0.0f, 6.0f, 0.0f);
    glEnd();
    
    // Side walls
    glBegin(GL_QUADS);
    glVertex3f(-8.0f, 0.0f, -3.0f); glVertex3f(-8.0f, 0.0f, 3.0f);
    glVertex3f(-8.0f, 2.0f, 3.0f); glVertex3f(-8.0f, 2.0f, -3.0f);
    
    glVertex3f(8.0f, 0.0f, 3.0f); glVertex3f(8.0f, 0.0f, -3.0f);
    glVertex3f(8.0f, 2.0f, -3.0f); glVertex3f(8.0f, 2.0f, 3.0f);
    glEnd();
    
    // The massive ram itself - suspended inside
    glColor3f(0.2f, 0.15f, 0.1f); // Very dark heavy wood
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    
    // Ram shaft
    gluCylinder(quadric, 0.6f, 0.6f, 12.0f, 8, 2);
    
    // Iron ram head
    glTranslatef(0.0f, 0.0f, 12.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    gluCylinder(quadric, 0.8f, 0.4f, 2.0f, 6, 2);
    glPopMatrix();
    
    // Wheels
    glColor3f(0.35f, 0.3f, 0.25f);
    for (int wheel = 0; wheel < 4; wheel++) {
        glPushMatrix();
        float wx = (wheel % 2 == 0) ? -6.0f : 6.0f;
        float wz = (wheel < 2) ? 2.8f : -2.8f;
        glTranslatef(wx, 1.0f, wz);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadric, 1.0f, 1.0f, 0.6f, 6, 1);
        glPopMatrix();
    }
    
    glPopMatrix();
    
    // European battering ram - different design
    glPushMatrix();
    glTranslatef(25.0f, 0.0f, -12.0f);
    glColor3f(0.28f, 0.23f, 0.18f);
    
    // More open framework design
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    // Support frame
    glVertex3f(-6.0f, 0.0f, -2.0f); glVertex3f(-6.0f, 4.0f, -2.0f);
    glVertex3f(6.0f, 0.0f, -2.0f); glVertex3f(6.0f, 4.0f, -2.0f);
    glVertex3f(-6.0f, 0.0f, 2.0f); glVertex3f(-6.0f, 4.0f, 2.0f);
    glVertex3f(6.0f, 0.0f, 2.0f); glVertex3f(6.0f, 4.0f, 2.0f);
    
    // Cross braces
    glVertex3f(-6.0f, 4.0f, -2.0f); glVertex3f(6.0f, 4.0f, -2.0f);
    glVertex3f(-6.0f, 4.0f, 2.0f); glVertex3f(6.0f, 4.0f, 2.0f);
    glVertex3f(-6.0f, 4.0f, -2.0f); glVertex3f(-6.0f, 4.0f, 2.0f);
    glVertex3f(6.0f, 4.0f, -2.0f); glVertex3f(6.0f, 4.0f, 2.0f);
    glEnd();
    
    // Ram suspended by chains
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glColor3f(0.22f, 0.18f, 0.14f);
    gluCylinder(quadric, 0.5f, 0.5f, 8.0f, 8, 2);
    
    // Iron head
    glTranslatef(0.0f, 0.0f, 8.0f);
    glColor3f(0.25f, 0.25f, 0.3f);
    gluSphere(quadric, 0.8f, 6, 4);
    glPopMatrix();
    
    glPopMatrix();
}

void BackgroundRenderer::drawArrowVolleys() {
    // Massive arrow volleys flying overhead (increased quantities)
    glColor3f(0.3f, 0.2f, 0.1f); // Dark arrow shafts
    glLineWidth(2.0f);
    
    glBegin(GL_LINES);
    // Multiple volleys at different stages of flight (increased back to 3 volleys)
    for (int volley = 0; volley < 3; volley++) {
        float volleyTime = fmod(siegeTime + volley * 2.0f, 8.0f); // 8 second cycle
        float volleyX = -60.0f + volleyTime * 15.0f; // Flies across battlefield
        float volleyHeight = 25.0f + sin(volleyTime * M_PI / 4.0f) * 8.0f; // Arc trajectory
        
        // 40 arrows per volley (increased from 25)
        for (int arrow = 0; arrow < 40; arrow++) {
            float arrowX = volleyX + (arrow % 10) * 2.0f - 10.0f;  // Better spacing
            float arrowZ = -30.0f + (arrow / 10) * 3.0f;           // Better spacing
            float arrowY = volleyHeight + sin(arrow * 0.3f) * 1.0f;
            
            // Arrow shaft
            glVertex3f(arrowX, arrowY, arrowZ);
            glVertex3f(arrowX + 1.5f, arrowY - 0.3f, arrowZ + 0.2f);
        }
    }
    glEnd();
    
    // Arrow heads
    glColor3f(0.25f, 0.25f, 0.3f);
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (int volley = 0; volley < 3; volley++) {  // Increased back to 3 volleys
        float volleyTime = fmod(siegeTime + volley * 2.0f, 8.0f);
        float volleyX = -60.0f + volleyTime * 15.0f;
        float volleyHeight = 25.0f + sin(volleyTime * M_PI / 4.0f) * 8.0f;
        
        for (int arrow = 0; arrow < 40; arrow++) {  // Increased from 25 to 40
            float arrowX = volleyX + (arrow % 10) * 2.0f - 10.0f;  // Better spacing
            float arrowZ = -30.0f + (arrow / 10) * 3.0f;           // Better spacing
            float arrowY = volleyHeight + sin(arrow * 0.3f) * 1.0f;
            
            glVertex3f(arrowX + 1.5f, arrowY - 0.3f, arrowZ + 0.2f);
        }
    }
    glEnd();
    glPointSize(1.0f);
}

// ===== VEGETATION =====
void BackgroundRenderer::drawTrees() {
    // Enable forest texture for realistic tree rendering
    bindTexture(TEX_FOREST);
    
    // Ink-wash style pine trees using GL_TRIANGLES
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    
    // Stylized Chinese pine trees on mountainsides
    for (int i = 0; i < 6; i++) {
        glPushMatrix();
        float x = -40.0f + i * 15.0f + sin(i) * 8.0f;
        float z = -20.0f + cos(i * 1.5f) * 12.0f;
        float height = 8.0f + sin(i * 2.0f) * 3.0f;
        
        glTranslatef(x, 0.0f, z);
        
        // Tree trunk using GL_QUADS with texture coordinates
        glBegin(GL_QUADS);
        // Front face
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.3f, 0.0f, -0.3f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(0.3f, 0.0f, -0.3f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(0.3f, height * 0.6f, -0.3f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.3f, height * 0.6f, -0.3f);
        
        // Right face  
        glTexCoord2f(0.0f, 0.0f); glVertex3f(0.3f, 0.0f, -0.3f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(0.3f, 0.0f, 0.3f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(0.3f, height * 0.6f, 0.3f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(0.3f, height * 0.6f, -0.3f);
        glEnd();
        
        // Pine needles/branches using GL_TRIANGLES with mixed forest texture
        bindTexture(TEX_MIXED_FOREST);
        glBegin(GL_TRIANGLES);
        for (int layer = 0; layer < 4; layer++) {
            float y_base = height * 0.3f + layer * height * 0.2f;
            float radius = 3.0f - layer * 0.6f;
            
            // Create layered triangular foliage with texture coordinates
            for (int side = 0; side < 6; side++) {
                float angle = side * M_PI / 3.0f;
                float wind_sway = sin(windTime + i + layer) * 0.3f;
                
                glTexCoord2f(0.5f, 1.0f); glVertex3f(0.0f, y_base + radius, 0.0f);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(radius * cos(angle) + wind_sway, y_base, radius * sin(angle));
                glTexCoord2f(1.0f, 0.0f); glVertex3f(radius * cos(angle + M_PI/3) + wind_sway, y_base, radius * sin(angle + M_PI/3));
            }
        }
        glEnd();
        glPopMatrix();
    }
    
    // War-damaged/burnt trees using GL_LINES
    glColor3f(0.15f, 0.1f, 0.08f); // Charred black-brown
    glLineWidth(4.0f);
    
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        float x = -30.0f + i * 8.0f;
        float z = 10.0f + sin(i * 2.0f) * 6.0f;
        glTranslatef(x, 0.0f, z);
        
        glBegin(GL_LINES);
        // Main trunk
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.2f, 6.0f + i * 0.5f, 0.1f);
        
        // Broken branches
        float trunk_height = 6.0f + i * 0.5f;
        for (int branch = 0; branch < 3; branch++) {
            float branch_y = trunk_height * (0.3f + branch * 0.25f);
            float branch_angle = branch * 2.0f;
            
            glVertex3f(0.2f, branch_y, 0.1f);
            glVertex3f(1.5f * cos(branch_angle), branch_y + 1.0f, 1.5f * sin(branch_angle));
        }
        glEnd();
        glPopMatrix();
    }
    
    glLineWidth(1.0f);
}

void BackgroundRenderer::drawGrass() {
    // Apply grass texture for realistic battlefield vegetation
    bindTexture(TEX_GRASS);
    
    // Sparse battlefield grass using textured quads instead of lines
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    
    glBegin(GL_QUADS);
    for (int i = 0; i < 300; i++) {
        float x = -80.0f + (rand() % 16000) / 100.0f;
        float z = -50.0f + (rand() % 10000) / 100.0f;
        
        // Avoid grass in fortress and battle areas
        if ((x > -50 && x < -30 && z > -40 && z < -25) || // Chinese fortress
            (x > 40 && x < 60 && z > -50 && z < -35) ||   // Western castle
            (x > 15 && x < 25 && z > -15 && z < -5)) {    // Fire area
            continue;
        }
        
        float height = 0.3f + (rand() % 100) / 300.0f;
        float sway = sin(windTime * 1.5f + x * 0.1f) * 0.2f;
        float width = 0.05f + (rand() % 50) / 1000.0f;
        
        // Draw grass as small textured quads
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x - width, 0.0f, z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x + width, 0.0f, z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x + width + sway, height, z);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(x - width + sway, height, z);
    }
    glEnd();
    
    // Battlefield herbs and small bushes using GL_TRIANGLE_FAN
    glColor3f(0.15f, 0.25f, 0.1f);
    for (int i = 0; i < 15; i++) {
        glPushMatrix();
        float x = -60.0f + (rand() % 12000) / 100.0f;
        float z = -40.0f + (rand() % 8000) / 100.0f;
        
        // Skip if in structure areas
        if ((x > -50 && x < -30 && z > -40 && z < -25) ||
            (x > 40 && x < 60 && z > -50 && z < -35)) {
            glPopMatrix();
            continue;
        }
        
        glTranslatef(x, 0.0f, z);
        
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.5f, 0.0f); // Center
        for (int j = 0; j <= 8; j++) {
            float angle = j * 2.0f * M_PI / 8.0f;
            float radius = 0.8f + sin(angle * 3.0f) * 0.3f;
            glVertex3f(radius * cos(angle), 0.0f, radius * sin(angle));
        }
        glEnd();
        glPopMatrix();
    }
}

// ===== BATTLE AFTERMATH =====
void BackgroundRenderer::drawWeaponsAndDebris() {
    // Apply debris texture for realistic battlefield remnants
    bindTexture(TEX_DEBRIS);
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    
    // Chinese jian swords scattered across battlefield - as textured quads
    glBegin(GL_QUADS);
    // Sword 1
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-18.5f, 0.1f, 7.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-14.5f, 0.1f, 11.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-14.3f, 0.1f, 11.3f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-18.3f, 0.1f, 7.3f);
    
    // Sword 2
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-22.5f, 0.15f, -12.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-19.0f, 0.15f, -8.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-18.8f, 0.15f, -8.7f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-22.3f, 0.15f, -12.7f);
    
    // European longsword
    glTexCoord2f(0.0f, 0.0f); glVertex3f(31.5f, 0.08f, -8.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(36.5f, 0.08f, -5.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(36.3f, 0.08f, -5.7f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(31.3f, 0.08f, -8.7f);
    glEnd();
    
    // Broken shields as textured triangles
    glBegin(GL_TRIANGLES);
    // Chinese shield fragment
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-32.0f, 0.05f, 12.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-28.0f, 0.05f, 10.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(-30.0f, 2.2f, 14.0f);
    
    // European shield piece
    glTexCoord2f(0.0f, 0.0f); glVertex3f(22.0f, 0.1f, -18.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(26.0f, 0.1f, -16.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(24.5f, 1.5f, -14.0f);
    glEnd();
    
    // More shield fragments scattered
    glColor3f(0.2f, 0.15f, 0.1f); // Wooden shield backing
    glVertex3f(-8.0f, 0.08f, -8.0f); glVertex3f(-5.0f, 0.08f, -7.0f); glVertex3f(-6.2f, 0.8f, -5.5f);
    glVertex3f(35.0f, 0.12f, 5.0f); glVertex3f(38.0f, 0.12f, 6.5f); glVertex3f(37.0f, 1.0f, 8.0f);
    glEnd();
    
    // Helmets and armor pieces scattered on battlefield
    glColor3f(0.3f, 0.3f, 0.35f); // Dented steel armor
    for (int armor = 0; armor < 15; armor++) {
        glPushMatrix();
        float ax = -40.0f + armor * 5.5f + sin(armor * 1.8f) * 8.0f;
        float az = -25.0f + armor * 3.2f + cos(armor * 1.4f) * 12.0f;
        float ay = 0.1f + cos(armor * 0.7f) * 0.08f;
        
        glTranslatef(ax, ay, az);
        glRotatef(armor * 25.0f, 0.0f, 1.0f, 0.0f); // Random rotations
        glScalef(0.8f, 0.4f, 0.6f); // Flattened helmet shape
        gluSphere(quadric, 1.2f, 8, 6);
        glPopMatrix();
    }
    
    // Arrows scattered and stuck in ground
    glColor3f(0.35f, 0.22f, 0.12f); // Arrow shafts
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    
    // Arrows stuck in ground at various angles
    for (int arrow = 0; arrow < 25; arrow++) {
        float arrowX = -45.0f + arrow * 3.8f + sin(arrow * 2.2f) * 4.0f;
        float arrowZ = -30.0f + arrow * 2.5f + cos(arrow * 1.9f) * 6.0f;
        float arrowAngle = arrow * 0.5f;
        float arrowLength = 3.5f + sin(arrow * 1.1f) * 1.0f;
        
        glVertex3f(arrowX, 0.0f, arrowZ);
        glVertex3f(arrowX + sin(arrowAngle) * arrowLength, 
                  arrowLength * 0.6f + cos(arrow * 1.3f) * 0.8f, 
                  arrowZ + cos(arrowAngle) * arrowLength);
    }
    glEnd();
    
    // Battle axes and war hammers
    glColor3f(0.4f, 0.3f, 0.2f); // Wooden handles
    glLineWidth(7.0f);
    glBegin(GL_LINES);
    glVertex3f(-15.0f, 0.1f, -25.0f); glVertex3f(-12.0f, 0.1f, -22.0f);
    glVertex3f(20.0f, 0.15f, 18.0f); glVertex3f(23.5f, 0.15f, 21.0f);
    glVertex3f(-38.0f, 0.08f, 3.0f); glVertex3f(-35.0f, 0.08f, 6.5f);
    glEnd();
    
    // Axe heads and hammer heads
    glColor3f(0.28f, 0.28f, 0.32f);
    glBegin(GL_QUADS);
    // Battle axe blades
    glVertex3f(-12.0f, 0.1f, -22.0f); glVertex3f(-11.5f, 1.2f, -21.5f);
    glVertex3f(-10.8f, 1.0f, -20.8f); glVertex3f(-11.3f, 0.1f, -21.3f);
    
    glVertex3f(23.5f, 0.15f, 21.0f); glVertex3f(24.2f, 1.3f, 21.7f);
    glVertex3f(24.8f, 1.1f, 22.3f); glVertex3f(24.1f, 0.15f, 21.6f);
    glEnd();
    
    // Extensive rubble and debris near damaged structures
    glColor3f(0.28f, 0.28f, 0.32f); // Stone rubble
    for(int i = 0; i < 25; i++) {
        glPushMatrix();
        float rubbleX = 35.0f + (rand() % 200) / 10.0f - 10.0f;
        float rubbleZ = -50.0f + (rand() % 200) / 10.0f;
        float rubbleY = 0.0f;
        glTranslatef(rubbleX, rubbleY, rubbleZ);
        glRotatef(rand() % 360, 1.0f, 0.5f, 0.8f);
        glScalef(0.4f + (rand() % 100) / 200.0f, 
                0.2f + (rand() % 100) / 300.0f, 
                0.5f + (rand() % 100) / 150.0f);
        gluSphere(quadric, 1.2f, 6, 4);
        glPopMatrix();
    }
    
    // Wooden debris from siege engines and structures
    glColor3f(0.3f, 0.2f, 0.12f); // Splintered wood
    for(int wood = 0; wood < 20; wood++) {
        glPushMatrix();
        float wx = -50.0f + (rand() % 1000) / 10.0f;
        float wz = -40.0f + (rand() % 800) / 10.0f;
        glTranslatef(wx, 0.05f, wz);
        glRotatef(rand() % 180, 0.0f, 1.0f, 0.0f);
        glScalef(2.0f + (rand() % 100) / 50.0f, 0.3f, 0.5f);
        gluSphere(quadric, 0.8f, 4, 3); // Low-poly for performance
        glPopMatrix();
    }
    
    // Broken siege equipment pieces - catapult arms, ladder rungs
    glColor3f(0.25f, 0.15f, 0.1f); // Dark weathered wood
    glLineWidth(8.0f);
    glBegin(GL_LINES);
    // Broken catapult arm
    glVertex3f(-42.0f, 0.2f, -15.0f); glVertex3f(-38.0f, 2.5f, -12.0f);
    glVertex3f(-38.0f, 2.5f, -12.0f); glVertex3f(-36.0f, 1.8f, -10.5f); // Broken end
    
    // Siege ladder pieces
    glVertex3f(38.0f, 0.1f, -25.0f); glVertex3f(42.0f, 0.8f, -22.0f);
    glVertex3f(40.0f, 0.1f, -28.0f); glVertex3f(44.0f, 1.2f, -25.0f);
    glEnd();
}

void BackgroundRenderer::drawTatteredBanner(float x, float y, float z, float height, float width, float r, float g, float b) {
    // Pole
    glColor3f(0.3f, 0.2f, 0.1f);
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    glVertex3f(x, 0.0f, z);
    glVertex3f(x, height, z);
    glEnd();

    // Tattered cloth with texture
    bindTexture(TEX_BANNER);
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture
    
    glBegin(GL_QUADS);
    // Main banner quad with texture coordinates
    float windOffset1 = sin(windTime * 2.5f) * 0.5f;
    float windOffset2 = sin(windTime * 2.5f + 1.0f) * 0.3f;
    
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x, height, z);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + width + windOffset1, height, z);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + width + windOffset2, height - 3.0f, z);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x, height - 3.0f, z);
    
    // Add tattered edge effect with additional small quads
    for (int i = 0; i < 3; i++) {
        float segmentHeight = height - 3.0f - i * 1.0f;
        float jaggedOffset = sin(i * 2.0f + windTime) * 0.3f;
        
        glTexCoord2f(0.0f, (float)i / 3.0f); 
        glVertex3f(x + i * 0.2f, segmentHeight, z);
        glTexCoord2f(1.0f, (float)i / 3.0f); 
        glVertex3f(x + width * 0.8f + jaggedOffset, segmentHeight, z);
        glTexCoord2f(1.0f, (float)(i + 1) / 3.0f); 
        glVertex3f(x + width * 0.7f + jaggedOffset, segmentHeight - 0.8f, z);
        glTexCoord2f(0.0f, (float)(i + 1) / 3.0f); 
        glVertex3f(x + i * 0.3f, segmentHeight - 0.8f, z);
    }
    glEnd();
}

void BackgroundRenderer::drawBanners() {
    // Multiple Chinese banners and standards
    drawTatteredBanner(-25.0f, 0.0f, -20.0f, 12.0f, 4.0f, 0.7f, 0.1f, 0.1f); // Main Chinese banner
    drawTatteredBanner(-45.0f, 0.0f, -15.0f, 10.0f, 3.0f, 0.6f, 0.4f, 0.0f); // Orange Chinese banner
    drawTatteredBanner(-15.0f, 0.0f, -30.0f, 8.0f, 2.5f, 0.8f, 0.2f, 0.1f); // Yellow Chinese standard
    
    // Western banners and heraldry
    drawTatteredBanner(30.0f, 0.0f, -25.0f, 10.0f, 3.0f, 0.1f, 0.1f, 0.6f); // Main Western banner
    drawTatteredBanner(45.0f, 0.0f, -18.0f, 9.0f, 2.8f, 0.6f, 0.1f, 0.1f); // Red Western banner
    drawTatteredBanner(25.0f, 0.0f, -35.0f, 7.0f, 2.0f, 0.1f, 0.4f, 0.1f); // Green Western standard
    
    // Fallen banners on battlefield
    drawTatteredBanner(-8.0f, 0.0f, 5.0f, 6.0f, 2.0f, 0.5f, 0.0f, 0.5f); // Purple fallen standard
    drawTatteredBanner(12.0f, 0.0f, -8.0f, 4.0f, 1.5f, 0.4f, 0.4f, 0.0f); // Small fallen banner
    
    // Military camp elements
    // Command tents - Chinese camp
    glColor3f(0.6f, 0.2f, 0.1f); // Red silk tent
    glPushMatrix();
    glTranslatef(-60.0f, 0.0f, 20.0f);
    
    // Large command tent
    glBegin(GL_TRIANGLES);
    // Tent front
    glVertex3f(0.0f, 6.0f, 0.0f); // Peak
    glVertex3f(-4.0f, 0.0f, -4.0f); glVertex3f(4.0f, 0.0f, -4.0f);
    // Tent sides
    glVertex3f(0.0f, 6.0f, 0.0f);
    glVertex3f(4.0f, 0.0f, -4.0f); glVertex3f(4.0f, 0.0f, 4.0f);
    glVertex3f(0.0f, 6.0f, 0.0f);
    glVertex3f(4.0f, 0.0f, 4.0f); glVertex3f(-4.0f, 0.0f, 4.0f);
    glVertex3f(0.0f, 6.0f, 0.0f);
    glVertex3f(-4.0f, 0.0f, 4.0f); glVertex3f(-4.0f, 0.0f, -4.0f);
    glEnd();
    
    // Tent guy ropes
    glColor3f(0.4f, 0.3f, 0.2f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (int rope = 0; rope < 8; rope++) {
        float angle = rope * M_PI / 4.0f;
        float tentX = cos(angle) * 4.0f;
        float tentZ = sin(angle) * 4.0f;
        glVertex3f(tentX, 0.0f, tentZ);
        glVertex3f(cos(angle) * 8.0f, 0.0f, sin(angle) * 8.0f);
    }
    glEnd();
    glPopMatrix();
    
    // Smaller Chinese tents
    for (int tent = 0; tent < 5; tent++) {
        glPushMatrix();
        float tx = -70.0f + tent * 8.0f + sin(tent * 1.5f) * 3.0f;
        float tz = 15.0f + tent * 2.0f + cos(tent * 1.2f) * 4.0f;
        glTranslatef(tx, 0.0f, tz);
        glRotatef(tent * 30.0f, 0.0f, 1.0f, 0.0f);
        
        glColor3f(0.5f + tent * 0.1f, 0.1f + tent * 0.05f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 3.0f, 0.0f);
        glVertex3f(-2.0f, 0.0f, -2.0f); glVertex3f(2.0f, 0.0f, -2.0f);
        glVertex3f(0.0f, 3.0f, 0.0f);
        glVertex3f(2.0f, 0.0f, -2.0f); glVertex3f(2.0f, 0.0f, 2.0f);
        glVertex3f(0.0f, 3.0f, 0.0f);
        glVertex3f(2.0f, 0.0f, 2.0f); glVertex3f(-2.0f, 0.0f, 2.0f);
        glVertex3f(0.0f, 3.0f, 0.0f);
        glVertex3f(-2.0f, 0.0f, 2.0f); glVertex3f(-2.0f, 0.0f, -2.0f);
        glEnd();
        glPopMatrix();
    }
    
    // Western camp - different tent style
    glColor3f(0.1f, 0.1f, 0.5f); // Blue canvas
    glPushMatrix();
    glTranslatef(55.0f, 0.0f, 25.0f);
    
    // Pavilion-style tent
    glBegin(GL_QUADS);
    // More rectangular European tent
    glVertex3f(-5.0f, 0.0f, -3.0f); glVertex3f(5.0f, 0.0f, -3.0f);
    glVertex3f(5.0f, 4.0f, -1.0f); glVertex3f(-5.0f, 4.0f, -1.0f);
    
    glVertex3f(-5.0f, 4.0f, -1.0f); glVertex3f(5.0f, 4.0f, -1.0f);
    glVertex3f(5.0f, 4.0f, 1.0f); glVertex3f(-5.0f, 4.0f, 1.0f);
    
    glVertex3f(-5.0f, 4.0f, 1.0f); glVertex3f(5.0f, 4.0f, 1.0f);
    glVertex3f(5.0f, 0.0f, 3.0f); glVertex3f(-5.0f, 0.0f, 3.0f);
    glEnd();
    
    // Side walls
    glBegin(GL_QUADS);
    glVertex3f(-5.0f, 0.0f, -3.0f); glVertex3f(-5.0f, 0.0f, 3.0f);
    glVertex3f(-5.0f, 4.0f, 1.0f); glVertex3f(-5.0f, 4.0f, -1.0f);
    
    glVertex3f(5.0f, 0.0f, 3.0f); glVertex3f(5.0f, 0.0f, -3.0f);
    glVertex3f(5.0f, 4.0f, -1.0f); glVertex3f(5.0f, 4.0f, 1.0f);
    glEnd();
    glPopMatrix();
    
    // Western smaller tents
    for (int wtent = 0; wtent < 6; wtent++) {
        glPushMatrix();
        float wx = 45.0f + wtent * 6.0f + sin(wtent * 1.8f) * 2.0f;
        float wz = 18.0f + wtent * 1.5f + cos(wtent * 1.4f) * 3.0f;
        glTranslatef(wx, 0.0f, wz);
        glRotatef(wtent * 45.0f, 0.0f, 1.0f, 0.0f);
        
        glColor3f(0.05f + wtent * 0.08f, 0.05f + wtent * 0.04f, 0.4f + wtent * 0.1f);
        glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 2.5f, 0.0f);
        glVertex3f(-1.5f, 0.0f, -1.5f); glVertex3f(1.5f, 0.0f, -1.5f);
        glVertex3f(0.0f, 2.5f, 0.0f);
        glVertex3f(1.5f, 0.0f, -1.5f); glVertex3f(1.5f, 0.0f, 1.5f);
        glVertex3f(0.0f, 2.5f, 0.0f);
        glVertex3f(1.5f, 0.0f, 1.5f); glVertex3f(-1.5f, 0.0f, 1.5f);
        glVertex3f(0.0f, 2.5f, 0.0f);
        glVertex3f(-1.5f, 0.0f, 1.5f); glVertex3f(-1.5f, 0.0f, -1.5f);
        glEnd();
        glPopMatrix();
    }
    
    // Supply wagons and camp equipment
    glColor3f(0.3f, 0.25f, 0.2f); // Wooden wagons
    
    // Chinese supply wagon
    glPushMatrix();
    glTranslatef(-40.0f, 0.0f, 35.0f);
    
    // Wagon bed
    glBegin(GL_QUADS);
    glVertex3f(-3.0f, 1.0f, -1.5f); glVertex3f(3.0f, 1.0f, -1.5f);
    glVertex3f(3.0f, 1.5f, 1.5f); glVertex3f(-3.0f, 1.5f, 1.5f);
    glEnd();
    
    // Wagon sides
    glBegin(GL_QUADS);
    glVertex3f(-3.0f, 1.0f, -1.5f); glVertex3f(-3.0f, 1.0f, 1.5f);
    glVertex3f(-3.0f, 2.5f, 1.5f); glVertex3f(-3.0f, 2.5f, -1.5f);
    
    glVertex3f(3.0f, 1.0f, 1.5f); glVertex3f(3.0f, 1.0f, -1.5f);
    glVertex3f(3.0f, 2.5f, -1.5f); glVertex3f(3.0f, 2.5f, 1.5f);
    glEnd();
    
    // Wagon wheels
    glColor3f(0.4f, 0.3f, 0.2f);
    for (int wheel = 0; wheel < 4; wheel++) {
        glPushMatrix();
        float wx = (wheel % 2 == 0) ? -2.5f : 2.5f;
        float wz = (wheel < 2) ? 1.0f : -1.0f;
        glTranslatef(wx, 0.8f, wz);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadric, 0.8f, 0.8f, 0.3f, 8, 1);
        glPopMatrix();
    }
    glPopMatrix();
    
    // Western supply wagon - different design
    glPushMatrix();
    glTranslatef(40.0f, 0.0f, 38.0f);
    glColor3f(0.28f, 0.23f, 0.18f);
    
    // More elaborate European wagon
    glBegin(GL_QUADS);
    glVertex3f(-2.5f, 1.2f, -2.0f); glVertex3f(2.5f, 1.2f, -2.0f);
    glVertex3f(2.5f, 1.8f, 2.0f); glVertex3f(-2.5f, 1.8f, 2.0f);
    
    // Canvas cover frame
    glVertex3f(-2.5f, 1.8f, -2.0f); glVertex3f(2.5f, 1.8f, -2.0f);
    glVertex3f(2.5f, 3.0f, 0.0f); glVertex3f(-2.5f, 3.0f, 0.0f);
    
    glVertex3f(-2.5f, 3.0f, 0.0f); glVertex3f(2.5f, 3.0f, 0.0f);
    glVertex3f(2.5f, 1.8f, 2.0f); glVertex3f(-2.5f, 1.8f, 2.0f);
    glEnd();
    
    glPopMatrix();
    
    // Campfires and cooking areas
    drawCampfires();
    
    // Weapon racks and equipment stores
    drawWeaponRacks();
}

void BackgroundRenderer::drawCampfires() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Chinese camp cooking fire
    glPushMatrix();
    glTranslatef(-55.0f, 0.1f, 30.0f);
    for (int i = 0; i < 20; i++) {
        float flicker = sin(windTime * 12.0f + i * 0.8f) * 0.25f;
        float height = 1.0f + sin(windTime * 8.0f + i) * 0.4f;
        float width = 0.3f + flicker;
        float angle = (i * 18.0f + windTime * 35.0f);
        
        glPushMatrix();
        glRotatef(angle, 0, 1, 0);
        glBegin(GL_TRIANGLES);
        // Inner core
        glColor4f(1.0f, 0.9f, 0.4f, 0.6f);
        glVertex3f(0.0f, height, 0.0f);
        glColor4f(1.0f, 0.6f, 0.1f, 0.8f);
        glVertex3f(-width * 0.4f, 0.0f, 0.0f);
        glVertex3f(width * 0.4f, 0.0f, 0.0f);
        
        // Outer flames
        glColor4f(1.0f, 0.4f, 0.0f, 0.4f);
        glVertex3f(0.0f, height * 1.3f, 0.0f);
        glColor4f(0.8f, 0.2f, 0.0f, 0.7f);
        glVertex3f(-width, 0.0f, 0.0f);
        glVertex3f(width, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    
    // Cooking pot over fire
    glColor3f(0.2f, 0.15f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f);
    gluSphere(quadric, 0.6f, 10, 8);
    glPopMatrix();
    
    // Tripod support
    glColor3f(0.3f, 0.25f, 0.2f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    for (int leg = 0; leg < 3; leg++) {
        float angle = leg * 120.0f * M_PI / 180.0f;
        float legX = cos(angle) * 1.2f;
        float legZ = sin(angle) * 1.2f;
        glVertex3f(0.0f, 2.5f, 0.0f);
        glVertex3f(legX, 0.0f, legZ);
    }
    glEnd();
    glPopMatrix();
    
    // Western camp cooking fire
    glPushMatrix();
    glTranslatef(50.0f, 0.1f, 32.0f);
    for (int i = 0; i < 18; i++) {
        float flicker = sin(windTime * 10.0f + i * 0.9f) * 0.2f;
        float height = 0.9f + sin(windTime * 7.0f + i) * 0.35f;
        float width = 0.28f + flicker;
        float angle = (i * 20.0f + windTime * 32.0f);
        
        glPushMatrix();
        glRotatef(angle, 0, 1, 0);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.8f, 0.3f, 0.6f);
        glVertex3f(0.0f, height, 0.0f);
        glColor4f(1.0f, 0.5f, 0.0f, 0.8f);
        glVertex3f(-width * 0.5f, 0.0f, 0.0f);
        glVertex3f(width * 0.5f, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    
    // Western style cauldron
    glColor3f(0.15f, 0.1f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    gluSphere(quadric, 0.5f, 8, 6);
    glPopMatrix();
    
    glPopMatrix();
    
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void BackgroundRenderer::drawWeaponRacks() {
    // Chinese weapon rack
    glColor3f(0.3f, 0.25f, 0.2f); // Dark wood
    glPushMatrix();
    glTranslatef(-50.0f, 0.0f, 25.0f);
    
    // Rack frame
    glBegin(GL_QUADS);
    // Vertical posts
    glVertex3f(-0.1f, 0.0f, -0.1f); glVertex3f(0.1f, 0.0f, -0.1f);
    glVertex3f(0.1f, 3.0f, -0.1f); glVertex3f(-0.1f, 3.0f, -0.1f);
    
    glVertex3f(-2.9f, 0.0f, -0.1f); glVertex3f(-2.7f, 0.0f, -0.1f);
    glVertex3f(-2.7f, 3.0f, -0.1f); glVertex3f(-2.9f, 3.0f, -0.1f);
    
    // Horizontal support
    glVertex3f(-2.9f, 2.5f, -0.1f); glVertex3f(0.1f, 2.5f, -0.1f);
    glVertex3f(0.1f, 2.7f, -0.1f); glVertex3f(-2.9f, 2.7f, -0.1f);
    glEnd();
    
    // Spears leaning against rack
    glLineWidth(6.0f);
    glColor3f(0.4f, 0.3f, 0.2f);
    glBegin(GL_LINES);
    for (int spear = 0; spear < 8; spear++) {
        float sx = -2.5f + spear * 0.35f;
        glVertex3f(sx, 0.0f, 0.0f);
        glVertex3f(sx - 0.3f, 3.5f, -0.5f);
    }
    glEnd();
    
    // Spear heads
    glColor3f(0.6f, 0.6f, 0.65f);
    for (int head = 0; head < 8; head++) {
        float hx = -2.5f + head * 0.35f;
        glPushMatrix();
        glTranslatef(hx - 0.3f, 3.5f, -0.5f);
        glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 0.3f, 0.0f);
        glVertex3f(-0.05f, 0.0f, 0.0f);
        glVertex3f(0.05f, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
    
    // Western weapon rack - different design
    glColor3f(0.28f, 0.23f, 0.18f);
    glPushMatrix();
    glTranslatef(48.0f, 0.0f, 28.0f);
    
    // A-frame style rack
    glBegin(GL_QUADS);
    // Left support
    glVertex3f(-0.1f, 0.0f, 0.0f); glVertex3f(0.1f, 0.0f, 0.0f);
    glVertex3f(-0.5f, 2.5f, 0.0f); glVertex3f(-0.3f, 2.5f, 0.0f);
    
    // Right support
    glVertex3f(2.9f, 0.0f, 0.0f); glVertex3f(3.1f, 0.0f, 0.0f);
    glVertex3f(2.5f, 2.5f, 0.0f); glVertex3f(2.7f, 2.5f, 0.0f);
    
    // Cross beam
    glVertex3f(-0.4f, 2.3f, -0.05f); glVertex3f(2.6f, 2.3f, -0.05f);
    glVertex3f(2.6f, 2.5f, -0.05f); glVertex3f(-0.4f, 2.5f, -0.05f);
    glEnd();
    
    // Swords hanging from rack
    glColor3f(0.5f, 0.5f, 0.55f); // Steel blades
    for (int sword = 0; sword < 6; sword++) {
        float sx = 0.2f + sword * 0.4f;
        glPushMatrix();
        glTranslatef(sx, 2.2f, 0.0f);
        
        // Blade
        glBegin(GL_QUADS);
        glVertex3f(-0.02f, 0.0f, 0.0f); glVertex3f(0.02f, 0.0f, 0.0f);
        glVertex3f(0.02f, -1.5f, 0.0f); glVertex3f(-0.02f, -1.5f, 0.0f);
        glEnd();
        
        // Hilt
        glColor3f(0.4f, 0.3f, 0.2f);
        glBegin(GL_QUADS);
        glVertex3f(-0.03f, 0.0f, 0.0f); glVertex3f(0.03f, 0.0f, 0.0f);
        glVertex3f(0.03f, 0.3f, 0.0f); glVertex3f(-0.03f, 0.3f, 0.0f);
        glEnd();
        
        // Cross guard
        glBegin(GL_QUADS);
        glVertex3f(-0.15f, 0.0f, 0.0f); glVertex3f(0.15f, 0.0f, 0.0f);
        glVertex3f(0.15f, 0.05f, 0.0f); glVertex3f(-0.15f, 0.05f, 0.0f);
        glEnd();
        
        glColor3f(0.5f, 0.5f, 0.55f); // Reset for next blade
        glPopMatrix();
    }
    glPopMatrix();
    
    glLineWidth(1.0f); // Reset line width
}

void BackgroundRenderer::drawChineseWarDrums() {
    // Large Chinese war drums positioned with Chinese forces
    // Apply wood texture as specifically requested for drums
    bindTexture(TEX_WOOD);
    glColor3f(0.8f, 0.4f, 0.2f); // Warm wood color to blend with texture
    
    // Main battle drum near Chinese camp
    glPushMatrix();
    glTranslatef(-65.0f, 0.0f, 15.0f);
    
    // Drum body - cylinder with wood texture
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 16; i++) {
        float angle = i * 2.0f * M_PI / 16.0f;
        float x = 2.5f * cos(angle);
        float z = 2.5f * sin(angle);
        float u = (float)i / 16.0f;
        
        glTexCoord2f(u, 0.0f);
        glVertex3f(x, 0.0f, z);
        glTexCoord2f(u, 1.0f);
        glVertex3f(x, 1.5f, z);
    }
    glEnd();
    
    // Drum heads (top and bottom) - switch to hide texture for drum skin
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.8f, 0.7f, 0.5f); // Animal hide color
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(quadric, 0.0f, 2.5f, 16, 1);
    glTranslatef(0.0f, 0.0f, 1.5f);
    gluDisk(quadric, 0.0f, 2.5f, 16, 1);
    glPopMatrix();
    
    // Drum stand legs - back to wood texture
    bindTexture(TEX_WOOD);
    glColor3f(0.6f, 0.3f, 0.15f); // Darker wood for legs
    for (int leg = 0; leg < 4; leg++) {
        glPushMatrix();
        float angle = leg * M_PI / 2.0f;
        glTranslatef(cos(angle) * 2.8f, -1.0f, sin(angle) * 2.8f);
        
        // Textured cylindrical leg
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 8; i++) {
            float legAngle = i * 2.0f * M_PI / 8.0f;
            float lx = 0.15f * cos(legAngle);
            float lz = 0.15f * sin(legAngle);
            float u = (float)i / 8.0f;
            
            glTexCoord2f(u, 0.0f);
            glVertex3f(lx, 0.0f, lz);
            glTexCoord2f(u, 1.0f);
            glVertex3f(lx, 2.0f, lz);
        }
        glEnd();
        glPopMatrix();
    }
    
    // Decorative dragon carvings on drum sides - disable texture for carvings
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.9f, 0.7f, 0.2f); // Gold dragon
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 8; i++) {
        float angle = i * M_PI / 4.0f;
        float x1 = cos(angle) * 2.3f;
        float z1 = sin(angle) * 2.3f;
        float x2 = cos(angle + 0.3f) * 2.4f;
        float z2 = sin(angle + 0.3f) * 2.4f;
        
        glVertex3f(x1, 0.3f, z1);
        glVertex3f(x2, 0.7f, z2);
    }
    glEnd();
    glPopMatrix();
    
    // Smaller drums around the battlefield - also with wood texture
    bindTexture(TEX_WOOD);
    glColor3f(0.7f, 0.35f, 0.18f); // Slightly different wood tone
    for (int drum = 0; drum < 3; drum++) {
        glPushMatrix();
        float dx = -50.0f + drum * 15.0f;
        float dz = 8.0f + drum * 6.0f;
        glTranslatef(dx, 0.0f, dz);
        glScalef(0.6f, 0.6f, 0.6f);
        
        // Textured drum body
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 12; i++) {
            float angle = i * 2.0f * M_PI / 12.0f;
            float x = 2.0f * cos(angle);
            float z = 2.0f * sin(angle);
            float u = (float)i / 12.0f;
            
            glTexCoord2f(u, 0.0f);
            glVertex3f(x, 0.0f, z);
            glTexCoord2f(u, 1.0f);
            glVertex3f(x, 1.2f, z);
        }
        glEnd();
        
        // Drum heads without texture
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.7f, 0.6f, 0.4f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluDisk(quadric, 0.0f, 2.0f, 12, 1);
        glTranslatef(0.0f, 0.0f, 1.2f);
        gluDisk(quadric, 0.0f, 2.0f, 12, 1);
        glPopMatrix();
        
        bindTexture(TEX_WOOD); // Re-enable for next drum
        glColor3f(0.7f, 0.35f, 0.18f);
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0f);
}

void BackgroundRenderer::drawChineseDragonBanners() {
    // Elaborate Chinese dragon war banners
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Main dragon banner - large and prominent
    glPushMatrix();
    glTranslatef(-40.0f, 0.0f, -10.0f);
    
    // Banner pole - tall and ornate
    glColor3f(0.3f, 0.25f, 0.2f);
    gluCylinder(quadric, 0.2f, 0.2f, 18.0f, 8, 2);
    
    // Banner cloth with dragon design
    glColor4f(0.8f, 0.1f, 0.1f, 0.8f); // Red silk background
    glBegin(GL_QUADS);
    glVertex3f(0.5f, 16.0f, 0.0f);  glVertex3f(8.0f, 16.0f, 0.0f);
    glVertex3f(8.5f, 8.0f, 0.0f);   glVertex3f(0.0f, 8.0f, 0.0f);
    glEnd();
    
    // Dragon emblem - golden lines
    glColor4f(0.9f, 0.8f, 0.2f, 0.9f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    // Dragon body curve
    for (int i = 0; i < 6; i++) {
        float t = i / 5.0f;
        float x1 = 1.0f + t * 6.0f;
        float y1 = 12.0f + sin(t * M_PI * 2.0f) * 2.0f;
        float x2 = 1.5f + (t + 0.2f) * 6.0f;
        float y2 = 12.0f + sin((t + 0.2f) * M_PI * 2.0f) * 2.0f;
        
        if (x2 <= 7.5f) {
            glVertex3f(x1, y1, 0.1f);
            glVertex3f(x2, y2, 0.1f);
        }
    }
    // Dragon claws and features
    glVertex3f(2.0f, 14.0f, 0.1f); glVertex3f(2.5f, 13.5f, 0.1f);
    glVertex3f(6.0f, 10.0f, 0.1f); glVertex3f(6.5f, 9.5f, 0.1f);
    glEnd();
    
    // Banner ornamental tassels
    glColor4f(0.9f, 0.7f, 0.2f, 0.7f);
    for (int tassel = 0; tassel < 5; tassel++) {
        glPushMatrix();
        float tx = 1.0f + tassel * 1.5f;
        glTranslatef(tx, 8.0f, 0.0f);
        glBegin(GL_LINES);
        for (int strand = 0; strand < 3; strand++) {
            float sx = strand * 0.2f - 0.2f;
            float sway = sin(windTime * 2.0f + tassel + strand) * 0.3f;
            glVertex3f(sx, 0.0f, 0.0f);
            glVertex3f(sx + sway, -2.0f, 0.0f);
        }
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
    
    // Smaller dragon standards around Chinese positions
    for (int standard = 0; standard < 4; standard++) {
        glPushMatrix();
        float sx = -55.0f + standard * 12.0f;
        float sz = -5.0f + standard * 4.0f;
        glTranslatef(sx, 0.0f, sz);
        glScalef(0.4f, 0.4f, 0.4f);
        
        // Pole
        glColor3f(0.25f, 0.2f, 0.15f);
        gluCylinder(quadric, 0.15f, 0.15f, 12.0f, 6, 2);
        
        // Small banner
        glColor4f(0.7f, 0.05f, 0.05f, 0.75f);
        glBegin(GL_TRIANGLES);
        glVertex3f(0.3f, 11.0f, 0.0f);
        glVertex3f(4.0f, 10.0f, 0.0f);
        glVertex3f(0.3f, 7.0f, 0.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
}

void BackgroundRenderer::drawChineseFireLances() {
    // Early Chinese firearms - fire lances and primitive cannons
    glColor3f(0.2f, 0.15f, 0.1f); // Dark metal/bamboo
    
    // Fire lance squad position
    glPushMatrix();
    glTranslatef(-35.0f, 0.0f, 5.0f);
    
    // Multiple fire lances arranged for battle
    for (int lance = 0; lance < 8; lance++) {
        glPushMatrix();
        float lx = (lance % 4) * 1.5f;
        float lz = (lance / 4) * 2.0f;
        glTranslatef(lx, 0.0f, lz);
        glRotatef(-30.0f, 0.0f, 0.0f, 1.0f); // Angled up for firing
        
        // Lance shaft (bamboo tube)
        glColor3f(0.4f, 0.3f, 0.1f);
        gluCylinder(quadric, 0.08f, 0.08f, 3.0f, 6, 1);
        
        // Metal spear point
        glColor3f(0.3f, 0.3f, 0.35f);
        glTranslatef(0.0f, 0.0f, 3.0f);
        gluCylinder(quadric, 0.05f, 0.02f, 0.8f, 6, 1);
        
        // Fire effects (during battle)
        if (fmod(siegeTime + lance * 0.3f, 4.0f) < 0.5f) {
            glColor4f(1.0f, 0.6f, 0.1f, 0.7f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0f, 0.0f, -0.2f);
            glVertex3f(0.1f, 0.0f, 0.5f);
            glVertex3f(-0.1f, 0.0f, 0.5f);
            glEnd();
            glDisable(GL_BLEND);
        }
        
        glPopMatrix();
    }
    glPopMatrix();
    
    // Early Chinese cannon
    glPushMatrix();
    glTranslatef(-45.0f, 0.5f, -8.0f);
    glColor3f(0.15f, 0.15f, 0.2f); // Iron/bronze
    
    // Cannon barrel
    gluCylinder(quadric, 0.6f, 0.8f, 4.0f, 12, 2);
    
    // Cannon carriage
    glColor3f(0.3f, 0.25f, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(-1.0f, -0.5f, -1.0f); glVertex3f(1.0f, -0.5f, -1.0f);
    glVertex3f(1.0f, 0.0f, -1.0f);   glVertex3f(-1.0f, 0.0f, -1.0f);
    
    glVertex3f(-1.0f, -0.5f, -3.0f); glVertex3f(1.0f, -0.5f, -3.0f);
    glVertex3f(1.0f, 0.0f, -3.0f);   glVertex3f(-1.0f, 0.0f, -3.0f);
    glEnd();
    
    // Cannon wheels
    glColor3f(0.4f, 0.3f, 0.2f);
    for (int wheel = 0; wheel < 2; wheel++) {
        glPushMatrix();
        float wx = wheel == 0 ? -1.2f : 1.2f;
        glTranslatef(wx, -0.3f, -2.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadric, 0.8f, 0.8f, 0.3f, 8, 1);
        glPopMatrix();
    }
    
    glPopMatrix();
}

void BackgroundRenderer::drawFires() {
    // Apply fire texture for realistic flame effects
    bindTexture(TEX_FIRE);
    
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f); // Semi-transparent white to show texture

    // Major battlefield fire - burning siege equipment
    glPushMatrix();
    glTranslatef(20.0f, 0.1f, -8.0f);
    for (int i = 0; i < 25; i++) {
        float flicker = sin(windTime * 10.0f + i * 0.5f) * 0.3f;
        float height = 2.0f + sin(windTime * 6.0f + i) * 0.8f;
        float width = 0.6f + flicker;
        float angle = (i * 15.0f + windTime * 30.0f);
        
        glPushMatrix();
        glRotatef(angle, 0, 1, 0);
        glBegin(GL_QUADS);
        // Draw fire as textured quads instead of triangles for better texture display
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-width * 0.3f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(width * 0.3f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(width * 0.3f, height * 1.2f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-width * 0.3f, height * 1.2f, 0.0f);
        
        // Outer flame layer
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-width, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(width, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(width * 0.5f, height * 1.6f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-width * 0.5f, height * 1.6f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
    
    // Multiple smaller fires across battlefield
    for (int fireSpot = 0; fireSpot < 5; fireSpot++) {
        glPushMatrix();
        float fx = -40.0f + fireSpot * 16.0f + sin(fireSpot * 2.1f) * 8.0f;
        float fz = -20.0f + fireSpot * 6.0f + cos(fireSpot * 1.7f) * 10.0f;
        glTranslatef(fx, 0.05f, fz);
        
        for (int flame = 0; flame < 15; flame++) {
            float flicker = sin(windTime * 12.0f + flame + fireSpot * 3.0f) * 0.25f;
            float height = 1.2f + sin(windTime * 8.0f + flame) * 0.5f;
            float width = 0.4f + flicker;
            float angle = flame * 24.0f + windTime * 40.0f + fireSpot * 60.0f;
            
            glPushMatrix();
            glRotatef(angle, 0, 1, 0);
            glBegin(GL_TRIANGLES);
            glColor4f(1.0f, 0.9f, 0.3f, 0.5f);
            glVertex3f(0.0f, height, 0.0f);
            glColor4f(1.0f, 0.4f, 0.1f, 0.7f);
            glVertex3f(-width, 0.0f, 0.0f);
            glVertex3f(width, 0.0f, 0.0f);
            glEnd();
            glPopMatrix();
        }
        glPopMatrix();
    }
    
    // Burning structures - fortress fires
    glPushMatrix();
    glTranslatef(-35.0f, 0.2f, -25.0f); // Chinese fortress fire
    for (int i = 0; i < 18; i++) {
        float flicker = sin(windTime * 9.0f + i * 0.7f) * 0.4f;
        float height = 2.5f + sin(windTime * 5.0f + i) * 1.0f;
        float width = 0.8f + flicker;
        
        glPushMatrix();
        glRotatef(i * 20.0f + windTime * 25.0f, 0, 1, 0);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.8f, 0.1f, 0.4f);
        glVertex3f(0.0f, height * 1.4f, 0.0f);
        glColor4f(0.9f, 0.2f, 0.0f, 0.8f);
        glVertex3f(-width, 0.0f, 0.0f);
        glVertex3f(width, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
    
    // Western castle tower fire
    glPushMatrix();
    glTranslatef(45.0f, 0.3f, -40.0f);
    for (int i = 0; i < 20; i++) {
        float flicker = sin(windTime * 11.0f + i * 0.4f) * 0.3f;
        float height = 3.0f + sin(windTime * 7.0f + i) * 1.2f;
        float width = 0.7f + flicker;
        
        glPushMatrix();
        glRotatef(i * 18.0f + windTime * 35.0f, 0, 1, 0);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.9f, 0.4f, 0.3f);
        glVertex3f(0.0f, height * 1.5f, 0.0f);
        glColor4f(1.0f, 0.3f, 0.0f, 0.7f);
        glVertex3f(-width, 0.0f, 0.0f);
        glVertex3f(width, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
    
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// Realistic fallen warriors across the battlefield
void BackgroundRenderer::drawFallenWarriors() {
    glDisable(GL_LIGHTING);
    
    // Apply fallen warrior texture for realistic detail
    bindTexture(TEX_FALLEN_WARRIOR);
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture properly
    
    // Chinese warriors - with enhanced texturing
    for (int warrior = 0; warrior < 12; warrior++) {
        glPushMatrix();
        
        float wx = -45.0f + warrior * 7.5f + sin(warrior * 1.9f) * 6.0f;
        float wz = -25.0f + warrior * 3.8f + cos(warrior * 1.4f) * 10.0f;
        float angle = warrior * 45.0f + sin(warrior * 2.1f) * 90.0f;
        
        glTranslatef(wx, 0.02f, wz);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        
        // Body (elongated ellipse lying down) - now textured
        glPushMatrix();
        glScalef(0.4f, 0.15f, 1.8f); // Prone figure
        
        // Use textured quad for body instead of sphere for better texture display
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -0.5f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, -0.5f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 0.5f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 0.5f, 1.0f);
        glEnd();
        glPopMatrix();
        
        // Head with skin texture
        bindTexture(TEX_SKIN);
        glColor3f(0.9f, 0.8f, 0.7f); // Slight skin tone tint
        glPushMatrix();
        glTranslatef(0.0f, 0.1f, 0.9f);
        glScalef(0.3f, 0.25f, 0.3f);
        gluSphere(quadric, 1.0f, 8, 6);
        glPopMatrix();
        
        // Arms with armor texture
        bindTexture(TEX_ARMOR);
        glColor3f(0.8f, 0.8f, 0.85f); // Metallic tint
        glPushMatrix();
        glTranslatef(0.6f, 0.05f, 0.3f);
        glRotatef(30.0f + warrior * 10.0f, 0.0f, 0.0f, 1.0f);
        glScalef(0.15f, 0.12f, 0.8f);
        gluSphere(quadric, 1.0f, 6, 4);
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(-0.6f, 0.05f, 0.2f);
        glRotatef(-25.0f - warrior * 8.0f, 0.0f, 0.0f, 1.0f);
        glScalef(0.15f, 0.12f, 0.7f);
        gluSphere(quadric, 1.0f, 6, 4);
        glPopMatrix();
        
        // Legs with sabatons texture
        bindTexture(TEX_SABATONS);
        glColor3f(0.7f, 0.7f, 0.75f); // Boot/armor tint
        glPushMatrix();
        glTranslatef(0.3f, 0.03f, -0.8f);
        glScalef(0.18f, 0.15f, 1.0f);
        gluSphere(quadric, 1.0f, 6, 4);
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(-0.3f, 0.03f, -0.9f);
        glScalef(0.18f, 0.15f, 1.1f);
        gluSphere(quadric, 1.0f, 6, 4);
        glPopMatrix();
        
        glPopMatrix();
    }
    
    // Western/European warriors - with enhanced texturing
    bindTexture(TEX_FALLEN_WARRIOR);
    glColor3f(0.9f, 0.9f, 0.95f); // Slightly different tone for Western warriors
    
    for (int western = 0; western < 10; western++) {
        glPushMatrix();
        
        float wx = -35.0f + western * 8.2f + sin(western * 2.3f) * 7.0f;
        float wz = -30.0f + western * 4.5f + cos(western * 1.7f) * 8.0f;
        float angle = western * 52.0f + cos(western * 1.8f) * 120.0f;
        
        glTranslatef(wx, 0.03f, wz);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        
        // Textured body for Western warriors
        glPushMatrix();
        glScalef(0.45f, 0.18f, 1.7f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -0.5f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, -0.5f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 0.5f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 0.5f, 1.0f);
        glEnd();
        glPopMatrix();
        
        // Helmet/head with helmet texture
        bindTexture(TEX_HELMET);
        glColor3f(0.7f, 0.7f, 0.8f); // Metallic helmet tint
        glPushMatrix();
        glTranslatef(0.0f, 0.15f, 0.85f);
        glScalef(0.32f, 0.28f, 0.32f);
        gluSphere(quadric, 1.0f, 8, 6);
        glPopMatrix();
        
        // Arms with armor texture
        bindTexture(TEX_ARMOR);
        glColor3f(0.8f, 0.8f, 0.85f);
        glPushMatrix();
        glTranslatef(0.65f, 0.08f, 0.4f);
        glRotatef(45.0f + western * 12.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.16f, 0.14f, 0.9f);
        gluSphere(quadric, 1.0f, 6, 4);
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(-0.65f, 0.06f, 0.1f);
        glRotatef(-35.0f - western * 15.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.16f, 0.14f, 0.8f);
        gluSphere(quadric, 1.0f, 6, 4);
        glPopMatrix();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

// Fallen war horses on the battlefield
void BackgroundRenderer::drawWarHorses() {
    // Apply horses texture for realistic war steeds
    bindTexture(TEX_HORSES);
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    
    for (int horse = 0; horse < 6; horse++) {
        glPushMatrix();
        
        float hx = -50.0f + horse * 18.0f + sin(horse * 1.6f) * 12.0f;
        float hz = -35.0f + horse * 8.0f + cos(horse * 2.2f) * 15.0f;
        float angle = horse * 60.0f + sin(horse * 1.9f) * 45.0f;
        
        glTranslatef(hx, 0.1f, hz);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        
        // Horse body - much larger than human, now textured
        glPushMatrix();
        glScalef(1.2f, 0.8f, 3.0f); // Large fallen horse body
        
        // Use textured quadric surface
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 8; i++) {
            float theta = (float)i / 8.0f * 2.0f * 3.14159f;
            float x = cos(theta);
            float z = sin(theta);
            
            glTexCoord2f((float)i / 8.0f, 0.0f);
            glVertex3f(x, -0.5f, z);
            
            glTexCoord2f((float)i / 8.0f, 1.0f);
            glVertex3f(x, 0.5f, z);
        }
        glEnd();
        glPopMatrix();
        
        // Horse head and neck - textured quad
        glPushMatrix();
        glTranslatef(0.0f, 0.3f, 1.8f);
        glRotatef(-20.0f, 1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.3f, -0.25f, -0.6f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(0.3f, -0.25f, -0.6f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(0.3f, 0.25f, 0.6f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.3f, 0.25f, 0.6f);
        glEnd();
        glPopMatrix();
        
        // Legs - fallen at various angles, textured
        for (int leg = 0; leg < 4; leg++) {
            glPushMatrix();
            float legX = (leg % 2 == 0) ? 0.8f : -0.8f;
            float legZ = (leg < 2) ? 1.0f : -1.2f;
            glTranslatef(legX, 0.0f, legZ);
            glRotatef(30.0f + leg * 15.0f + horse * 10.0f, 1.0f, 0.0f, 1.0f);
            
            // Textured leg as rectangular prism
            glBegin(GL_QUADS);
            // Front face
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.1f, 0.0f, 0.75f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(0.1f, 0.0f, 0.75f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(0.1f, 0.0f, -0.75f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.1f, 0.0f, -0.75f);
            glEnd();
            glPopMatrix();
        }
        
        glPopMatrix();
    }
}

void BackgroundRenderer::drawSmokePlumes() {
    // Apply smoke texture for realistic smoke effects
    bindTexture(TEX_SMOKE);
    
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dense black smoke from major fires with texture
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f); // Semi-transparent white to show texture
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        float timeOffset = windTime + i * 1.2f;
        float rise = fmod(timeOffset * 2.5f, 35.0f);
        float sway = sin(timeOffset * 0.6f) * 5.0f + cos(timeOffset * 0.3f) * 2.0f;
        glTranslatef(20.0f + sway, rise + 2.0f, -8.0f);
        float scale = 1.5f + rise * 0.25f + sin(timeOffset) * 0.3f;
        glScalef(scale, scale, scale);
        
        // Use textured sphere with automatic texture coordinates
        gluQuadricTexture(quadric, GL_TRUE);
        gluSphere(quadric, 1.2f, 10, 8);
        glPopMatrix();
    }
    
    // Multiple smoke columns from ruined structures
    glColor4f(0.2f, 0.2f, 0.22f, 0.55f);
    for (int column = 0; column < 3; column++) {
        float baseX = 40.0f + column * 15.0f;
        float baseZ = -45.0f + column * 10.0f;
        
        for (int i = 0; i < 6; i++) {
            glPushMatrix();
            float timeOffset = windTime + i * 1.3f + column * 2.0f;
            float rise = fmod(timeOffset * 1.8f, 42.0f);
            float sway = sin(timeOffset * 0.35f + column) * 4.0f;
            glTranslatef(baseX + sway, 6.0f + rise, baseZ);
            float scale = 2.0f + rise * 0.22f + cos(timeOffset) * 0.25f;
            glScalef(scale, scale, scale);
            gluSphere(quadric, 1.1f, 8, 6);
            glPopMatrix();
        }
    }
    
    // Dust clouds from battlefield - lighter, more dispersed
    glColor4f(0.4f, 0.35f, 0.28f, 0.3f); // Dusty brown
    for (int dust = 0; dust < 10; dust++) {
        glPushMatrix();
        float timeOffset = windTime + dust * 0.8f;
        float drift = fmod(timeOffset * 1.2f, 60.0f) - 30.0f; // Horizontal drift
        float rise = sin(timeOffset * 0.5f) * 2.0f + 1.0f;
        float dustX = -40.0f + dust * 8.0f + drift;
        float dustZ = -20.0f + dust * 4.0f + cos(timeOffset * 0.3f) * 8.0f;
        
        glTranslatef(dustX, rise, dustZ);
        float scale = 3.0f + sin(timeOffset * 0.7f) * 1.0f;
        glScalef(scale, scale * 0.6f, scale);
        gluSphere(quadric, 0.8f, 6, 4);
        glPopMatrix();
    }
    
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void BackgroundRenderer::drawRain() {
    if (currentWeather != WEATHER_RAIN && currentWeather != WEATHER_STORM) return;
    
    // Apply rain texture for realistic rain effects
    bindTexture(TEX_RAIN);
    
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f); // Semi-transparent white to show texture
    float windSlant = 0.5f + sin(windTime) * 0.2f;

    glBegin(GL_QUADS);
    for (int i = 0; i < 250; i++) {
        float x = -60.0f + (rand() % 12000) / 100.0f;
        float z = -60.0f + (rand() % 12000) / 100.0f;
        float y_base = fmod(30.0f - (windTime * 20.0f) + (float)(rand() % 30), 30.0f);
        float width = 0.02f;
        
        // Draw rain drops as textured quads
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x - width, y_base, z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x + width, y_base, z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x + width + windSlant, y_base - 2.0f, z);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(x - width + windSlant, y_base - 2.0f, z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void BackgroundRenderer::drawBirds() {
    // Birds don't fly in bad weather
    if (currentWeather != WEATHER_CLEAR) return;
    
    // Apply bird texture for realistic flying birds
    bindTexture(TEX_BIRD);
    glColor3f(1.0f, 1.0f, 1.0f); // White to show texture clearly
    
    for (int flock = 0; flock < 2; flock++) {
        float baseX = -40.0f + flock * 50.0f + sin(cloudTime + flock) * 15.0f;
        float baseY = 30.0f + cos(cloudTime * 0.7f + flock) * 5.0f;
        float baseZ = -70.0f + flock * 10.0f;
        
        for (int bird = 0; bird < 6; bird++) {
            float x = baseX + (bird - 3) * 2.5f;
            float y = baseY - abs(bird - 3) * 0.8f;
            float wingFlap = sin(windTime * 5.0f + bird) * 0.4f;
            
            glPushMatrix();
            glTranslatef(x, y, baseZ);
            
            // Draw bird as textured quad with wing animation
            glBegin(GL_QUADS);
            // Bird body with flapping wing effect
            float wingSpread = 0.8f + wingFlap * 0.5f;
            float wingHeight = 0.3f + abs(wingFlap) * 0.2f;
            
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-wingSpread, -wingHeight, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(wingSpread, -wingHeight, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(wingSpread * 0.6f, wingHeight, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-wingSpread * 0.6f, wingHeight, 0.0f);
            glEnd();
            
            glPopMatrix();
        }
    }
}

// ===== MAIN RENDER FUNCTION =====
void BackgroundRenderer::render() {
    // War sound no longer auto-starts - user controls via T key
    
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);

    // Set up enhanced lighting based on storm flashes, siege fires, and time of day
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.25f, 1.0f }; // Darker base for dramatic effect
    
    // Lightning flash lighting
    light_ambient[0] += lightningBrightness * 0.8f;
    light_ambient[1] += lightningBrightness * 0.8f;
    light_ambient[2] += lightningBrightness;
    
    // Fire glow lighting from battlefield
    float fireGlow = sin(siegeTime * 4.0f) * 0.1f + 0.15f;
    light_ambient[0] += fireGlow;
    light_ambient[1] += fireGlow * 0.6f; // Orange-red fire glow
    
    // Day/night cycle lighting
    float dayIntensity = 0.5f + cos(dayNightTime * 3.14159f) * 0.3f;
    light_ambient[0] *= dayIntensity;
    light_ambient[1] *= dayIntensity;
    light_ambient[2] *= dayIntensity;
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    
    // Add secondary light source for dramatic battlefield lighting
    GLfloat light1_pos[] = { 50.0f, 40.0f, -30.0f, 1.0f };
    GLfloat light1_diffuse[] = { 0.8f, 0.6f, 0.4f, 1.0f }; // Warm firelight
    GLfloat light1_specular[] = { 1.0f, 0.8f, 0.6f, 1.0f };
    
    // Flicker the secondary light based on fires
    light1_diffuse[0] *= (0.8f + fireGlow);
    light1_diffuse[1] *= (0.6f + fireGlow * 0.8f);
    light1_diffuse[2] *= (0.4f + fireGlow * 0.6f);
    
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    
    // Storm lighting - blue-tinted directional light during storms
    if (currentWeather == WEATHER_STORM) {
        GLfloat storm_light_pos[] = { -100.0f, 80.0f, -50.0f, 0.0f }; // Directional
        GLfloat storm_light_diffuse[] = { 
            0.4f + lightningBrightness, 
            0.5f + lightningBrightness, 
            0.8f + lightningBrightness * 0.5f, 
            1.0f 
        }; // Blue storm light
        
        glEnable(GL_LIGHT2);
        glLightfv(GL_LIGHT2, GL_POSITION, storm_light_pos);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, storm_light_diffuse);
    } else {
        glDisable(GL_LIGHT2);
    }

    // Drawing order optimized for dramatic layering (far to near)
    setupFog();
    
    // 1. Atmospheric elements (farthest)
    drawSkyDome();
    drawSunOrMoon();
    drawStars(); // Draw stars during night time
    
    // 2. Distant mountain ranges (ink-wash style)
    drawMountains();
    drawClouds();
    drawBirds();
    
    // 3. Mid-ground fortifications and structures
    drawChineseFortress();
    drawWesternCastle();
    
    // 4. Siege equipment and effects
    drawSiegeLadders();
    drawSiegeTowers(); // Massive siege towers
    drawTrebuchets(); // Long-range siege engines
    drawBatteringRams(); // Gate-breaking weapons
    drawCatapultStones();
    drawArrowVolleys(); // Massive arrow formations
    
    // 5. Expanded War Scene - Massive Battle Elements
    drawMassiveArmy(); // Multiple army battalions
    drawArcherFormations(); // Archer positions on hills
    drawCavalryCharges(); // Massive cavalry charges from flanks
    drawTrebuchetBattery(); // Battery of trebuchets
    drawBatteringRamAssault(); // Battering ram assault on gates
    drawArrowVolley(); // Dense arrow volleys filling the sky
    drawCampfires(); // Military campfires throughout battlefield
    drawWarDrums(); // War drums for battle rhythm
    drawStarField(); // Enhanced star field for night battles
    drawForestTrees(); // Massive forest with individual trees
    
    // 6. Natural elements
    drawTrees();
    drawBattlefield();
    drawGrass();
    
    // 7. Battle aftermath and active siege effects
    drawWeaponsAndDebris();
    drawFallenWarriors(); // Fallen soldiers across battlefield
    drawWarHorses(); // Fallen war horses
    drawBanners();
    drawFires();
    drawSiegeEffects(); // Active battle effects
    drawSmokePlumes(); // Smoke should be drawn over most elements

    // 7. Weather overlays (drawn last)
    drawRain();
    drawLightning(); // 2D effect, drawn over everything
    
    // 8. Debug visualization (drawn on top)
    drawCollisionBoxes(); // Show collision boundaries if debug mode enabled

    glPopMatrix();
}

void BackgroundRenderer::drawWesternKnightFormations() {
    // Western knight formations and heavy cavalry positions (reduced quantity, raised position)
    glColor3f(0.4f, 0.4f, 0.45f); // Steel armor
    
    // Knight formation near Western castle (reduced to minimal 2x2, raised position)
    glPushMatrix();
    glTranslatef(40.0f, 2.5f, -20.0f);  // Raised Y position from 0.0f to 2.5f
    
    // Create formation of knights with shields and weapons
    for (int row = 0; row < 2; row++) {        // Reduced from 3 to 2 rows
        for (int col = 0; col < 2; col++) {    // Reduced from 3 to 2 columns
            glPushMatrix();
            float kx = col * 4.0f - 2.0f;      // Increased spacing for better visibility
            float kz = row * 4.0f;             // Increased spacing
            glTranslatef(kx, 0.0f, kz);
            
            // Knight's shield - heraldic design
            glColor3f(0.1f, 0.1f, 0.6f); // Blue heraldic background
            glBegin(GL_QUADS);
            glVertex3f(-0.8f, 0.5f, 0.0f);  glVertex3f(0.8f, 0.5f, 0.0f);
            glVertex3f(0.8f, 2.5f, 0.0f);   glVertex3f(-0.8f, 2.5f, 0.0f);
            glEnd();
            
            // Shield boss (center)
            glColor3f(0.6f, 0.6f, 0.7f);
            glPushMatrix();
            glTranslatef(0.0f, 1.5f, 0.1f);
            gluSphere(quadric, 0.3f, 8, 6);
            glPopMatrix();
            
            // Heraldic cross
            glColor3f(0.9f, 0.9f, 0.1f); // Gold cross
            glLineWidth(6.0f);
            glBegin(GL_LINES);
            glVertex3f(0.0f, 0.8f, 0.1f); glVertex3f(0.0f, 2.2f, 0.1f);
            glVertex3f(-0.5f, 1.5f, 0.1f); glVertex3f(0.5f, 1.5f, 0.1f);
            glEnd();
            
            // Lance/spear
            glColor3f(0.4f, 0.3f, 0.2f);
            glLineWidth(4.0f);
            glBegin(GL_LINES);
            glVertex3f(0.5f, 0.0f, 0.0f);
            glVertex3f(0.5f, 4.0f, 0.0f);
            glEnd();
            
            // Lance head
            glColor3f(0.5f, 0.5f, 0.55f);
            glPushMatrix();
            glTranslatef(0.5f, 4.0f, 0.0f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0f, 0.8f, 0.0f);
            glVertex3f(-0.1f, 0.0f, 0.0f);
            glVertex3f(0.1f, 0.0f, 0.0f);
            glEnd();
            glPopMatrix();
            
            glPopMatrix();
        }
    }
    glPopMatrix();
    
    // Heavy cavalry unit
    glPushMatrix();
    glTranslatef(55.0f, 0.0f, -10.0f);
    
    for (int horse = 0; horse < 6; horse++) {
        glPushMatrix();
        float hx = (horse % 3) * 3.0f;
        float hz = (horse / 3) * 4.0f;
        glTranslatef(hx, 0.0f, hz);
        
        // Horse barding (armor)
        glColor3f(0.3f, 0.3f, 0.35f);
        glBegin(GL_QUADS);
        glVertex3f(-1.0f, 0.8f, -1.5f); glVertex3f(1.0f, 0.8f, -1.5f);
        glVertex3f(1.0f, 1.8f, 1.5f);   glVertex3f(-1.0f, 1.8f, 1.5f);
        glEnd();
        
        // Knight's banner on lance
        glColor3f(0.6f, 0.1f, 0.1f);
        glBegin(GL_TRIANGLES);
        glVertex3f(1.5f, 3.5f, 0.0f);
        glVertex3f(2.5f, 3.0f, 0.0f);
        glVertex3f(1.5f, 2.5f, 0.0f);
        glEnd();
        
        glPopMatrix();
    }
    glPopMatrix();
    
    glLineWidth(1.0f);
}

void BackgroundRenderer::drawWesternCrossbowSquads() {
    // Western crossbow battalions in defensive positions
    glColor3f(0.3f, 0.25f, 0.2f); // Wood and steel
    
    // Main crossbow squadron
    glPushMatrix();
    glTranslatef(35.0f, 0.0f, 5.0f);
    
    // Pavise shields (large standing shields for crossbowmen)
    for (int pavise = 0; pavise < 6; pavise++) {
        glPushMatrix();
        float px = pavise * 2.0f - 5.0f;
        glTranslatef(px, 0.0f, 0.0f);
        
        // Large defensive shield
        glColor3f(0.4f, 0.3f, 0.2f);
        glBegin(GL_QUADS);
        glVertex3f(-0.8f, 0.0f, -0.1f); glVertex3f(0.8f, 0.0f, -0.1f);
        glVertex3f(0.8f, 3.0f, -0.1f);  glVertex3f(-0.8f, 3.0f, -0.1f);
        glEnd();
        
        // Shield reinforcements
        glColor3f(0.2f, 0.2f, 0.25f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        for (int band = 0; band < 4; band++) {
            float by = 0.5f + band * 0.6f;
            glVertex3f(-0.7f, by, -0.05f);
            glVertex3f(0.7f, by, -0.05f);
        }
        glEnd();
        
        glPopMatrix();
    }
    
    // Crossbows positioned behind shields
    for (int xbow = 0; xbow < 8; xbow++) {
        glPushMatrix();
        float cx = (xbow % 4) * 2.0f - 3.0f;
        float cz = (xbow / 4) * 1.5f + 1.0f;
        glTranslatef(cx, 1.2f, cz);
        glRotatef(-15.0f, 0.0f, 0.0f, 1.0f); // Aimed upward
        
        // Crossbow stock
        glColor3f(0.4f, 0.3f, 0.2f);
        glBegin(GL_QUADS);
        glVertex3f(-0.05f, -0.05f, 0.0f); glVertex3f(0.05f, -0.05f, 0.0f);
        glVertex3f(0.05f, 0.05f, 1.5f);   glVertex3f(-0.05f, 0.05f, 1.5f);
        glEnd();
        
        // Crossbow prod (bow part)
        glColor3f(0.3f, 0.3f, 0.35f);
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glVertex3f(-0.8f, 0.0f, 1.2f);
        glVertex3f(0.8f, 0.0f, 1.2f);
        glEnd();
        
        // Bolt (if loaded)
        if ((xbow + (int)siegeTime) % 3 == 0) {
            glColor3f(0.4f, 0.3f, 0.1f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(0.0f, 0.0f, 0.8f);
            glVertex3f(0.0f, 0.0f, 1.4f);
            glEnd();
        }
        
        glPopMatrix();
    }
    glPopMatrix();
    
    glLineWidth(1.0f);
}

void BackgroundRenderer::drawWesternWarHorns() {
    // Western war horns and musical instruments
    glColor3f(0.6f, 0.5f, 0.3f); // Brass/bronze
    
    // Main war horn position
    glPushMatrix();
    glTranslatef(50.0f, 2.0f, 10.0f);
    
    // Large war horn
    glPushMatrix();
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(15.0f, 0.0f, 0.0f, 1.0f);
    
    // Horn tube - curved
    for (int segment = 0; segment < 8; segment++) {
        glPushMatrix();
        float t = segment / 7.0f;
        float curve = sin(t * M_PI * 0.5f) * 2.0f;
        glTranslatef(t * 3.0f, curve, 0.0f);
        glRotatef(t * 30.0f, 0.0f, 0.0f, 1.0f);
        
        float radius = 0.3f + t * 0.4f; // Expanding horn
        gluCylinder(quadric, radius, radius + 0.1f, 0.4f, 8, 1);
        glPopMatrix();
    }
    glPopMatrix();
    
    // Horn player's platform
    glColor3f(0.3f, 0.25f, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(-1.0f, -2.0f, -1.0f); glVertex3f(1.0f, -2.0f, -1.0f);
    glVertex3f(1.0f, 0.0f, 1.0f);    glVertex3f(-1.0f, 0.0f, 1.0f);
    glEnd();
    glPopMatrix();
    
    // Battle trumpets around Western positions
    for (int trumpet = 0; trumpet < 4; trumpet++) {
        glPushMatrix();
        float tx = 38.0f + trumpet * 8.0f;
        float tz = -3.0f + trumpet * 2.0f;
        glTranslatef(tx, 1.5f, tz);
        glColor3f(0.7f, 0.6f, 0.2f); // Bright brass
        
        // Trumpet tube
        glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadric, 0.15f, 0.25f, 1.5f, 8, 2);
        
        // Trumpet bell
        glTranslatef(0.0f, 0.0f, 1.5f);
        gluCylinder(quadric, 0.25f, 0.5f, 0.3f, 8, 2);
        
        glPopMatrix();
    }
    glPopMatrix();
    
    glLineWidth(1.0f);
}

void BackgroundRenderer::setWeather(int weather) {
    if (weather >= 0 && weather <= WEATHER_STORM) {
        currentWeather = weather;
        if (weather != WEATHER_STORM) {
            lightningBrightness = 0.0f;
            lightningTimer = 5.0f; // Reset timer
        }
    }
}

// Draw massive army formations spanning the entire battlefield (reduced quantities, raised position)
void BackgroundRenderer::drawMassiveArmy() {
    bindTexture(TEX_KNIGHT_FORMATIONS);
    glEnable(GL_TEXTURE_2D);
    
    // Draw multiple army battalions across the battlefield (reduced to minimal 2x3)
    for (int battalion = 0; battalion < 2; battalion++) {      // Reduced from 4 to 2 battalions
        for (int formation = 0; formation < 3; formation++) {  // Reduced from 6 to 3 formations
            glPushMatrix();
            
            float x = -60.0f + battalion * 120.0f;             // Wider spacing for better visibility
            float z = -60.0f + formation * 60.0f;              // Wider spacing
            
            glTranslatef(x, -2.0f, z);  // Raised Y position from -15.0f to -2.0f
            glScalef(8.0f, 5.0f, 8.0f);
            
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 2.0f, 1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 2.0f, 1.0f);
            glEnd();
            
            glPopMatrix();
        }
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw archer formations on elevated positions (increased quantities)
void BackgroundRenderer::drawArcherFormations() {
    bindTexture(TEX_ARCHER_FORMATIONS);
    glEnable(GL_TEXTURE_2D);
    
    // Archer positions on hills and battlements (reduced to 4 positions, raised above ground)
    float positions[][3] = {
        {-80.0f, 3.0f, -40.0f}, {80.0f, 6.0f, -52.0f},
        {-90.0f, 8.0f, 20.0f}, {90.0f, 7.0f, 25.0f}
    };
    
    for (int i = 0; i < 4; i++) {  // Reduced from 8 to 4
        glPushMatrix();
        glTranslatef(positions[i][0], positions[i][1], positions[i][2]);
        glScalef(6.0f, 4.0f, 6.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.5f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.5f, 1.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw massive cavalry charges from both flanks
void BackgroundRenderer::drawCavalryCharges() {
    bindTexture(TEX_HORSE_CAVALRY);
    glEnable(GL_TEXTURE_2D);
    
    // Left flank cavalry charge
    for (int wave = 0; wave < 5; wave++) {
        for (int rider = 0; rider < 15; rider++) {
            glPushMatrix();
            
            float x = -150.0f + wave * 8.0f;
            float z = -60.0f + rider * 8.0f;
            float charge_offset = sin(siegeTime * 2.0f + wave * 0.5f) * 5.0f;
            
            glTranslatef(x + charge_offset, -12.0f, z);
            glScalef(4.0f, 3.0f, 6.0f);
            
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 2.0f, 1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 2.0f, 1.0f);
            glEnd();
            
            glPopMatrix();
        }
    }
    
    // Right flank cavalry charge
    for (int wave = 0; wave < 5; wave++) {
        for (int rider = 0; rider < 15; rider++) {
            glPushMatrix();
            
            float x = 150.0f - wave * 8.0f;
            float z = -60.0f + rider * 8.0f;
            float charge_offset = sin(siegeTime * 2.0f + wave * 0.5f + 3.14f) * 5.0f;
            
            glTranslatef(x + charge_offset, -12.0f, z);
            glScalef(4.0f, 3.0f, 6.0f);
            
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 2.0f, 1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 2.0f, 1.0f);
            glEnd();
            
            glPopMatrix();
        }
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw battery of trebuchets for massive siege warfare
void BackgroundRenderer::drawTrebuchetBattery() {
    bindTexture(TEX_TREBUCHET);
    glEnable(GL_TEXTURE_2D);
    
    // Multiple trebuchet positions
    float trebuchet_positions[][3] = {
        {-100.0f, -10.0f, -70.0f}, {-80.0f, -8.0f, -75.0f}, {-60.0f, -12.0f, -68.0f},
        {60.0f, -11.0f, -72.0f}, {80.0f, -9.0f, -66.0f}, {100.0f, -13.0f, -74.0f},
        {-40.0f, -7.0f, -80.0f}, {40.0f, -14.0f, -78.0f}
    };
    
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        glTranslatef(trebuchet_positions[i][0], trebuchet_positions[i][1], trebuchet_positions[i][2]);
        
        // Animate trebuchet arm
        float arm_angle = sin(siegeTime * 1.5f + i * 0.8f) * 30.0f - 45.0f;
        glRotatef(arm_angle, 1.0f, 0.0f, 0.0f);
        glScalef(8.0f, 12.0f, 8.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 3.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 3.0f, 1.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw military campfires throughout the battlefield
// Draw war drums for battle rhythm
void BackgroundRenderer::drawWarDrums() {
    bindTexture(TEX_DRUMS);
    glEnable(GL_TEXTURE_2D);
    
    // Drum positions around army formations
    float drum_positions[][3] = {
        {-110.0f, -12.0f, -50.0f}, {-90.0f, -10.0f, -55.0f}, {-70.0f, -14.0f, -48.0f},
        {70.0f, -13.0f, -52.0f}, {90.0f, -11.0f, -47.0f}, {110.0f, -15.0f, -54.0f},
        {0.0f, -8.0f, -65.0f}, {-20.0f, -12.0f, -62.0f}, {20.0f, -10.0f, -68.0f}
    };
    
    for (int i = 0; i < 9; i++) {
        glPushMatrix();
        glTranslatef(drum_positions[i][0], drum_positions[i][1], drum_positions[i][2]);
        
        // Animate drumbeat
        float beat = abs(sin(siegeTime * 4.0f + i * 1.2f));
        glScalef(3.0f, 2.0f + beat * 0.5f, 3.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.5f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.5f, 1.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw fallen warriors across the battlefield
// Draw battering ram assault on castle gates
void BackgroundRenderer::drawBatteringRamAssault() {
    bindTexture(TEX_BATTERING_RAMS);
    glEnable(GL_TEXTURE_2D);
    
    // Multiple battering rams approaching castle
    float ram_positions[][3] = {
        {-15.0f, -13.0f, 45.0f}, {0.0f, -12.0f, 48.0f}, {15.0f, -14.0f, 42.0f},
        {-8.0f, -13.0f, 52.0f}, {8.0f, -11.0f, 50.0f}
    };
    
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        
        float assault_motion = sin(siegeTime * 3.0f + i * 1.5f) * 2.0f;
        glTranslatef(ram_positions[i][0], ram_positions[i][1], ram_positions[i][2] + assault_motion);
        glScalef(8.0f, 4.0f, 12.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.5f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.5f, 1.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// Draw massive arrow volleys filling the sky
void BackgroundRenderer::drawArrowVolley() {
    bindTexture(TEX_ARROWS);
    glEnable(GL_TEXTURE_2D);
    
    // Dense arrow volleys across the battlefield
    for (int volley = 0; volley < 150; volley++) {
        glPushMatrix();
        
        float x = -160.0f + (volley % 20) * 16.0f + (rand() % 10) - 5.0f;
        float y = 10.0f + (volley / 20) * 8.0f + sin(siegeTime * 2.0f + volley * 0.3f) * 5.0f;
        float z = -70.0f + (volley / 40) * 25.0f + (rand() % 8) - 4.0f;
        
        glTranslatef(x, y, z);
        glRotatef(sin(siegeTime + volley * 0.2f) * 15.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.3f, 4.0f, 0.3f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 2.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 2.0f, 1.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// ===== COLLISION DETECTION SYSTEM =====
void BackgroundRenderer::initializeCollisionBoxes() {
    collisionBoxes.clear();
    
    // Chinese Fortress collision (at -45, 0, -35 with scale 1.5)
    // Original fortress size: -15 to +15 (X), -8 to +8 (Z), scaled by 1.5
    collisionBoxes.push_back({-67.5f, -22.5f, -47.0f, -23.0f, 18.0f, "Chinese Fortress"});
    
    // Western Castle collision (at 50, 0, -45)
    // Castle size: -8 to +8 (X), -8 to +8 (Z) plus cylindrical keep
    collisionBoxes.push_back({42.0f, 58.0f, -53.0f, -37.0f, 20.0f, "Western Castle"});
    
    // Major siege equipment collision boxes (moved away from center)
    // Chinese siege tower (at -25, 0, -15)
    collisionBoxes.push_back({-31.0f, -19.0f, -21.0f, -9.0f, 24.0f, "Chinese Siege Tower"});
    
    // Western siege tower (at 35, 0, -20)
    collisionBoxes.push_back({31.0f, 39.0f, -24.0f, -16.0f, 18.0f, "Western Siege Tower"});
    
    // Trebuchets (far from center)
    collisionBoxes.push_back({-56.0f, -44.0f, 19.0f, 31.0f, 15.0f, "Chinese Trebuchet"});
    collisionBoxes.push_back({44.0f, 56.0f, 19.0f, 31.0f, 15.0f, "Western Trebuchet"});
    
    // Battering rams (near gates, away from center)
    collisionBoxes.push_back({-50.0f, -40.0f, -32.0f, -28.0f, 3.0f, "Chinese Gate Ram"});
    collisionBoxes.push_back({45.0f, 55.0f, -48.0f, -42.0f, 3.0f, "Western Gate Ram"});
    
    // Large campfires and weapon racks (moved away from center)
    collisionBoxes.push_back({17.0f, 23.0f, -12.0f, -2.0f, 2.5f, "Major Campfire"});
    collisionBoxes.push_back({-53.0f, -47.0f, 23.0f, 27.0f, 3.0f, "Chinese Weapon Rack"});
    collisionBoxes.push_back({47.0f, 53.0f, 23.0f, 27.0f, 3.0f, "Western Weapon Rack"});
    
    // Battlefield debris and obstacles (moved away from starting area)
    collisionBoxes.push_back({-35.0f, -25.0f, 8.0f, 18.0f, 2.0f, "Battlefield Debris 1"});
    collisionBoxes.push_back({25.0f, 35.0f, -25.0f, -15.0f, 2.0f, "Battlefield Debris 2"});
    collisionBoxes.push_back({-45.0f, -35.0f, 5.0f, 12.0f, 1.8f, "Supply Wagon"});
    collisionBoxes.push_back({30.0f, 37.0f, 8.0f, 15.0f, 2.2f, "Overturned Cart"});
    collisionBoxes.push_back({-18.0f, -12.0f, -8.0f, -2.0f, 1.0f, "Boulder"});
    collisionBoxes.push_back({40.0f, 47.0f, -10.0f, -3.0f, 1.5f, "Broken Catapult"});
    collisionBoxes.push_back({-25.0f, -18.0f, 25.0f, 32.0f, 2.0f, "Abandoned Tent"});
    
    printf("Initialized %zu collision boxes\n", collisionBoxes.size());
    printf("Center area (±15, ±15) kept clear for character movement\n");
}

bool BackgroundRenderer::checkCollision(float x, float z, float radius) {
    // First check battlefield boundaries (prevent falling off the edge)
    const float battlefieldSize = 90.0f; // Battlefield extends roughly from -90 to +90
    if (x < -battlefieldSize || x > battlefieldSize || z < -80.0f || z > 60.0f) {
        return true; // Blocked at battlefield edge
    }
    
    // Check collision with all objects
    return checkCollisionWithCastles(x, z, radius) || checkCollisionWithObjects(x, z, radius);
}

bool BackgroundRenderer::checkCollisionWithCastles(float x, float z, float radius) {
    // Check major fortifications
    for (const auto& box : collisionBoxes) {
        if (strstr(box.name, "Fortress") || strstr(box.name, "Castle")) {
            if (circleBoxCollision(x, z, radius, box)) {
                return true;
            }
        }
    }
    return false;
}

bool BackgroundRenderer::checkCollisionWithObjects(float x, float z, float radius) {
    // Check all other objects (siege equipment, debris, etc.)
    for (const auto& box : collisionBoxes) {
        if (!strstr(box.name, "Fortress") && !strstr(box.name, "Castle")) {
            if (circleBoxCollision(x, z, radius, box)) {
                return true;
            }
        }
    }
    return false;
}

bool BackgroundRenderer::pointInBox(float x, float z, const CollisionBox& box) {
    return (x >= box.minX && x <= box.maxX && z >= box.minZ && z <= box.maxZ);
}

bool BackgroundRenderer::circleBoxCollision(float cx, float cz, float radius, const CollisionBox& box) {
    // Find the closest point on the box to the circle center
    float closestX = (box.minX > cx) ? box.minX : ((cx > box.maxX) ? box.maxX : cx);
    float closestZ = (box.minZ > cz) ? box.minZ : ((cz > box.maxZ) ? box.maxZ : cz);
    
    // Calculate distance from circle center to closest point
    float distanceX = cx - closestX;
    float distanceZ = cz - closestZ;
    float distanceSquared = distanceX * distanceX + distanceZ * distanceZ;
    
    // Collision occurs if distance is less than radius
    return distanceSquared < (radius * radius);
}

void BackgroundRenderer::drawCollisionBoxes() {
    if (!collisionDebugMode) return;
    
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw collision boxes as wireframe
    glColor4f(1.0f, 0.0f, 0.0f, 0.5f); // Semi-transparent red
    glLineWidth(2.0f);
    
    for (const auto& box : collisionBoxes) {
        // Draw wireframe box
        glBegin(GL_LINE_LOOP);
        // Bottom face
        glVertex3f(box.minX, 0.0f, box.minZ);
        glVertex3f(box.maxX, 0.0f, box.minZ);
        glVertex3f(box.maxX, 0.0f, box.maxZ);
        glVertex3f(box.minX, 0.0f, box.maxZ);
        glEnd();
        
        glBegin(GL_LINE_LOOP);
        // Top face
        glVertex3f(box.minX, box.height, box.minZ);
        glVertex3f(box.maxX, box.height, box.minZ);
        glVertex3f(box.maxX, box.height, box.maxZ);
        glVertex3f(box.minX, box.height, box.maxZ);
        glEnd();
        
        // Vertical lines
        glBegin(GL_LINES);
        glVertex3f(box.minX, 0.0f, box.minZ); glVertex3f(box.minX, box.height, box.minZ);
        glVertex3f(box.maxX, 0.0f, box.minZ); glVertex3f(box.maxX, box.height, box.minZ);
        glVertex3f(box.maxX, 0.0f, box.maxZ); glVertex3f(box.maxX, box.height, box.maxZ);
        glVertex3f(box.minX, 0.0f, box.maxZ); glVertex3f(box.minX, box.height, box.maxZ);
        glEnd();
    }
    
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glLineWidth(1.0f);
}

// ===== AUDIO SYSTEM =====
void BackgroundRenderer::playWarSound() {
    if (!warSoundInitialized) {
        printf("Warning: Audio system not initialized\n");
        return;
    }
    
    if (warSoundPlaying) {
        return; // Already playing
    }
    
    // Use MCI to play MP3 file
    char command[512];
    sprintf_s(command, sizeof(command), "open \"texturess/war sound.mp3\" type mpegvideo alias warsound");
    MCIERROR mciError = mciSendStringA(command, NULL, 0, NULL);
    
    if (mciError != 0) {
        printf("Error opening war sound: %lu\n", mciError);
        return;
    }
    
    // Set volume (0 to 1000, where 1000 is maximum)
    int volume = (int)(warSoundVolume * 1000.0f);
    sprintf_s(command, sizeof(command), "setaudio warsound volume to %d", volume);
    mciSendStringA(command, NULL, 0, NULL);
    
    // Play the sound in a loop
    sprintf_s(command, sizeof(command), "play warsound repeat");
    mciError = mciSendStringA(command, NULL, 0, NULL);
    
    if (mciError != 0) {
        printf("Error playing war sound: %lu\n", mciError);
        mciSendStringA("close warsound", NULL, 0, NULL);
        return;
    }
    
    warSoundPlaying = true;
    printf("War sound started playing\n");
}

void BackgroundRenderer::stopWarSound() {
    if (!warSoundPlaying) {
        return; // Not playing
    }
    
    // Stop and close the sound
    mciSendStringA("stop warsound", NULL, 0, NULL);
    mciSendStringA("close warsound", NULL, 0, NULL);
    
    warSoundPlaying = false;
    printf("War sound stopped\n");
}

void BackgroundRenderer::setWarSoundVolume(float volume) {
    // Clamp volume between 0.0 and 1.0
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    warSoundVolume = volume;
    
    // If currently playing, update the volume
    if (warSoundPlaying) {
        char command[256];
        int mciVolume = (int)(volume * 1000.0f);
        sprintf_s(command, sizeof(command), "setaudio warsound volume to %d", mciVolume);
        mciSendStringA(command, NULL, 0, NULL);
    }
    
    printf("War sound volume set to %.1f%%\n", volume * 100.0f);
}

// Draw enhanced star field for night battles
void BackgroundRenderer::drawStarField() {
    if (dayNightTime > 0.7f) {  // Only at night
        bindTexture(TEX_STARS);
        glEnable(GL_TEXTURE_2D);
        
        // Dense star field
        for (int star = 0; star < 200; star++) {
            glPushMatrix();
            
            float x = -200.0f + (star % 25) * 16.0f;
            float y = 50.0f + (star / 25) * 15.0f + sin(siegeTime * 0.5f + star * 0.1f) * 2.0f;
            float z = -150.0f + (star / 50) * 30.0f;
            
            glTranslatef(x, y, z);
            
            float twinkle = 0.5f + abs(sin(siegeTime * 3.0f + star * 0.8f)) * 0.5f;
            glScalef(twinkle, twinkle, twinkle);
            
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.0f, -0.5f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, 0.0f, -0.5f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f, 1.0f, 0.5f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 1.0f, 0.5f);
            glEnd();
            
            glPopMatrix();
        }
        
        glDisable(GL_TEXTURE_2D);
    }
}

// Draw massive forest with individual trees
void BackgroundRenderer::drawForestTrees() {
    bindTexture(TEX_TREES);
    glEnable(GL_TEXTURE_2D);
    
    // Dense forest on both sides of battlefield
    for (int tree = 0; tree < 80; tree++) {
        glPushMatrix();
        
        float x = (tree < 40) ? -180.0f + (tree % 20) * 8.0f : 140.0f + (tree % 20) * 8.0f;
        float z = -100.0f + (tree / 20) * 30.0f;
        float height = 15.0f + (rand() % 10);
        
        glTranslatef(x, -15.0f + height * 0.3f, z);
        
        // Slight wind sway
        float sway = sin(windTime + tree * 0.5f) * 2.0f;
        glRotatef(sway, 0.0f, 0.0f, 1.0f);
        glScalef(6.0f, height, 6.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 2.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 2.0f, 1.0f);
        glEnd();
        
        glPopMatrix();
    }
    
    glDisable(GL_TEXTURE_2D);
}