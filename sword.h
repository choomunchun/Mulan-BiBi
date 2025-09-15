#pragma once

#define NOMINMAX
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#define _USE_MATH_DEFINES
#include <cmath>

// Sword state variables (extern declarations)
extern bool gSwordVisible;
extern bool gSwordInRightHand;
extern float gSwordScale;

// Sword material structure
struct SwordMaterial {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess;
};

// Material definitions (extern declarations)
extern SwordMaterial polishedSteel;
extern SwordMaterial darkJade;
extern SwordMaterial hiltWrap;

// Function declarations
void setSwordMaterial(const SwordMaterial& mat);
void drawSwordSphere(double r, int lats, int longs);
void drawSwordCylinder(double r, double h, int slices);
void drawSwordInscription();
void drawSwordBlade();
void drawSwordGuard();
void drawSwordHilt();
void drawSwordPommel();
void drawSwordTassel();
void drawSword();