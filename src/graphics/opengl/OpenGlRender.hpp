#pragma once

struct P2 {
    float x;
    float y;
};

// Layout parameters for isometric scene rendering
struct IsoLayout {
    float panelW;       // Side panel width
    float halfW;        // Diamond half-width
    float halfH;        // Diamond half-height
    float pitchScale;   // Vertical projection scale from camera pitch
    bool cameraBelow;   // True when camera is under the board
    float originX;      // Screen-space origin X
    float originY;      // Screen-space origin Y
    float centerX;      // Grid center X
    float centerY;      // Grid center Y
    float yawCos;       // Camera yaw cosine
    float yawSin;       // Camera yaw sine
    P2 ex;              // Isometric X basis vector
    P2 ey;              // Isometric Y basis vector
};

// Cellules avec les infos profondeurs
struct RenderCell {
    int x;
    int y;
    float depth;
    float tie;
};
