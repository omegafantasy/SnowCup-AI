#include "schema.h"

namespace thuai {
void to_json(json &j, const Vec2D &vec) { j = json{{"x", vec.x}, {"y", vec.y}}; }
void from_json(const json &j, Vec2D &vec) {
    j.at("x").get_to(vec.x);
    j.at("y").get_to(vec.y);
}

void to_json(json &j, const PlayerStatus &p) {
    j = json{{"position", p.position}, {"facing", p.facing}, {"status", p.status}};
}

void from_json(const json &j, PlayerStatus &p) {
    j.at("position").get_to(p.position);
    j.at("facing").get_to(p.facing);
    j.at("status").get_to(p.status);
    try {
        j.at("endurance").get_to(p.endurance);
    } catch (json::out_of_range &e) {
        p.endurance = -1.0; // backward compatibility
    }
}

void to_json(json &j, const EggStatus &p) {
    j = json{{"position", p.position}, {"holder", p.holder}, {"score", p.score}};
}

void from_json(const json &j, EggStatus &p) {
    j.at("position").get_to(p.position);
    j.at("holder").get_to(p.holder);
    j.at("score").get_to(p.score);
}
} // namespace thuai