#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cmath>
#include "sword.h"
#include "utils.h"

// --- Sword State Variables ---
bool gSwordVisible = false;
bool gSwordInRightHand = true; // true = right hand, false = left hand
float gSwordScale = 0.3f; // Scale down the sword to fit in hand

// SWORD CONTROLS:
// Press 'X' to toggle sword visibility
// Press 'Z' to switch sword between left and right hand
// When holding sword, the hand automatically forms a fist for proper grip

// Define materials for our sword parts
SwordMaterial polishedSteel = {
    {0.23f, 0.23f, 0.23f, 1.0f}, {0.77f, 0.77f, 0.77f, 1.0f},
    {0.99f, 0.99f, 0.99f, 1.0f}, 100.0f
};
SwordMaterial darkJade = {
    {0.0f, 0.1f, 0.05f, 1.0f}, {0.1f, 0.35f, 0.2f, 1.0f},
    {0.45f, 0.55f, 0.45f, 1.0f}, 32.0f
};
SwordMaterial hiltWrap = {
    {0.08f, 0.05f, 0.05f, 1.0f}, {0.2f, 0.15f, 0.15f, 1.0f},
    {0.1f, 0.1f, 0.1f, 1.0f}, 10.0f
};

// Helper function to apply a material
void setSwordMaterial(const SwordMaterial& mat) {
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat.specular);
    glMaterialf(GL_FRONT, GL_SHININESS, mat.shininess);
}

// --- Sword Drawing Functions ---
void drawSwordSphere(double r, int lats, int longs) {
    for (int i = 0; i <= lats; i++) {
        double lat0 = PI * (-0.5 + (double)(i - 1) / lats);
        double z0 = r * sin(lat0); double zr0 = r * cos(lat0);
        double lat1 = PI * (-0.5 + (double)i / lats);
        double z1 = r * sin(lat1); double zr1 = r * cos(lat1);
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= longs; j++) {
            double lng = 2 * PI * (double)(j - 1) / longs;
            double x = cos(lng); double y = sin(lng);
            glNormal3f(x * zr0, y * zr0, z0); glVertex3f(x * zr0, y * zr0, z0);
            glNormal3f(x * zr1, y * zr1, z1); glVertex3f(x * zr1, y * zr1, z1);
        }
        glEnd();
    }
}

void drawSwordCylinder(double r, double h, int slices) {
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * PI * i / slices;
        float x = cos(angle); float z = sin(angle);
        glNormal3f(x, 0.0f, z);
        glVertex3f(r * x, h, r * z); glVertex3f(r * x, 0.0f, r * z);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, h, 0.0f);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * PI * i / slices;
        glVertex3f(r * cos(angle), h, r * sin(angle));
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = slices; i >= 0; i--) {
        float angle = 2.0f * PI * i / slices;
        glVertex3f(r * cos(angle), 0.0f, r * sin(angle));
    }
    glEnd();
}

