#pragma once

#include <cassert>
#include <cmath>
#include <functional>

#include "typedefs.h"
template <typename T>
constexpr inline T loop_clamp(T v, T min, T max) {
    if(v < min)
        return v + max;
    if(v >= max)
        return v - max;
    return v;
}



struct coord_t {
    
    int x: 30 = 0;
    int y: 30 = 0;
    Dir::DirType dir : 4 = Dir::INVALID;
    constexpr coord_t() {};
    constexpr coord_t(int x, int y)
    {
        this->x = x;
        this->y = y;        
        _LoopClamp();
    }

    constexpr void _LoopClamp() {
        this->x = loop_clamp(this->x, 0, I_N_COLS);
        this->y = loop_clamp(this->y, 0, I_N_ROWS);
    }
    // Note: dir behavior is pretty haphazard, make sure to manually assign it before using it in relavent functions
    coord_t Add(const coord_t&other) const
    {
        return coord_t(x + other.x, y + other.y);
    }
    coord_t Sub(const coord_t &other) const
    {
        return coord_t(x - other.x, y - other.y);
    }
    void AddFrom(const coord_t &other)
    {
        x += other.x;
        y += other.y;
        _LoopClamp();
    }

    void SubFrom(const coord_t &other)
    {
        
        x -= other.x;
        y -= other.y;
        _LoopClamp();
    }


    coord_t operator+(const coord_t &other) const
    {
        return Add(other);
    }

    coord_t operator-(const coord_t &other) const
    {
        return Sub(other);
    }

        void operator+=(const coord_t &other)
    {
        AddFrom(other);
    }

    void operator-=(const coord_t &other)
    {
        SubFrom(other);
    }

    coord_t GetNegated() const
    {
        return coord_t(-x, -y);
    }


    coord_t operator-() const
    {
        return GetNegated();
    }

    coord_t Scale(int v) const
    {
        return coord_t(x * v, y * v);
    }

    coord_t operator*(int v) const
    {
        return Scale(v);
    }

    bool operator==(const coord_t& other) const {
        return this->x == other.x && this->y == other.y;
    }
};



// the 2 structs are essentially the same but its very important that the 2 types are not confused
struct code_pos_t {
    
    int x: 30 = 0;
    int y: 30 = 0;
    Dir::DirType dir : 4 = Dir::UP;
    code_pos_t() {};
    constexpr code_pos_t(int x, int y, Dir::DirType dir)
    {
        this->x = x;
        this->y = y;
        this->dir = dir;
        _LoopClamp();
    }

    constexpr void _LoopClamp() {
        this->x = loop_clamp(this->x, 0, I_N_COLS);
        this->y = loop_clamp(this->y, 0, I_N_ROWS);
    }
    // Note: dir behavior is pretty haphazard, make sure to manually assign it before using it in relavent functions
    code_pos_t Add(const code_pos_t &other) const
    {
        assert(dir == other.dir);
        return code_pos_t(x + other.x, y + other.y, dir);
    }
    code_pos_t Sub(const code_pos_t &other) const
    {
        assert(dir == other.dir);
        return code_pos_t(x - other.x, y - other.y, dir);
    }
    void AddFrom(const code_pos_t &other)
    {
        x += other.x;
        y += other.y;
        _LoopClamp();
    }

    void SubFrom(const code_pos_t &other)
    {
        
        x -= other.x;
        y -= other.y;
        _LoopClamp();
    }


        code_pos_t operator+(const code_pos_t &other) const
    {
        return Add(other);
    }

    code_pos_t operator-(const code_pos_t &other) const
    {
        return Sub(other);
    }

        void operator+=(const code_pos_t &other)
    {
        AddFrom(other);
    }

    void operator-=(const code_pos_t &other)
    {
        SubFrom(other);
    }

    code_pos_t GetNegated() const
    {
        return code_pos_t(-x, -y, dir);
    }


    code_pos_t operator-() const
    {
        return GetNegated();
    }

    code_pos_t Scale(int v) const
    {
        return code_pos_t(x * v, y * v, dir);
    }

    code_pos_t operator*(int v) const
    {
        return Scale(v);
    }

    bool operator==(const code_pos_t& other) const {
        return this->x == other.x && this->y == other.y && this->dir == other.dir;
    }
    coord_t to_coord() const {
        return coord_t(x, y);
    }
};


namespace std {
    template <>
    struct hash<code_pos_t> {
        size_t operator()(const code_pos_t& pos) const noexcept {
            uint64 combined_value = 0;
            combined_value |= static_cast<uint64_t>(pos.dir);
            combined_value |= (static_cast<uint64_t>(pos.y) << 4);
            combined_value |= (static_cast<uint64_t>(pos.x) << 34);
            return std::hash<uint64>{}(combined_value);
        }
    };
}