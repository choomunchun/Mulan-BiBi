#include "shield.h"
#include <Windows.h>
#include <gl/GL.h>
#define _USE_MATH_DEFINES
#include <cmath>

void drawShield()
{
    const int segments = 32; // Increased for a smoother circle
    const int rings = 8;
    const float radius = 1.8f;
    const float curve = 0.4f;
    const float rimWidth = 0.15f; // How thick the rim is

    // --- 1. Draw the inner, curved face of the shield ---
    glColor3f(1.0f, 0.84f, 0.0f); // Gold color
    float innerRadius = radius * (1.0f - rimWidth);

    for (int i = 0; i < rings; ++i) {
        float r0 = innerRadius * ((float)i / rings);
        float r1 = innerRadius * ((float)(i + 1) / rings);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= segments; ++j) {
            float angle = 2.0f * (float)M_PI * j / segments;
            float c = cos(angle);
            float s = sin(angle);

            float x0 = r0 * c;
            float y0 = r0 * s;
            float z0 = curve * (1.0f - (r0 / radius) * (r0 / radius));
            glNormal3f(x0, y0, 1.0f);
            glVertex3f(x0, y0, z0);

            float x1 = r1 * c;
            float y1 = r1 * s;
            float z1 = curve * (1.0f - (r1 / radius) * (r1 / radius));
            glNormal3f(x1, y1, 1.0f);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }

    // --- 2. Draw the outer rim ---
    glColor3f(0.9f, 0.74f, 0.0f); // Slightly darker gold for the rim
    float rim_z = curve * (1.0f - (innerRadius / radius) * (innerRadius / radius));

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * (float)M_PI * i / segments;
        float c = cos(angle);
        float s = sin(angle);

        // Inner edge of the rim
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(innerRadius * c, innerRadius * s, rim_z);

        // Outer edge of the rim
        glVertex3f(radius * c, radius * s, rim_z * 0.8f); // Slightly tapered
    }
    glEnd();
}
