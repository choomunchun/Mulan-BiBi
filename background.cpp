#include "background.h"
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Initialize static variables
GLUquadric* BackgroundRenderer::quadric = nullptr;
float BackgroundRenderer::windTime = 0.0f;
float BackgroundRenderer::cloudTime = 0.0f;
int BackgroundRenderer::currentWeather = WEATHER_CLEAR;
float BackgroundRenderer::lightningTimer = 0.0f;
float BackgroundRenderer::lightningBrightness = 0.0f;
float BackgroundRenderer::siegeTime = 0.0f;
std::vector<Spark> BackgroundRenderer::sparks;
std::vector<Arrow> BackgroundRenderer::arrows;
std::vector<Ember> BackgroundRenderer::embers;


void BackgroundRenderer::init() {
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
}

void BackgroundRenderer::cleanup() {
    if (quadric) {
        gluDeleteQuadric(quadric);
        quadric = nullptr;
    }
}

void BackgroundRenderer::update(float deltaTime) {
    windTime += deltaTime * 0.8f;
    cloudTime += deltaTime * 0.15f;
    siegeTime += deltaTime;

    // Update lightning for storms
    if (currentWeather == WEATHER_STORM) {
        lightningTimer -= deltaTime;
        if (lightningTimer <= 0.0f) {
            lightningBrightness = 0.5f + (rand() % 50) / 100.0f; // Random brightness
            lightningTimer = 2.0f + (rand() % 60) / 10.0f;      // Next strike in 2-8 seconds
        }
        // Fade out the lightning flash
        lightningBrightness -= deltaTime * 4.0f;
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
// ===== SKY & ATMOSPHERE =====
void BackgroundRenderer::drawSkyDome() {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);

    // Dramatic dusk sky with ink-wash painting style
    glBegin(GL_TRIANGLE_FAN);
    // Zenith - deep purple-blue like ink wash
    glColor3f(0.15f, 0.1f, 0.3f);
    glVertex3f(0.0f, 80.0f, -50.0f);
    
    // Horizon ring with warm dusk colors
    for (int i = 0; i <= 360; i += 15) {
        float angle = i * M_PI / 180.0;
        float horizonR, horizonG, horizonB;
        
        // Varying horizon colors for dramatic effect
        float variation = sin(angle * 3.0f) * 0.2f;
        switch (currentWeather) {
            case WEATHER_STORM:
                horizonR = 0.3f + variation; horizonG = 0.2f; horizonB = 0.1f;
                break;
            case WEATHER_CLEAR:
            default:
                horizonR = 0.9f + variation; horizonG = 0.4f + variation; horizonB = 0.1f;
                break;
        }
        
        glColor3f(horizonR, horizonG, horizonB);
        glVertex3f(cos(angle) * 200.0f, -10.0f, sin(angle) * 200.0f - 50.0f);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void BackgroundRenderer::drawSunOrMoon() {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    
    // If stormy, no sun/moon visible
    if (currentWeather == WEATHER_STORM || currentWeather == WEATHER_RAIN) {
        glPopMatrix();
        return;
    }

    glTranslatef(-60.0f, 30.0f, -80.0f);
    
    // Setting sun for clear weather, pale moon for fog
    if (currentWeather == WEATHER_FOG) {
        glColor3f(0.8f, 0.8f, 0.9f); // Pale moon
    } else {
        glColor3f(1.0f, 0.7f, 0.3f); // Setting sun
    }

    gluSphere(quadric, 4.0f, 24, 18);

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void BackgroundRenderer::drawClouds() {
    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING); // Clouds are self-illuminated for simplicity
    
    glTranslatef(0.0f, 25.0f, -60.0f);

    float r, g, b;
    if (currentWeather == WEATHER_RAIN || currentWeather == WEATHER_STORM) {
        r = 0.3f; g = 0.3f; b = 0.35f; // Dark storm clouds
    } else {
        r = 0.8f; g = 0.8f; b = 0.9f; // Normal clouds
    }
    glColor4f(r, g, b, 0.75f);
    
    for (int i = 0; i < 10; i++) {
        glPushMatrix();
        float x = -80.0f + i * 18.0f + sin(cloudTime + i) * 8.0f;
        float y = sin(cloudTime * 0.5f + i * 0.8f) * 4.0f;
        float z = cos(cloudTime * 0.3f + i) * 15.0f;
        glTranslatef(fmod(x + cloudTime * 20.0f, 200.0f) - 100.0f, y, z); // Horizontal drift
        
        // Multiple spheres for cloud volume
        gluSphere(quadric, 6.0f, 12, 8);
        glTranslatef(4.0f, 1.5f, 0.0f);
        gluSphere(quadric, 4.0f, 12, 8);
        glTranslatef(-8.0f, 1.0f, 1.5f);
        gluSphere(quadric, 5.0f, 12, 8);
        glPopMatrix();
    }
    
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

    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up a 2D projection to draw a screen flash
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw a white quad that covers the screen with variable transparency
    glColor4f(0.9f, 0.9f, 1.0f, lightningBrightness);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(1, 0);
    glVertex2f(1, 1);
    glVertex2f(0, 1);
    glEnd();

    // Restore original projection
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
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
    
    // Mid-range mountains with more detail
    glTranslatef(0.0f, 0.0f, 60.0f);
    glColor3f(0.25f, 0.25f, 0.35f);
    
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 20; i++) {
        float x = -100.0f + i * 10.0f;
        float height = 20.0f + sin(i * 0.8f) * 12.0f + cos(i * 1.3f) * 8.0f;
        glVertex3f(x, 0.0f, 0.0f);
        glVertex3f(x, height, -10.0f + sin(i) * 3.0f);
    }
    glEnd();
    
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
    // Uneven ground terrain
    glColor3f(0.3f, 0.25f, 0.15f); // Dark muddy brown
    for (int j = -50; j < 50; j++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = -50; i <= 50; i++) {
            float x1 = i * 2.0f;
            float z1 = j * 2.0f;
            float y1 = sin(x1 * 0.1f) * cos(z1 * 0.1f) * 0.3f; // Gentle undulations
            
            float x2 = i * 2.0f;
            float z2 = (j + 1) * 2.0f;
            float y2 = sin(x2 * 0.1f) * cos(z2 * 0.1f) * 0.3f;

            glNormal3f(0.0f, 1.0f, 0.0f); // Simple normal
            glVertex3f(x1, y1, z1);
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }

    // Craters using TRIANGLE_FAN
    glColor3f(0.15f, 0.1f, 0.05f); // Darker soil
    // Crater 1
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-15.0f, -0.5f, 10.0f); // Center
    for (int i = 0; i <= 16; i++) {
        float angle = i * 2.0f * M_PI / 16.0f;
        float radius = 5.0f + sin(angle * 3.0f) * 1.0f;
        glVertex3f(radius * cos(angle) - 15.0f, 0.1f, radius * sin(angle) + 10.0f);
    }
    glEnd();

    // Scorch marks
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.1f, 0.1f, 0.1f, 0.6f);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glVertex3f(18.0f, 0.1f, -10.0f); glVertex3f(24.0f, 0.1f, -11.0f);
    glVertex3f(23.0f, 0.1f, -5.0f);  glVertex3f(17.0f, 0.1f, -4.0f);
    glEnd();
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

// ===== STRUCTURE RENDERING =====
void BackgroundRenderer::drawChineseFortress() {
    glPushMatrix();
    glTranslatef(-45.0f, 0.0f, -35.0f);
    glScalef(1.5f, 1.5f, 1.5f);
    
    // Main fortress walls using GL_QUADS
    glColor3f(0.4f, 0.25f, 0.2f); // Dark reddish-brown traditional color
    glBegin(GL_QUADS);
    
    // Front wall with traditional Chinese architecture
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-15.0f, 0.0f, 8.0f); glVertex3f(15.0f, 0.0f, 8.0f);
    glVertex3f(15.0f, 12.0f, 8.0f); glVertex3f(-15.0f, 12.0f, 8.0f);
    
    // Side walls
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-15.0f, 0.0f, -8.0f); glVertex3f(-15.0f, 0.0f, 8.0f);
    glVertex3f(-15.0f, 12.0f, 8.0f); glVertex3f(-15.0f, 12.0f, -8.0f);
    
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(15.0f, 0.0f, 8.0f); glVertex3f(15.0f, 0.0f, -8.0f);
    glVertex3f(15.0f, 12.0f, -8.0f); glVertex3f(15.0f, 12.0f, 8.0f);
    
    // Back wall
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(15.0f, 0.0f, -8.0f); glVertex3f(-15.0f, 0.0f, -8.0f);
    glVertex3f(-15.0f, 12.0f, -8.0f); glVertex3f(15.0f, 12.0f, -8.0f);
    glEnd();
    
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
    // Large siege stones in flight using gluSphere for realism
    glColor3f(0.3f, 0.3f, 0.35f);
    
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
}

