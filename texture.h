#pragma once
#include <Windows.h>
#include <gl/GL.h>

// Central registry of all model textures
namespace Tex {
    enum Name {
        Skin,         // skin.bmp
        Skirt,        // skirt.bmp (default)
        SkirtPink,    // texturess/pink skirt.bmp
        SkirtTiffany, // texturess/tiffany skirt.bmp
        SkirtRed,     // texturess/red skirt.bmp
        SkirtYellow,  // texturess/yellow skirt.bmp
        Helmet,       // helmet.bmp
        Armor,        // armor.bmp
        Sabatons,     // sabatons.bmp
        Silver,       // silver.bmp        (concrete hands, leg armor)
        Shield,       // shield.bmp
        Weapon,       // weapon.bmp        (sword/spear bits)
        Wood,         // wood.bmp          (spear shaft)
        Gold,         // gold.bmp          (guards)
        Handle,       // handle.bmp        (sword grip)
        Hair,         // hair.bmp
        COUNT
    };

    extern GLuint id[COUNT];

    // Loads every texture into Tex::id[]. Safe to call once at init.
    void loadAll();

    // Simple 2D bind/unbind (inline to avoid link issues)
    inline void bind(GLuint texId) { glBindTexture(GL_TEXTURE_2D, texId); }
    inline void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

    // Object-linear S/T generator (S from X/Z, T from Y)
    void enableObjectLinearST(float sX, float sZ, float tY);
    void disableObjectLinearST();
    extern bool g_TextureEnabled;

    // Skirt texture cycling support
    extern int currentSkirtIndex;
    extern const int skirtTextureCount;
    extern Name skirtTextures[5];
    void cycleSkirtTexture();
    GLuint getCurrentSkirtTexture();
}
