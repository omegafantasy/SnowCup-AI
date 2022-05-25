#include <cmath>
#include <exception>
#include <iostream>

#include "egg_sdk.h"
#include "nlohmann/json.hpp"
#include "schema.h"
#include "stream_helper.h"

/*
 * Contestants don't actually have to look at the contents of this file.
 */
namespace thuai {
PlayerStatus GameState::get_player(int player_id) {
    auto player = m_teams[player_id / PLAYER_PER_TEAM][player_id % PLAYER_PER_TEAM].get<PlayerStatus>();
    player.holding = -1;
    for (int i = 0; i < EGG_COUNT; i++) {
        if (get_egg(i).holder == player_id) {
            player.holding = i;
            break;
        }
    }
    return player;
}

PlayerStatus GameState::get_player(Team team_id, int player_id_in_team) {
    auto player = m_teams[(int)team_id][player_id_in_team].get<PlayerStatus>();
    player.holding = -1;
    for (int i = 0; i < EGG_COUNT; i++) {
        if (get_egg(i).holder == int(team_id) * PLAYER_PER_TEAM + player_id_in_team) {
            player.holding = i;
            break;
        }
    }
    return player;
}

EggStatus GameState::get_egg(int egg_id) { return m_eggs[egg_id].get<EggStatus>(); }

Team GameState::current_team() { return m_current_team; }

void GameState::refresh_reply() {
    m_reply = json({{"state", m_state}, {"actions", {json(), json(), json(), json()}}});
    for (int i = 0; i < 4; i++) {
        auto player = get_player(m_current_team, i);
        m_reply["actions"][i]["action"] = player.status;
        m_reply["actions"][i]["facing"] = player.facing;
    }
}

void GameState::set_status_of_player(int player_id_in_team, PlayerMovement status) {
    if (status == PlayerMovement::SLIPPED) { // this makes no sense
        throw std::out_of_range(
            "manually setting status to slipped is not allowed, this can only be done by the simulator");
    }
    m_reply["actions"][player_id_in_team % 4]["action"] = status;
}

void GameState::set_facing_of_player(int player_id_in_team, Vec2D facing) {
    double len = std::sqrt(facing.x * facing.x + facing.y * facing.y);
    if (len < 1e-7) {
        facing.x = facing.y = .0;
    } else {
        facing.x /= len;
        facing.y /= len;
    }

    auto &j = m_reply["actions"][player_id_in_team % 4]["facing"];
    j = {{"x", facing.x}, {"y", facing.y}};
}

void GameState::try_grab_egg(int player_id_in_team, int egg_id) {
    m_reply["actions"][player_id_in_team % 4]["grab"] = egg_id;
}

void GameState::try_drop_egg(int player_id_in_team, double radian) {
    m_reply["actions"][player_id_in_team % 4]["drop"] = radian;
}

void GameState::_run() {
    static bool is_running = false;
    if (is_running)
        return;
    else
        is_running = true; // disable re-entry

    using json = nlohmann::json;

    json game_logic_data;
    while (std::cin >> game_logic_data) {
        // state, team, eggs, teams
        m_state = game_logic_data["state"];
        m_current_team = static_cast<Team>((int)game_logic_data["team"]);
        m_eggs = game_logic_data["eggs"];
        m_teams = game_logic_data["teams"];

        // get the modification
        refresh_reply();
        update();

        // send the reply
        write_to_judger(m_reply);
    }
}
} // namespace thuai

int main(void) {
    using json = nlohmann::json;
    using namespace thuai;

    GameState::instance()._run();

    return 0;
}
