#ifndef EGG_SDK_H
#define EGG_SDK_H

#include "nlohmann/json.hpp"
#include "schema.h"
#include "singleton.h"

namespace thuai {
using json = nlohmann::json;
class GameState : public Singleton<GameState> {
    Team m_current_team;
    int m_state;
    nlohmann::json m_eggs, m_teams, m_reply;
    void refresh_reply();

  public:
    /* DO NOT CALL THIS. This is only used by internal functions. */
    void _run();

    /* The functions below are designed to be used by the contestant. */
    PlayerStatus get_player(int player_id);
    PlayerStatus get_player(Team team, int player_id_in_team);
    EggStatus get_egg(int egg_id);

    void set_status_of_player(int player_id_in_team, PlayerMovement status);
    void set_facing_of_player(int player_id_in_team, Vec2D facing);
    void try_grab_egg(int player_id_in_team, int egg);
    void try_drop_egg(int player_id_in_team, double radian);
    Team current_team();
};
} // namespace thuai

/*
 * Called every 0.1s
 * Should be used to update the state of players, etc.
 * You can get the current world from thuai::GameState::instance()
 * and then call set_... / get_... / try_... methods to interact with the world
 */
void update();

#endif