void BackgroundRenderer::drawSiegeLadders() {
    // Siege ladders against the walls using GL_QUADS
    glColor3f(0.4f, 0.3f, 0.2f); // Dark wood
    
    // Ladder against Chinese fortress
    glPushMatrix();
    glTranslatef(-42.0f, 0.0f, -27.0f);
    glRotatef(-75.0f, 1.0f, 0.0f, 0.0f); // Leaning against wall
    
    glBegin(GL_QUADS);
    // Left rail
    glVertex3f(-1.0f, 0.0f, 0.0f); glVertex3f(-0.8f, 0.0f, 0.0f);
    glVertex3f(-0.8f, 20.0f, 0.0f); glVertex3f(-1.0f, 20.0f, 0.0f);
    
    // Right rail
    glVertex3f(0.8f, 0.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 20.0f, 0.0f); glVertex3f(0.8f, 20.0f, 0.0f);
    
    // Rungs
    for (int i = 1; i < 15; i++) {
        float y = i * 1.3f;
        glVertex3f(-1.0f, y, 0.0f); glVertex3f(1.0f, y, 0.0f);
        glVertex3f(1.0f, y + 0.2f, 0.0f); glVertex3f(-1.0f, y + 0.2f, 0.0f);
    }
    glEnd();
    glPopMatrix();
}
// ===== VEGETATION =====
void BackgroundRenderer::drawTrees() {
    // Ink-wash style pine trees using GL_TRIANGLES
    glColor3f(0.1f, 0.2f, 0.1f); // Dark green, almost black
    
    // Stylized Chinese pine trees on mountainsides
    for (int i = 0; i < 6; i++) {
        glPushMatrix();
        float x = -40.0f + i * 15.0f + sin(i) * 8.0f;
        float z = -20.0f + cos(i * 1.5f) * 12.0f;
        float height = 8.0f + sin(i * 2.0f) * 3.0f;
        
        glTranslatef(x, 0.0f, z);
        
        // Tree trunk using GL_QUADS
        glColor3f(0.2f, 0.15f, 0.1f); // Dark brown
        glBegin(GL_QUADS);
        glVertex3f(-0.3f, 0.0f, -0.3f); glVertex3f(0.3f, 0.0f, -0.3f);
        glVertex3f(0.3f, height * 0.6f, -0.3f); glVertex3f(-0.3f, height * 0.6f, -0.3f);
        
        glVertex3f(0.3f, 0.0f, -0.3f); glVertex3f(0.3f, 0.0f, 0.3f);
        glVertex3f(0.3f, height * 0.6f, 0.3f); glVertex3f(0.3f, height * 0.6f, -0.3f);
        glEnd();
        
        // Pine needles/branches using GL_TRIANGLES (ink-wash style)
        glColor3f(0.05f, 0.15f, 0.05f);
        glBegin(GL_TRIANGLES);
        for (int layer = 0; layer < 4; layer++) {
            float y_base = height * 0.3f + layer * height * 0.2f;
            float radius = 3.0f - layer * 0.6f;
            
            // Create layered triangular foliage
            for (int side = 0; side < 6; side++) {
                float angle = side * M_PI / 3.0f;
                float wind_sway = sin(windTime + i + layer) * 0.3f;
                
                glVertex3f(0.0f, y_base + radius, 0.0f);
                glVertex3f(radius * cos(angle) + wind_sway, y_base, radius * sin(angle));
                glVertex3f(radius * cos(angle + M_PI/3) + wind_sway, y_base, radius * sin(angle + M_PI/3));
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
    // Sparse battlefield grass using GL_LINES
    glColor3f(0.2f, 0.3f, 0.15f); // Muted battlefield green
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
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
        
        glVertex3f(x, 0.0f, z);
        glVertex3f(x + sway, height, z);
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
    // Scattered weapons
    glColor3f(0.4f, 0.4f, 0.45f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex3f(-8.0f, 0.1f, 5.0f); glVertex3f(-6.0f, 0.1f, 7.0f); // Sword
    glVertex3f(12.0f, 0.1f, -3.0f); glVertex3f(14.5f, 0.1f, -2.5f);
    glEnd();

    // Broken shields
    glColor3f(0.3f, 0.1f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-12.0f, 0.0f, -5.0f); glVertex3f(-10.0f, 0.0f, -5.5f); glVertex3f(-11.0f, 1.5f, -4.0f);
    glEnd();
    
    // Rubble near damaged structures
    glColor3f(0.3f, 0.3f, 0.3f);
    for(int i = 0; i < 10; i++) {
        glPushMatrix();
        glTranslatef(45.0f -10.0f + (rand()%100)/50.0f, 0.0f, -40.0f + 5.0f + (rand()%100)/50.0f);
        glScalef(0.3f, 0.2f, 0.4f);
        gluSphere(quadric, 1.0, 6, 4); // Use low-poly spheres for rocks
        glPopMatrix();
    }
}

void BackgroundRenderer::drawTatteredBanner(float x, float y, float z, float height, float width, float r, float g, float b) {
    // Pole
    glColor3f(0.3f, 0.2f, 0.1f);
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    glVertex3f(x, 0.0f, z);
    glVertex3f(x, height, z);
    glEnd();

    // Tattered cloth
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 6; i++) {
        float seg = i / 6.0f;
        float windOffset = sin(windTime * 2.5f + seg * 2.0f) * 0.5f * (1.0f - seg);
        float jag = (i % 2 == 0) ? 0.0f : -0.8f; // Jagged bottom edge
        
        glVertex3f(x, height - (seg * 3.0f), z);
        glVertex3f(x + width + windOffset, height - (seg * 3.0f) + jag, z);
    }
    glEnd();
}

void BackgroundRenderer::drawBanners() {
    // Chinese Banner (Gold on Red)
    drawTatteredBanner(-25.0f, 0.0f, -20.0f, 12.0f, 4.0f, 0.7f, 0.1f, 0.1f);
    // Western Banner (Silver on Blue)
    drawTatteredBanner(30.0f, 0.0f, -25.0f, 10.0f, 3.0f, 0.1f, 0.1f, 0.6f);
}

void BackgroundRenderer::drawFires() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Fire on the battlefield
    glPushMatrix();
    glTranslatef(20.0f, 0.1f, -8.0f); // Base of fire
    for (int i = 0; i < 20; i++) {
        float flicker = sin(windTime * 8.0f + i) * 0.2f;
        float height = 1.0f + (rand() % 100) / 100.0f;
        float width = 0.5f + flicker;
        float angle = (rand() % 360) * M_PI / 180.0f;
        
        glPushMatrix();
        glRotatef(angle, 0, 1, 0);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.8f, 0.2f, 0.2f); // Yellow
        glVertex3f(0.0f, height * 1.5f, 0.0f);
        glColor4f(1.0f, 0.4f, 0.1f, 0.6f); // Orange/Red
        glVertex3f(-width, 0.0f, 0.0f);
        glVertex3f(width, 0.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
    
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void BackgroundRenderer::drawSmokePlumes() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f, 0.2f, 0.2f, 0.5f);

    // Smoke from battlefield fire
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        float timeOffset = windTime + i * 1.5f;
        float rise = fmod(timeOffset * 2.0f, 30.0f);
        float sway = sin(timeOffset * 0.5f) * 4.0f;
        glTranslatef(20.0f + sway, rise, -8.0f);
        glScalef(2.0f + rise * 0.2f, 2.0f + rise * 0.2f, 2.0f + rise * 0.2f);
        gluSphere(quadric, 1.0f, 8, 6);
        glPopMatrix();
    }
    
    // Smoke from ruined castle tower
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        float timeOffset = windTime + i * 1.2f;
        float rise = fmod(timeOffset * 1.5f, 40.0f);
        float sway = sin(timeOffset * 0.4f) * 3.0f;
        glTranslatef(45.0f - 10.0f + sway, 8.0f + rise, -40.0f + 5.0f);
        glScalef(2.5f + rise * 0.2f, 2.5f + rise * 0.2f, 2.5f + rise * 0.2f);
        gluSphere(quadric, 1.0f, 8, 6);
        glPopMatrix();
    }
    
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void BackgroundRenderer::drawRain() {
    if (currentWeather != WEATHER_RAIN && currentWeather != WEATHER_STORM) return;
    
    glDisable(GL_LIGHTING);
    glColor3f(0.6f, 0.7f, 0.8f);
    glLineWidth(1.0f);
    float windSlant = 0.5f + sin(windTime) * 0.2f;

    glBegin(GL_LINES);
    for (int i = 0; i < 250; i++) {
        float x = -60.0f + (rand() % 12000) / 100.0f;
        float z = -60.0f + (rand() % 12000) / 100.0f;
        float y_base = fmod(30.0f - (windTime * 20.0f) + (float)(rand() % 30), 30.0f);
        
        glVertex3f(x, y_base, z);
        glVertex3f(x + windSlant, y_base - 2.0f, z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void BackgroundRenderer::drawBirds() {
    // Birds don't fly in bad weather
    if (currentWeather != WEATHER_CLEAR) return;
    
    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int flock = 0; flock < 2; flock++) {
        float baseX = -40.0f + flock * 50.0f + sin(cloudTime + flock) * 15.0f;
        float baseY = 30.0f + cos(cloudTime * 0.7f + flock) * 5.0f;
        float baseZ = -70.0f + flock * 10.0f;
        
        for (int bird = 0; bird < 6; bird++) {
            float x = baseX + (bird - 3) * 2.5f;
            float y = baseY - abs(bird - 3) * 0.8f;
            float wingFlap = sin(windTime * 5.0f + bird) * 0.4f;
            
            glVertex3f(x - 0.5f, y + wingFlap, baseZ); glVertex3f(x, y, baseZ);
            glVertex3f(x, y, baseZ); glVertex3f(x + 0.5f, y + wingFlap, baseZ);
        }
    }
    glEnd();
}

// ===== MAIN RENDER FUNCTION =====
void BackgroundRenderer::render() {
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);

    // Set up lighting based on storm flashes and siege fires
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.25f, 1.0f }; // Darker base for dramatic effect
    light_ambient[0] += lightningBrightness * 0.8f;
    light_ambient[1] += lightningBrightness * 0.8f;
    light_ambient[2] += lightningBrightness;
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

    // Drawing order optimized for dramatic layering (far to near)
    setupFog();
    
    // 1. Atmospheric elements (farthest)
    drawSkyDome();
    drawSunOrMoon();
    
    // 2. Distant mountain ranges (ink-wash style)
    drawMountains();
    drawClouds();
    drawBirds();
    
    // 3. Mid-ground fortifications and structures
    drawChineseFortress();
    drawWesternCastle();
    
    // 4. Siege equipment and effects
    drawSiegeLadders();
    drawCatapultStones();
    
    // 5. Natural elements
    drawTrees();
    drawBattlefield();
    drawGrass();
    
    // 6. Battle aftermath and active siege effects
    drawWeaponsAndDebris();
    drawBanners();
    drawFires();
    drawSiegeEffects(); // Active battle effects
    drawSmokePlumes(); // Smoke should be drawn over most elements

    // 7. Weather overlays (drawn last)
    drawRain();
    drawLightning(); // 2D effect, drawn over everything

    glPopMatrix();
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