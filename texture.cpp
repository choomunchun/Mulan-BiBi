#include "texture.h"
#include <vector>
#include <cmath>

#ifndef GL_BGR
#define GL_BGR  0x80E0   // fallback if glext.h not included
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

static GLuint loadTextureBMP(const char* filename) {
    GLuint texture = 0;

    BITMAP bmp{};
    HBITMAP hBMP = (HBITMAP)LoadImageA(
        GetModuleHandleA(nullptr), filename, IMAGE_BITMAP,
        0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE
    );
    if (!hBMP) {
        printf("ERROR: Failed to load texture: %s\n", filename);
        return 0;
    }

    printf("Successfully loaded texture: %s\n", filename);

    GetObject(hBMP, sizeof(bmp), &bmp);

    // Windows DIB rows are 4-byte aligned; make that explicit
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Reasonable defaults
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // (no mipmaps)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Choose proper format based on bpp
    GLenum srcFormat = (bmp.bmBitsPixel == 32) ? GL_BGRA : GL_BGR;

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB,
        bmp.bmWidth, bmp.bmHeight, 0,
        srcFormat, GL_UNSIGNED_BYTE, bmp.bmBits
    );

    DeleteObject(hBMP);
    return texture;
}

// ===== Optional: procedural fallback for skin if skin.bmp is missing =====
static GLuint createProceduralSkinTexture() {
    const int W = 64, H = 64;
    std::vector<unsigned char> data(W * H * 3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = (y * W + x) * 3;
            float variation = (sinf(x * 0.2f) * cosf(y * 0.15f) + 1.0f) * 0.1f;
            data[i + 0] = (unsigned char)(220 + variation * 35.0f);  // R
            data[i + 1] = (unsigned char)(180 + variation * 25.0f);  // G
            data[i + 2] = (unsigned char)(140 + variation * 20.0f);  // B
        }
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

GLuint Tex::id[Tex::COUNT] = { 0 };

void Tex::loadAll() {
    printf("Loading textures...\n");
    
    id[Skin] = loadTextureBMP("skin.bmp");
    id[Skirt] = loadTextureBMP("skirt.bmp");  // Load default from root first
    id[SkirtPink] = loadTextureBMP("texturess/pink skirt.bmp");
    id[SkirtTiffany] = loadTextureBMP("texturess/tiffany skirt.bmp");
    id[SkirtRed] = loadTextureBMP("texturess/red skirt.bmp");
    id[SkirtYellow] = loadTextureBMP("texturess/yellow skirt.bmp");
    id[Helmet] = loadTextureBMP("helmet.bmp");
    id[Armor] = loadTextureBMP("armor.bmp");
    id[Sabatons] = loadTextureBMP("sabatons.bmp");
    id[Silver] = loadTextureBMP("silver.bmp");
    id[Shield] = loadTextureBMP("shield.bmp");
    id[Weapon] = loadTextureBMP("weapon.bmp");
    id[Wood] = loadTextureBMP("wood.bmp");
    id[Gold] = loadTextureBMP("gold.bmp");
    id[Handle] = loadTextureBMP("handle.bmp");
    id[Hair] = loadTextureBMP("hair.bmp");

    // Fallbacks - try texturess folder if main texture failed to load
    if (id[Skirt] == 0) {
        printf("Trying fallback path for default skirt...\n");
        id[Skirt] = loadTextureBMP("texturess/skirt.bmp");
    }
    if (id[Skin] == 0) id[Skin] = createProceduralSkinTexture();
    
    // Additional debug info for skirt textures
    printf("Skirt texture IDs: Default=%u, Pink=%u, Tiffany=%u, Red=%u, Yellow=%u\n", 
           id[Skirt], id[SkirtPink], id[SkirtTiffany], id[SkirtRed], id[SkirtYellow]);
    
    printf("Texture loading complete. Skirt textures loaded: %s\n", 
           (id[Skirt] && id[SkirtPink] && id[SkirtTiffany] && id[SkirtRed] && id[SkirtYellow]) ? "All" : "Some missing");
}

void Tex::enableObjectLinearST(float sX, float sZ, float tY) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    const GLfloat sPlane[4] = { sX, 0.0f, sZ, 0.0f }; // S from X/Z
    const GLfloat tPlane[4] = { 0.0f, tY, 0.0f, 0.0f }; // T from Y

    glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
}

void Tex::disableObjectLinearST() {
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}

// Skirt texture cycling implementation
int Tex::currentSkirtIndex = 0;
const int Tex::skirtTextureCount = 5;
Tex::Name Tex::skirtTextures[5] = { Tex::Skirt, Tex::SkirtPink, Tex::SkirtTiffany, Tex::SkirtRed, Tex::SkirtYellow };

void Tex::cycleSkirtTexture() {
    currentSkirtIndex = (currentSkirtIndex + 1) % skirtTextureCount;
    
    // Console output with texture names
    const char* textureNames[] = { "Default Skirt", "Pink Skirt", "Tiffany Skirt", "Red Skirt", "Yellow Skirt" };
    printf("Skirt texture changed to: %s (Index: %d)\n", textureNames[currentSkirtIndex], currentSkirtIndex);
}

GLuint Tex::getCurrentSkirtTexture() {
    GLuint currentTexture = id[skirtTextures[currentSkirtIndex]];
    
    // Safety check: if current texture failed to load, try to find any working skirt texture
    if (currentTexture == 0) {
        printf("WARNING: Current skirt texture (index %d) failed to load, searching for alternatives...\n", currentSkirtIndex);
        for (int i = 0; i < skirtTextureCount; i++) {
            if (id[skirtTextures[i]] != 0) {
                printf("Using fallback skirt texture at index %d\n", i);
                currentSkirtIndex = i; // Update to working texture
                return id[skirtTextures[i]];
            }
        }
        printf("ERROR: No skirt textures loaded successfully!\n");
        return 0; // This will cause solid color fallback
    }
    
    return currentTexture;
}
