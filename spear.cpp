#include "spear.h"
#include <Windows.h>
#include <gl/GL.h>
#define _USE_MATH_DEFINES
#include <cmath>

// Helper structure for 3D vectors
struct Vec3f {
    float x, y, z;
};

// Helper function for vector math
Vec3f cross(const Vec3f& a, const Vec3f& b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
Vec3f normalize(const Vec3f& v) {
    float l = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (l > 1e-6f) {
        return { v.x / l, v.y / l, v.z / l };
    }
    return { 0, 0, 0 };
}

void drawSpear()
{
    const int segments = 12;
    const float shaftHeight = 10.0f;
    const float shaftRadius = 0.1f;

    // --- 1. Draw Spear Shaft ---
    glColor3f(0.2f, 0.15f, 0.1f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * (float)M_PI * i / segments;
        float x = shaftRadius * cos(angle);
        float z = shaftRadius * sin(angle);
        glNormal3f(x / shaftRadius, 0.0f, z / shaftRadius);
        glVertex3f(x, 0, z);
        glVertex3f(x, shaftHeight, z);
    }
    glEnd();

    // --- 2. Draw Golden Butt Cap (Pommel) ---
    glColor3f(1.0f, 0.84f, 0.0f); // Gold color
    glPushMatrix();
    // Conical part of the pommel
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -0.4f, 0.0f); // Tip
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * (float)M_PI * i / segments;
        float x = shaftRadius * 1.5f * cos(angle);
        float z = shaftRadius * 1.5f * sin(angle);
        glNormal3f(x, -0.5f, z);
        glVertex3f(x, 0.0f, z); // Base
    }
    glEnd();
    glPopMatrix();
    // The decorative ring below the pommel has been removed.

    // --- 3. Draw Golden Guard ---
    glColor3f(1.0f, 0.84f, 0.0f); // Gold color
    glPushMatrix();
    glTranslatef(0.0f, shaftHeight, 0.0f);
    const float guardRadius = 0.2f;
    const float guardHeight = 0.3f;

    // Sides of the guard ring
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * (float)M_PI * i / segments;
        float x = guardRadius * cos(angle);
        float z = guardRadius * sin(angle);
        glNormal3f(x / guardRadius, 0.0f, z / guardRadius);
        glVertex3f(x, 0.0f, z); // Bottom edge of guard
        glVertex3f(x, guardHeight, z); // Top edge of guard
    }
    glEnd();

    // **NEW**: Bottom cap of the guard ring
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f); // Center vertex
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * (float)M_PI * i / segments;
        glVertex3f(guardRadius * cos(angle), 0.0f, guardRadius * sin(angle));
    }
    glEnd();

    // **NEW**: Top cap of the guard ring
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, guardHeight, 0.0f); // Center vertex
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * (float)M_PI * i / segments;
        glVertex3f(guardRadius * cos(-angle), guardHeight, guardRadius * sin(-angle));
    }
    glEnd();

    glPopMatrix();

    // --- 4. Draw 3D Spear Blade ---
    glColor3f(0.75f, 0.85f, 0.9f);
    glPushMatrix();
    glTranslatef(0.0f, shaftHeight + guardHeight, 0.0f);

    const float bladeHeight = 2.5f;
    const float bladeWidth = 0.5f;
    const float bladeThickness = 0.2f;

    Vec3f tip = { 0.0f, bladeHeight, 0.0f };
    Vec3f base_front = { 0.0f, 0.0f, bladeThickness / 2.0f };
    Vec3f base_back = { 0.0f, 0.0f, -bladeThickness / 2.0f };
    Vec3f base_left = { -bladeWidth / 2.0f, 0.0f, 0.0f };
    Vec3f base_right = { bladeWidth / 2.0f, 0.0f, 0.0f };

    Vec3f n_fr = normalize(cross({ base_right.x - tip.x, base_right.y - tip.y, base_right.z - tip.z }, { base_front.x - tip.x, base_front.y - tip.y, base_front.z - tip.z }));
    Vec3f n_fl = normalize(cross({ base_front.x - tip.x, base_front.y - tip.y, base_front.z - tip.z }, { base_left.x - tip.x, base_left.y - tip.y, base_left.z - tip.z }));
    Vec3f n_bl = normalize(cross({ base_left.x - tip.x, base_left.y - tip.y, base_left.z - tip.z }, { base_back.x - tip.x, base_back.y - tip.y, base_back.z - tip.z }));
    Vec3f n_br = normalize(cross({ base_back.x - tip.x, base_back.y - tip.y, base_back.z - tip.z }, { base_right.x - tip.x, base_right.y - tip.y, base_right.z - tip.z }));

    glBegin(GL_TRIANGLES);
    glNormal3f(n_fr.x, n_fr.y, n_fr.z);
    glVertex3f(tip.x, tip.y, tip.z);
    glVertex3f(base_right.x, base_right.y, base_right.z);
    glVertex3f(base_front.x, base_front.y, base_front.z);

    glNormal3f(n_fl.x, n_fl.y, n_fl.z);
    glVertex3f(tip.x, tip.y, tip.z);
    glVertex3f(base_front.x, base_front.y, base_front.z);
    glVertex3f(base_left.x, base_left.y, base_left.z);

    glNormal3f(n_bl.x, n_bl.y, n_bl.z);
    glVertex3f(tip.x, tip.y, tip.z);
    glVertex3f(base_left.x, base_left.y, base_left.z);
    glVertex3f(base_back.x, base_back.y, base_back.z);

    glNormal3f(n_br.x, n_br.y, n_br.z);
    glVertex3f(tip.x, tip.y, tip.z);
    glVertex3f(base_back.x, base_back.y, base_back.z);
    glVertex3f(base_right.x, base_right.y, base_right.z);
    glEnd();

    glPopMatrix();
}
