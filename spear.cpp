#include "spear.h"
#include <Windows.h>
#include <gl/GL.h>
#include <cmath>
#include "utils.h"
#include "texture.h"
// Honor global mode picked by keys 1/2/3 from your main file:
extern bool g_TextureEnabled;   // set by setRenderMode(...)

enum RenderMode { RM_WIREFRAME, RM_SOLID, RM_TEXTURED };
extern RenderMode gRenderMode;

void drawSpear()
{
    const int segments = 12;
    const float shaftHeight = 10.0f;
    const float shaftRadius = 0.1f;

    // --- 1) Spear Shaft (wood) ---
    { // wood-like brown when not textured
        // --- 1) Spear Shaft (wood) ---
        if (gRenderMode == RM_TEXTURED) {
            Tex::bind(Tex::id[Tex::Wood]);
            glColor3f(1.0f, 1.0f, 1.0f);
            Tex::enableObjectLinearST(0.5f, 0.5f, 1.0f);
        }
        else {
            glColor3f(0.6f, 0.4f, 0.2f); // Wood color
        }
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * PI * i / segments;
            float x = shaftRadius * cosf(angle);
            float z = shaftRadius * sinf(angle);
            glNormal3f(x / shaftRadius, 0.0f, z / shaftRadius);
            glVertex3f(x, 0, z);
            glVertex3f(x, shaftHeight, z);
        }
        glEnd();
        if (gRenderMode == RM_TEXTURED) {
            Tex::disableObjectLinearST();
            Tex::unbind();
        }
    }

    // --- 2) Golden Butt Cap (pommel) ---
    {
        if (gRenderMode == RM_TEXTURED) {
            Tex::bind(Tex::id[Tex::Gold]);
            glColor3f(1.0f, 1.0f, 1.0f);
            Tex::enableObjectLinearST(0.5f, 0.5f, 1.0f);
        }
        else {
            glColor3f(1.0f, 0.84f, 0.0f); // Gold color
        }
        glPushMatrix();
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.0f, -0.4f, 0.0f);
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * PI * i / segments;
            float x = shaftRadius * 1.5f * cosf(angle);
            float z = shaftRadius * 1.5f * sinf(angle);
            glNormal3f(x, -0.5f, z);
            glVertex3f(x, 0.0f, z);
        }
        glEnd();
        glPopMatrix();
        if (gRenderMode == RM_TEXTURED) {
            Tex::disableObjectLinearST();
            Tex::unbind();
        }
    }

    // --- 3) Golden Guard ---
    {
        if (gRenderMode == RM_TEXTURED) {
            Tex::bind(Tex::id[Tex::Gold]);
            glColor3f(1.0f, 1.0f, 1.0f);
            Tex::enableObjectLinearST(0.5f, 0.5f, 1.0f);
        }
        else {
            glColor3f(1.0f, 0.84f, 0.0f); // Gold color
        }
        glPushMatrix();
        glTranslatef(0.0f, shaftHeight, 0.0f);
        const float guardRadius = 0.2f;
        const float guardHeight = 0.3f;

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * PI * i / segments;
            float x = guardRadius * cosf(angle);
            float z = guardRadius * sinf(angle);
            glNormal3f(x / guardRadius, 0.0f, z / guardRadius);
            glVertex3f(x, 0.0f, z);
            glVertex3f(x, guardHeight, z);
        }
        glEnd();

        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * PI * i / segments;
            glVertex3f(guardRadius * cosf(angle), 0.0f, guardRadius * sinf(angle));
        }
        glEnd();

        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, guardHeight, 0.0f);
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * PI * i / segments;
            glVertex3f(guardRadius * cosf(-angle), guardHeight, guardRadius * sinf(-angle));
        }
        glEnd();
        glPopMatrix();
        if (gRenderMode == RM_TEXTURED) {
            Tex::disableObjectLinearST();
            Tex::unbind();
        }
    }

    // --- 4) Blade (weapon) ---
    {
        if (gRenderMode == RM_TEXTURED) {
            Tex::bind(Tex::id[Tex::Weapon]);
            glColor3f(1.0f, 1.0f, 1.0f);
            Tex::enableObjectLinearST(0.5f, 0.5f, 1.0f);
        }
        else {
            glColor3f(0.8f, 0.8f, 0.85f); // Silver/Steel color
        }
        glPushMatrix();
        glTranslatef(0.0f, 10.0f + 0.3f, 0.0f); // shaftHeight + guardHeight
        const float bladeHeight = 2.5f, bladeWidth = 0.5f, bladeThickness = 0.2f;

        Vec3f tip{ 0.0f, bladeHeight, 0.0f };
        Vec3f base_front{ 0.0f, 0.0f,  bladeThickness / 2.0f };
        Vec3f base_back{ 0.0f, 0.0f, -bladeThickness / 2.0f };
        Vec3f base_left{ -bladeWidth / 2.0f, 0.0f, 0.0f };
        Vec3f base_right{ bladeWidth / 2.0f, 0.0f, 0.0f };

        Vec3f n_fr = normalize(cross(sub(base_right, tip), sub(base_front, tip)));
        Vec3f n_fl = normalize(cross(sub(base_front, tip), sub(base_left, tip)));
        Vec3f n_bl = normalize(cross(sub(base_left, tip), sub(base_back, tip)));
        Vec3f n_br = normalize(cross(sub(base_back, tip), sub(base_right, tip)));

        glBegin(GL_TRIANGLES);
        glNormal3f(n_fr.x, n_fr.y, n_fr.z); glVertex3f(tip.x, tip.y, tip.z); glVertex3f(base_right.x, base_right.y, base_right.z); glVertex3f(base_front.x, base_front.y, base_front.z);
        glNormal3f(n_fl.x, n_fl.y, n_fl.z); glVertex3f(tip.x, tip.y, tip.z); glVertex3f(base_front.x, base_front.y, base_front.z); glVertex3f(base_left.x, base_left.y, base_left.z);
        glNormal3f(n_bl.x, n_bl.y, n_bl.z); glVertex3f(tip.x, tip.y, tip.z); glVertex3f(base_left.x, base_left.y, base_left.z); glVertex3f(base_back.x, base_back.y, base_back.z);
        glNormal3f(n_br.x, n_br.y, n_br.z); glVertex3f(tip.x, tip.y, tip.z); glVertex3f(base_back.x, base_back.y, base_back.z); glVertex3f(base_right.x, base_right.y, base_right.z);
        glEnd();

        glPopMatrix();
        if (gRenderMode == RM_TEXTURED) {
            Tex::disableObjectLinearST();
            Tex::unbind();
        }
    }
}
