#pragma once

#include <vector>
#include <gl/GL.h>   // Added for OpenGL types
#include <gl/GLU.h>  // Added for GLUquadric
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef PI
#define PI 3.1415926535f
#endif

// --- Data Structures ---
struct Vec3f { float x, y, z; };
struct Tri { int a, b, c; };

// --- Math Helpers ---
inline Vec3f sub(const Vec3f& p, const Vec3f& q) { return { p.x - q.x, p.y - q.y, p.z - q.z }; }
inline Vec3f cross(const Vec3f& a, const Vec3f& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
inline Vec3f normalize(const Vec3f& v) { float l = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); if (l > 1e-6f) return { v.x / l, v.y / l, v.z / l }; return { 0,0,0 }; }
extern GLUquadric* g_headQuadric;
extern float* segCos;
extern float* segSin;
// --- Torso Shape Definitions ---
#define TORSO_SEGMENTS 24
#define HEAD_RADIUS 0.3f
#define R0 0.50f
#define R1 0.49f
#define R2 0.48f
#define R3 0.47f
#define R4 0.46f
#define R5 0.43f
#define R6 0.40f
#define R7 0.37f
#define R8 0.35f
#define R9 0.37f
#define R10 0.39f
#define R11 0.41f
#define R12 0.45f
#define R13 0.44f
#define R14 0.43f
#define R15 0.40f
#define R16 0.28f
#define R17 0.16f
#define R18 0.09f
#define R19 0.07f
#define Y0 0.00f
#define Y1 0.08f
#define Y2 0.16f
#define Y3 0.24f
#define Y4 0.32f
#define Y5 0.45f
#define Y6 0.58f
#define Y7 0.70f
#define Y8 0.83f
#define Y9 0.96f
#define Y10 1.09f
#define Y11 1.22f
#define Y12 1.31f
#define Y13 1.41f
#define Y14 1.47f
#define Y15 1.54f
#define Y16 1.58f
#define Y17 1.66f
#define Y18 1.76f
#define Y19 1.85f

// --- Geometry Helper Functions ---
inline std::vector<Vec3f> generateRing(float cx, float cy, float cz, float radiusX, float radiusZ, int numSegments, float startAngle = 0.0f) {
    std::vector<Vec3f> ringVertices;
    for (int i = 0; i < numSegments; ++i) {
        float angle = startAngle + 2.0f * PI * i / numSegments;
        ringVertices.push_back({ cx + radiusX * sinf(angle), cy, cz + radiusZ * cosf(angle) });
    }
    return ringVertices;
}

inline void drawCurvedBand(float rA, float yA, float rB, float yB, const float* segCos, const float* segSin) {
    for (int i = 0; i < TORSO_SEGMENTS; ++i) {
        float cA = segCos[i];
        float sA = segSin[i];
        float cB = segCos[(i + 1) % TORSO_SEGMENTS];
        float sB = segSin[(i + 1) % TORSO_SEGMENTS];

        Vec3f v1A = { rA * cA, yA, rA * sA };
        Vec3f v2A = { rB * cA, yB, rB * sA };
        Vec3f v1B = { rA * cB, yA, rA * sB };
        Vec3f v2B = { rB * cB, yB, rB * sB };

        if (sA < 0) { v1A.z *= 0.5f; v2A.z *= 0.5f; }
        if (sB < 0) { v1B.z *= 0.5f; v2B.z *= 0.5f; }

        Vec3f nA = normalize(cross(sub(v2A, v1A), sub(v1B, v1A)));
        Vec3f nB = normalize(cross(sub(v2B, v1B), sub(v1A, v1B)));

        glNormal3f(nA.x, nA.y, nA.z);
        glVertex3f(v1A.x, v1A.y, v1A.z);
        glVertex3f(v2A.x, v2A.y, v2A.z);
        glVertex3f(v2B.x, v2B.y, v2B.z);

        glNormal3f(nB.x, nB.y, nB.z);
        glVertex3f(v1A.x, v1A.y, v1A.z);
        glVertex3f(v2B.x, v2B.y, v2B.z);
        glVertex3f(v1B.x, v1B.y, v1B.z);
    }
}
