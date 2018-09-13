#pragma once

namespace facelapse {
    struct CoordinatePair {
        float rX, rY;
        float lX, lY;

        CoordinatePair(float rx = -1, float ry = -1, float lx = -1, float ly = -1);

        float dist();
        bool isComplete();
    };
}