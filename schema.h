#ifndef SCHEMA_H
#define SCHEMA_H

#include "nlohmann/json.hpp"
#include <string>

namespace thuai {
using json = nlohmann::json;

const size_t EGG_COUNT = 15, PLAYER_COUNT = 12, PLAYER_PER_TEAM = 4;

struct Vec2D {
    double x, y;
    Vec2D operator+(const Vec2D &t) const { return {x + t.x, y + t.y}; }
    Vec2D operator-(const Vec2D &t) const { return {x - t.x, y - t.y}; }
    Vec2D operator*(const double &a) const { return {x * a, y * a}; }
    Vec2D operator/(const double &a) const { return {x / a, y / a}; }
    inline Vec2D normalize() {
        double len = sqrt(x * x + y * y);
        if (len > 0) {
            x /= len;
            y /= len;
        }
        return {x, y};
    }
    inline double mag() { return sqrt(x * x + y * y); }
};

enum Team { RED, YELLOW, BLUE };

enum PlayerMovement { STOPPED, WALKING, RUNNING, SLIPPED };
NLOHMANN_JSON_SERIALIZE_ENUM(PlayerMovement, {
                                                 {STOPPED, "stopped"},
                                                 {WALKING, "walking"},
                                                 {RUNNING, "running"},
                                                 {SLIPPED, "slipped"},
                                             })

struct PlayerStatus {
    Vec2D position;        // coordinate of player
    Vec2D facing;          // which direction is the player facing?
    PlayerMovement status; // movement status
    int holding;           // id of the egg being held
    double endurance;      // current endurance
};

struct EggStatus {
    Vec2D position; // coordinate of egg
    int holder;     // id of the player holding the egg
    int score;      // score of egg
};

void to_json(json &j, const Vec2D &vec);
void from_json(const json &j, Vec2D &vec);

void to_json(json &j, const PlayerStatus &p);
void from_json(const json &j, PlayerStatus &p);

void to_json(json &j, const EggStatus &p);
void from_json(const json &j, EggStatus &p);
} // namespace thuai
#endif