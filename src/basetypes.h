#ifndef BASETYPES_H
#define BASETYPES_H

#include <cstdint>

// Basic coordinate structure
struct Coord {
    int32_t x, y;
    
    bool operator<(const Coord& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    
    bool operator==(const Coord& other) const {
        return x == other.x && y == other.y;
    }
};

// Double precision coordinate structure
struct CoordD {
    double x, y;
    
    bool operator<(const CoordD& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    
    bool operator==(const CoordD& other) const {
        return x == other.x && y == other.y;
    }
};

// CDP with inline information
struct CdpWithInline {
    Coord coord;
    int32_t cdp;
    int32_t iline;
    int32_t xline;
    
    bool operator<(const CdpWithInline& other) const {
        if (cdp != other.cdp) return cdp < other.cdp;
        if (iline != other.iline) return iline < other.iline;
        return xline < other.xline;
    }
};

// Source information
struct SourceInfo {
    int32_t ffid;
    int32_t source;
    int32_t sou_x;
    int32_t sou_y;
    int32_t sou_elev;
    
    bool operator<(const SourceInfo& other) const {
        // Для уникальности источников сравниваем только координаты (X, Y)
        // FFID, source и elevation могут различаться для одного и того же источника
        if (sou_x != other.sou_x) return sou_x < other.sou_x;
        return sou_y < other.sou_y;
    }
};

// Receiver information
struct ReceiverInfo {
    int32_t rec_x;
    int32_t rec_y;
    int32_t rec_elev;
    
    bool operator<(const ReceiverInfo& other) const {
        // Для уникальности приемников сравниваем только координаты (X, Y)
        // Высота (elevation) может различаться для одного и того же приемника
        if (rec_x != other.rec_x) return rec_x < other.rec_x;
        return rec_y < other.rec_y;
    }
};

// CDP information
struct CdpInfo {
    int32_t cdp;
    int32_t cdp_x;
    int32_t cdp_y;
    
    bool operator<(const CdpInfo& other) const {
        // Для уникальности CDP сравниваем только координаты (X, Y)
        // Номер CDP может различаться для одной и той же позиции
        if (cdp_x != other.cdp_x) return cdp_x < other.cdp_x;
        return cdp_y < other.cdp_y;
    }
};

#endif // BASETYPES_H