void drawSwordInscription() {
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.1f, 0.1f); glLineWidth(2.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.051f);
    glScalef(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINES); // 忠
    glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, -1); glVertex2f(1, 1);
    glVertex2f(1, 1); glVertex2f(-1, 1); glVertex2f(-1, 1); glVertex2f(-1, -1);
    glVertex2f(0, -1); glVertex2f(0, 1);
    glEnd();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glBegin(GL_LINES); // 勇
    glVertex2f(0, 1); glVertex2f(-1, 0.5); glVertex2f(0, 1); glVertex2f(1, 0.5);
    glVertex2f(-0.5, 0); glVertex2f(0.5, 0); glVertex2f(-1.2, -0.5); glVertex2f(1.2, -0.5);
    glVertex2f(0, -0.5); glVertex2f(-0.5, -1);
    glEnd();
    glTranslatef(0.0f, -3.0f, 0.0f);
    glBegin(GL_LINES); // 真
    glVertex2f(0, 1); glVertex2f(0, 0); glVertex2f(-1, 1); glVertex2f(1, 1);
    glVertex2f(-1, 0); glVertex2f(1, 0); glVertex2f(-0.8, -0.5); glVertex2f(-0.2, -1);
    glVertex2f(0.8, -0.5); glVertex2f(0.2, -1);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawSwordBlade() {
    setSwordMaterial(polishedSteel);
    float bladeLength = 5.0f; float bladeWidth = 0.2f; float bladeThickness = 0.05f;
    glBegin(GL_QUADS);
    glNormal3f(0.5f, 0.125f, 0.0f);
    glVertex3f(0.0f, 0.0f, bladeThickness); glVertex3f(bladeWidth, 0.0f, 0.0f); glVertex3f(bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(0.5f, -0.125f, 0.0f);
    glVertex3f(bladeWidth, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, -bladeThickness); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(bladeWidth, bladeLength, 0.0f);
    glNormal3f(-0.5f, 0.125f, 0.0f);
    glVertex3f(0.0f, 0.0f, bladeThickness); glVertex3f(-bladeWidth, 0.0f, 0.0f); glVertex3f(-bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(-0.5f, -0.125f, 0.0f);
    glVertex3f(-bladeWidth, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, -bladeThickness); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glEnd();
    glBegin(GL_TRIANGLES);
    float tipLength = 0.5f;
    glNormal3f(0.5f, 0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(0.5f, -0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(bladeWidth, bladeLength, 0.0f);
    glNormal3f(-0.5f, 0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(-bladeWidth, bladeLength, 0.0f); glVertex3f(0.0f, bladeLength, bladeThickness);
    glNormal3f(-0.5f, -0.5f, 0.0f); glVertex3f(0.0f, bladeLength + tipLength, 0.0f); glVertex3f(0.0f, bladeLength, -bladeThickness); glVertex3f(-bladeWidth, bladeLength, 0.0f);
    glEnd();
    drawSwordInscription();
}

void drawSwordGuard() {
    setSwordMaterial(darkJade);
    float guardWidth = 0.6f; float guardHeight = 0.2f; float guardDepth = 0.1f; int segments = 10;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = (PI / segments) * i;
        float x = guardWidth * cos(angle); float y = guardHeight * sin(angle) - (guardHeight * 0.5f);
        glNormal3f(cos(angle), sin(angle), 0.0f);
        glVertex3f(x, y, guardDepth); glVertex3f(x, y, -guardDepth);
    }
    glEnd();
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = (PI / segments) * i;
        float x = -guardWidth * cos(angle); float y = guardHeight * sin(angle) - (guardHeight * 0.5f);
        glNormal3f(-cos(angle), sin(angle), 0.0f);
        glVertex3f(x, y, guardDepth); glVertex3f(x, y, -guardDepth);
    }
    glEnd();
}

void drawSwordHilt() {
    setSwordMaterial(hiltWrap);
    drawSwordCylinder(0.1f, 1.2f, 20);
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.4f, 0.25f);
    float wrapRadius = 0.1f * 1.05f; float wrapWidth = 0.02f;
    glBegin(GL_QUAD_STRIP);
    for (float i = 0; i < 1.2f; i += 0.05f) {
        float angle = i * 15.0f;
        glVertex3f(wrapRadius * cos(angle), i, wrapRadius * sin(angle));
        glVertex3f((wrapRadius - wrapWidth) * cos(angle), i, (wrapRadius - wrapWidth) * sin(angle));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawSwordPommel() {
    setSwordMaterial(darkJade);
    glPushMatrix();
    glScalef(1.0f, 0.7f, 1.0f);
    drawSwordSphere(0.18f, 30, 30);
    glPopMatrix();
}

void drawSwordTassel() {
    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.0f);
    setSwordMaterial(darkJade);
    drawSwordSphere(0.05f, 20, 20);
    glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.9f, 0.2f, 0.3f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
        float angle = 2 * PI * i / 12.0f;
        glVertex3f(0.0f, -0.1f, 0.0f);
        glVertex3f(0.2f * cos(angle), -0.8f, 0.2f * sin(angle));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// Main sword drawing function
void drawSword() {
    if (!gSwordVisible) return;

    glPushMatrix();

    // Scale the sword to fit in hand
    glScalef(gSwordScale, gSwordScale, gSwordScale);

    // Position sword components relative to grip point (0,0,0)
    glTranslatef(0.0, -3.0, 0.0); // Move so grip is at origin

    // Render sword components
    drawSwordBlade();
    drawSwordGuard();
    glPushMatrix();
    glTranslatef(0.0, -1.2, 0.0);
    drawSwordHilt();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0, -1.3, 0.0);
    drawSwordPommel();
    drawSwordTassel();
    glPopMatrix();

    glPopMatrix();
}