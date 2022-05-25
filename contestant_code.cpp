#include "egg_sdk.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>
#define PI 3.1415927
#define min(x, y) ((x) < (y) ? x : y)
#define max(x, y) ((x) > (y) ? x : y)
using namespace thuai;

int nowround = 0;
int myteam;
int otherteam[2];

double egg_radius = 0.35;
double player_radius = 0.24;
int scores[3];

struct MyEgg {
    Vec2D pos;  // coordinate of egg
    int holder; // id of the player holding the egg
    int score;  // score of egg
    int id;
    int lastgrab;
    int time_since_grab;
    bool chosen;
    Vec2D speed;
    MyEgg() = default;
    MyEgg(EggStatus egg) : pos(egg.position), holder(egg.holder), score(egg.score) {}
    void update(EggStatus egg) {
        if (nowround == 0) {
            speed = {0, 0};
        } else {
            speed = egg.position * 10 - pos * 10;
        }
        pos = egg.position;
        holder = egg.holder;
        score = egg.score;
        chosen = false;
        if (egg.holder >= 0) {
            lastgrab = egg.holder;
            time_since_grab = 0;
        } else {
            time_since_grab++;
        }
    }
    void init(int _id) {
        id = _id;
        lastgrab = -1;
        time_since_grab = 0;
    }
    void print() { std::cerr << "egg" << id << ":" << time_since_grab << std::endl; }
};

struct MyPlayer {
    Vec2D pos;             // coordinate of player
    Vec2D facing;          // which direction is the player facing?
    PlayerMovement status; // movement status
    int holding;           // id of the egg being held
    double hp;             // current endurance
    int id;
    int team;
    int role;   // 0抢,1骚扰,2防守
    int target; //目标:
    double standard_speed;
    bool should_kick;
    Vec2D speed;
    MyPlayer() = default;
    MyPlayer(PlayerStatus player)
        : pos(player.position), facing(player.facing), status(player.status), holding(player.holding),
          hp(player.endurance) {}
    void update(PlayerStatus player) {
        if (nowround == 0) {
            speed = {0, 0};
        } else {
            speed = player.position * 10 - pos * 10;
        }
        pos = player.position;
        facing = player.facing;
        status = player.status;
        holding = player.holding;
        hp = player.endurance;
        target = -1;
    }
    void init(int _id, int _team) {
        id = _id;
        team = _team;
        role = 0;
        target = -1;
        if (_id % 4 == 0) {
            standard_speed = 2.4;
        } else {
            standard_speed = 2.0;
        }
        should_kick = true;
    }
    void print() { std::cerr << "playerhp:" << hp << std::endl; }
};

MyEgg eggs[15];
MyPlayer players[12];

Vec2D center = {0, 0};
Vec2D goal_entries[3];
Vec2D goal_ends[3];
Vec2D patrols[3];
Vec2D mids[3];
bool debug = false;
inline double distance(Vec2D pos1, Vec2D pos2) {
    return sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y));
}
inline double angle_between(Vec2D dir1, Vec2D dir2) {
    double bet = atan2(dir1.y, dir1.x) - atan2(dir2.y, dir2.x);
    if (bet > PI) {
        bet -= 2 * PI;
    } else if (bet < -PI) {
        bet += 2 * PI;
    }
    return abs(bet);
}
inline int dir_between(Vec2D dir1, Vec2D dir2) { //返回正代表dir1到dir2是顺时针
    double cross = dir1.x * dir2.y - dir1.y * dir2.x;
    if (cross > 0) {
        return -1;
    } else if (cross < 0) {
        return 1;
    }
    return 0;
}
inline double get_crowded_value(Vec2D pos, double mydis) {
    double value = 0;
    for (int i = 0; i < 12; i++) {
        double dis = distance(players[i].pos, pos);
        if (players[i].team != myteam) {
            if (dis < 2) {
                value += 1;
            } else if (dis < min(mydis - 1, 5)) {
                value += sqrt(2) / sqrt(dis);
            }
        } else {
            if (dis < 2) {
                value += 0.33;
            } else if (dis < min(mydis - 1, 5)) {
                value += 0.66 / dis;
            }
        }
    }
    return value;
}

inline bool egg_out_of_bound(Vec2D pos) {
    if (sqrt(pos.x * pos.x + pos.y * pos.y) < 20 - egg_radius) {
        return false;
    }
    if (pos.x + pos.y * sqrt(3) > 10 * sqrt(15) + egg_radius * 2 &&
        pos.x + pos.y * sqrt(3) < 10 * sqrt(15) + (5 + egg_radius) * 2 &&
        pos.x * sqrt(3) - pos.y < (5 - egg_radius) * 2 && pos.x * sqrt(3) - pos.y > (egg_radius - 5) * 2) {
        return false;
    }
    if (pos.x - pos.y * sqrt(3) > 10 * sqrt(15) + egg_radius * 2 &&
        pos.x - pos.y * sqrt(3) < 10 * sqrt(15) + (5 + egg_radius) * 2 &&
        pos.x * sqrt(3) + pos.y < (5 - egg_radius) * 2 && pos.x * sqrt(3) + pos.y > (egg_radius - 5) * 2) {
        return false;
    }
    if (pos.x > -25 + egg_radius && pos.x < -20 - egg_radius && pos.y < (5 - egg_radius) && pos.y > (egg_radius - 5)) {
        return false;
    }
    return true;
}
inline bool inside_buffer(Vec2D pos) { return (distance(pos, center) >= 18) && (distance(pos, center) <= 20); }
inline bool inside_goal(Vec2D pos, int team) {
    if (sqrt(pos.x * pos.x + pos.y * pos.y) < 20) {
        return false;
    }
    if (team == 0) {
        if (pos.x + pos.y * sqrt(3) >= 10 * sqrt(15)) {
            return true;
        }
    } else if (team == 1) {
        if (pos.x <= -5 * sqrt(15)) {
            return true;
        }
    } else {
        if (pos.x - pos.y * sqrt(3) >= 10 * sqrt(15)) {
            return true;
        }
    }
    return false;
}
inline int inside_goals(Vec2D pos) {
    if (sqrt(pos.x * pos.x + pos.y * pos.y) < 20) {
        return -1;
    }
    if (pos.x + pos.y * sqrt(3) >= 10 * sqrt(15)) {
        return 0;
    } else if (pos.x <= -5 * sqrt(15)) {
        return 1;
    } else if (pos.x - pos.y * sqrt(3) >= 10 * sqrt(15)) {
        return 2;
    }
    return -1;
}
inline int inside_regions(Vec2D pos) {
    if (pos.x + pos.y * sqrt(3) >= 28 &&
        ((pos.x * sqrt(3) - pos.y < 5 * 2 && pos.x * sqrt(3) - pos.y > -5 * 2) || !inside_buffer(pos))) {
        return 0;
    } else if (pos.x <= -14 && ((pos.y < 5 && pos.y > -5) || !inside_buffer(pos))) {
        return 1;
    } else if (pos.x - pos.y * sqrt(3) >= 28 &&
               ((pos.x * sqrt(3) + pos.y < 5 * 2 && pos.x * sqrt(3) + pos.y > -5 * 2) || !inside_buffer(pos))) {
        return 2;
    }
    return -1;
}
inline int inside_sectors(Vec2D pos) {
    if (pos.y >= 0 && pos.x * sqrt(3) + pos.y > 0) {
        return 0;
    } else if (pos.y < 0 && pos.x * sqrt(3) - pos.y >= 0) {
        return 2;
    }
    return 1;
}
inline Vec2D target_to_pos(int target) {
    if (target < 0) {
        return center;
    } else if (target < 3) {
        return mids[target % 3];
    } else if (target < 6) {
        return patrols[target % 3];
    } else {
        return goal_ends[target % 3];
    }
}
inline int pos_to_target(Vec2D pos) {
    if (distance(pos, center) < 0.1) {
        return -2;
    }
    for (int i = 0; i < 3; i++) {
        if (distance(pos, mids[i]) < 0.1) {
            return i;
        }
        if (distance(pos, patrols[i]) < 0.1) {
            return i + 3;
        }
        if (distance(pos, goal_ends[i]) < 0.1) {
            return i + 6;
        }
    }
    return -1;
}

inline bool valid_drop(int index, int eggid, double radian) { //是否能合法放下
    Vec2D nowpos = players[index].pos;
    Vec2D newpos = {nowpos.x + cos(radian) * (egg_radius + player_radius),
                    nowpos.y + sin(radian) * (egg_radius + player_radius)};
    for (int i = 0; i < 15; i++) {
        if (i != eggid) {
            if (distance(eggs[i].pos, newpos) <= egg_radius * 2) {
                std::cerr << "inv:player" << std::endl;
                return false;
            }
        }
    }
    for (int i = 0; i < 12; i++) {
        if (i != index) {
            if (distance(players[i].pos, newpos) <= egg_radius + player_radius) {
                std::cerr << "inv:egg" << std::endl;
                return false;
            }
        }
    }
    if (egg_out_of_bound(newpos)) {
        std::cerr << "inv:out" << std::endl;
        return false;
    }
    return true;
}

inline double trans(double val, double a1, double a2, double b1, double b2) { //将[a1,a2]的值转化为[b1,b2]的值
    if (val <= a1) {
        return b1;
    } else if (val >= a2) {
        return b2;
    }
    return b1 + (b2 - b1) * (val - a1) / (a2 - a1);
}

inline int egg_region_count(int team) {
    int score = 0;
    for (int i = 0; i < 15; i++) {
        if (inside_regions(eggs[i].pos) == team) {
            score += eggs[i].score;
        }
    }
    return score;
}
inline int egg_sector_count(int team) {
    int score = 0;
    for (int i = 0; i < 15; i++) {
        if (inside_sectors(eggs[i].pos) == team) {
            score += eggs[i].score;
        }
    }
    return score;
}

std::vector<int> get_near_players(double bound, int index) {
    std::vector<int> near_players;
    Vec2D nowpos = players[index].pos;
    for (int i = 0; i < 12; i++) {
        if (i != index && distance(players[i].pos, nowpos) < bound) {
            near_players.push_back(i);
        }
    }
    return near_players;
}

std::pair<int, double> find_nearest_valuable_egg(int index, bool accept_far, double maxdis) {
    Vec2D nowpos = players[index].pos;
    double dis = 100;
    int target = -1;
    Vec2D force = {0, 0};
    for (int i = 0; i < 15; i++) {
        int goal = inside_goals(eggs[i].pos);
        if (eggs[i].chosen) { //被其他人选了
            continue;
        }
        if (goal == myteam) { //已经在自己框内
            continue;
        }
        if (eggs[i].lastgrab == index && eggs[i].time_since_grab < 25) { //最近拿过
            continue;
        }
        if (eggs[i].holder >= 0 && eggs[i].holder / 4 == myteam) { //自己队的人拿着
            continue;
        }

        Vec2D dist = nowpos - eggs[i].pos;
        Vec2D speed = eggs[i].speed;
        double cur_dis = dist.mag();
        double speed_ratio = speed.mag() * cos(angle_between(speed, dist));
        cur_dis *= 5 / (max(-4, speed_ratio) + 5.0);
        if (goal >= 0) {
            cur_dis /= 2;
        }
        double crowded_value = get_crowded_value(eggs[i].pos, cur_dis);
        if (crowded_value > 2) {
            continue;
        } else {
            cur_dis += crowded_value * 2 - 1;
        }
        if (inside_buffer(eggs[i].pos) && inside_regions(eggs[i].pos) == -1) {
            cur_dis += (distance(eggs[i].pos, center) - 18) * 1.5;
        }

        cur_dis *= (double)(15) / eggs[i].score;
        force = force + (eggs[i].pos - nowpos).normalize() / cur_dis;
        if (cur_dis < dis && cur_dis < maxdis) {
            dis = cur_dis;
            target = i;
        }
    }
    if (!accept_far || dis < 4 || (force.x == 0 && force.y == 0)) {
        return std::make_pair(target, dis);
    } else {
        double bias_angle = 10;
        target = -1;
        for (int i = 0; i < 15; i++) {
            if (eggs[i].chosen) { //被其他人选了
                continue;
            }
            if (inside_goal(eggs[i].pos, myteam)) { //已经在自己框内
                continue;
            }
            if (eggs[i].lastgrab == index && eggs[i].time_since_grab < 25) { //最近拿过
                continue;
            }
            if (eggs[i].holder >= 0 && eggs[i].holder / 4 == myteam) { //自己队的人拿着
                continue;
            }
            double cur_dis = distance(nowpos, eggs[i].pos);
            double crowded_value = get_crowded_value(eggs[i].pos, cur_dis);
            if (crowded_value > 3.5) {
                continue;
            }
            double angle_bet = angle_between((eggs[i].pos - nowpos), force); //选夹角最小的
            if (angle_bet < bias_angle) {
                bias_angle = angle_bet;
                target = i;
            }
        }
        if (target == -1) {
            return std::make_pair(-1, 100);
        } else {
            return std::make_pair(target, distance(eggs[target].pos, nowpos));
        }
    }
}

bool near_angles[360];
void avoid_move(GameState &game, int index, std::vector<int> near_players, std::vector<int> mid_players,
                Vec2D targetdir) { // apf，吸引方向的力恒定,回避人
    Vec2D nowpos = players[index].pos;
    Vec2D gravity = targetdir.normalize();
    Vec2D total_force = {0, 0};
    for (int i = 0; i < mid_players.size(); i++) {
        double ratio;
        if (players[mid_players[i]].team == myteam) {
            ratio = 2;
        } else {
            ratio = 1.5;
        }
        Vec2D dir = (players[mid_players[i]].pos - nowpos).normalize();
        total_force = total_force - dir * ratio / distance(players[mid_players[i]].pos, nowpos); //排斥
    }
    if (players[index].speed.mag() >= players[index].standard_speed + 0.6) {
        game.set_status_of_player(index, PlayerMovement::STOPPED);
    } else if (players[index].hp < 1.6 || inside_buffer(nowpos) || players[index].holding != -1) {
        game.set_status_of_player(index, PlayerMovement::WALKING);
    } else {
        game.set_status_of_player(index, PlayerMovement::RUNNING);
    }
    if (angle_between(gravity, gravity + total_force) < PI / 4) {
        game.set_facing_of_player(index, gravity + total_force);
    } else {
        double deg = atan2(gravity.y, gravity.x);
        if (dir_between(gravity, total_force) > 0) {
            deg -= PI / 4;
        } else {
            deg += PI / 4;
        }
        game.set_facing_of_player(index, {cos(deg), sin(deg)});
    }
}

void attack_move(GameState &game, int index, std::vector<int> near_players, std::vector<int> mid_players,
                 Vec2D targetpos) { // apf，吸引方向力与距离正比，回避人
    Vec2D nowpos = players[index].pos;
    Vec2D gravity = targetpos - nowpos;
    Vec2D total_force = {0, 0};
    for (int i = 0; i < mid_players.size(); i++) {
        double ratio;
        if (players[mid_players[i]].team == myteam) {
            ratio = 4;
        } else {
            ratio = 3;
        }
        Vec2D dir = (players[mid_players[i]].pos - nowpos).normalize();
        total_force = total_force - dir * ratio / distance(players[mid_players[i]].pos, nowpos); //排斥
    }
    if (players[index].speed.mag() >= players[index].standard_speed + 0.6) {
        game.set_status_of_player(index, PlayerMovement::STOPPED);
    } else if (players[index].hp < 1.6 || inside_buffer(nowpos) || players[index].holding != -1) {
        game.set_status_of_player(index, PlayerMovement::WALKING);
    } else {
        game.set_status_of_player(index, PlayerMovement::RUNNING);
    }
    if (angle_between(gravity, gravity + total_force) < PI / 4) {
        game.set_facing_of_player(index, gravity + total_force);
    } else {
        double deg = atan2(gravity.y, gravity.x);
        if (dir_between(gravity, total_force) > 0) {
            deg -= PI / 4;
        } else {
            deg += PI / 4;
        }
        game.set_facing_of_player(index, {cos(deg), sin(deg)});
    }
}

void defence_move(GameState &game, int index, std::vector<int> near_players, std::vector<int> mid_players,
                  Vec2D targetpos) { // apf，吸引方向力与距离正比，主动撞人
    Vec2D nowpos = players[index].pos;
    Vec2D gravity = targetpos - nowpos;
    Vec2D total_force = {0, 0};
    for (int i = 0; i < mid_players.size(); i++) {
        double ratio;
        if (players[mid_players[i]].team == myteam) {
            ratio = 4;
        } else {
            ratio = -5;
        }
        Vec2D dir = (players[mid_players[i]].pos - nowpos).normalize();
        total_force = total_force - dir * ratio / distance(players[mid_players[i]].pos, nowpos); //排斥
    }
    if (players[index].speed.mag() >= players[index].standard_speed + 0.6) {
        game.set_status_of_player(index, PlayerMovement::STOPPED);
    } else if (players[index].hp < 4 || inside_buffer(nowpos) || players[index].holding != -1) {
        game.set_status_of_player(index, PlayerMovement::WALKING);
    } else {
        game.set_status_of_player(index, PlayerMovement::RUNNING);
    }
    if (angle_between(gravity, gravity + total_force) < PI / 4) {
        game.set_facing_of_player(index, gravity + total_force);
    } else {
        double deg = atan2(gravity.y, gravity.x);
        if (dir_between(gravity, total_force) > 0) {
            deg -= PI / 4;
        } else {
            deg += PI / 4;
        }
        game.set_facing_of_player(index, {cos(deg), sin(deg)});
    }
}

void drop_egg(GameState &game, int index) { //拿着蛋时候的行动
    std::vector<int> near_players = get_near_players(1.5, index);
    std::vector<int> mid_players = get_near_players(5, index);
    Vec2D nowpos = players[index].pos;
    if (inside_buffer(nowpos)) { //在缓冲区内，跑
        if (inside_regions(nowpos) == myteam && inside_goals(nowpos) == myteam) {
            avoid_move(game, index, near_players, mid_players, goal_ends[myteam] - nowpos);
        } else if (inside_regions(nowpos) == myteam) {
            avoid_move(game, index, near_players, mid_players, goal_ends[myteam] - nowpos);
        } else {
            avoid_move(game, index, near_players, mid_players, center - nowpos);
        }
    } else {
        bool enemy_near = false;
        memset(near_angles, 0, sizeof(near_angles));
        for (int i = 0; i < near_players.size(); i++) {
            if (players[near_players[i]].team != myteam) {
                enemy_near = true;
            }
            Vec2D dir = players[near_players[i]].pos - nowpos;
            int degree = (int)(atan2(dir.y, dir.x) * 180 / PI);
            for (int j = degree - 89; j <= degree + 89; j++) {
                near_angles[(j + 360) % 360] = 1;
            }
        }
        for (int i = 0; i < mid_players.size(); i++) {
            if ((players[mid_players[i]].team != myteam) &&
                (std::find(near_players.begin(), near_players.end(), mid_players[i]) == near_players.end())) {
                Vec2D dir = players[mid_players[i]].pos - nowpos;
                int degree = (int)(atan2(dir.y, dir.x) * 180 / PI);
                int bound = trans(distance(players[mid_players[i]].pos, nowpos), 1.5, 5, 89, 45);
                for (int j = degree - bound; j <= degree + bound; j++) {
                    near_angles[(j + 360) % 360] = 1;
                }
            }
        }
        int goal = inside_goals(nowpos);
        Vec2D targetdir;
        if (goal == -1) {
            targetdir = goal_ends[myteam] - nowpos;
        } else if (goal == myteam) {
            targetdir = goal_ends[myteam] - nowpos;
        } else {
            targetdir = goal_entries[myteam] - nowpos;
        }

        double target_degree;
        bool success = false;
        int deg = (int)(atan2(targetdir.y, targetdir.x) * 180 / PI);
        for (int i = 0; i < 45; i++) { //在目标的45度范围内
            if (near_angles[(deg - i + 360) % 360] == 0) {
                success = true;
                target_degree = (deg - i + 360) % 360;
                break;
            } else if (near_angles[(deg + i + 360) % 360] == 0) {
                success = true;
                target_degree = (deg + i + 360) % 360;
                break;
            }
        }
        if (success) { //扔蛋
            if (enemy_near) {
                game.set_facing_of_player(index, {cos(target_degree / 180 * PI), sin(target_degree / 180 * PI)});
                game.set_status_of_player(index, PlayerMovement::WALKING);
                if (valid_drop(index, players[index].holding, target_degree / 180 * PI)) {
                    players[index].should_kick = true;
                    game.try_drop_egg(index, target_degree / 180 * PI);
                }
            } else {
                game.set_facing_of_player(index, {cos(target_degree / 180 * PI), sin(target_degree / 180 * PI)});
                game.set_status_of_player(index, PlayerMovement::STOPPED);
                if (players[index].speed.x == 0 && players[index].speed.y == 0) {
                    if (valid_drop(index, players[index].holding, target_degree / 180 * PI)) {
                        players[index].should_kick = true;
                        game.try_drop_egg(index, target_degree / 180 * PI);
                    } else {
                        game.set_status_of_player(index, PlayerMovement::WALKING);
                    }
                }
            }
        } else { //跑
            int region = inside_regions(nowpos);
            Vec2D targetdir;
            if (region == -1) {
                targetdir = goal_ends[myteam] - nowpos;
            } else if (region == myteam) {
                targetdir = goal_ends[myteam] - nowpos;
            } else {
                targetdir = center - nowpos;
            }
            avoid_move(game, index, near_players, mid_players, targetdir);
        }
    }
}

int should_kick(int index) {
    if (!players[index].should_kick || inside_buffer(players[index].pos)) {
        return -1;
    }
    for (int i = 0; i < 15; i++) {
        if (eggs[i].lastgrab == index && eggs[i].time_since_grab <= 4) {
            return i;
        }
    }
    return -1;
}
void kick_egg(GameState &game, int index, int eggid) { //补踢一下蛋
    game.set_status_of_player(index, PlayerMovement::RUNNING);
    game.set_facing_of_player(index, eggs[eggid].pos - players[index].pos);
}

void go_to_egg(GameState &game, int index, int eggid) {
    Vec2D nowpos = players[index].pos;
    Vec2D target = eggs[eggid].pos + eggs[eggid].speed / 10;
    double last_speed = players[index].speed.mag() / 10;
    bool inside_buf = inside_buffer(nowpos);
    double walk_speed;
    double run_speed;
    double stop_speed;
    if (inside_buf) {
        walk_speed = 0.05;
        run_speed = 0.05;
        stop_speed = min(last_speed, 0.05) / 2;
    } else {
        if (index % 4 == 0) {
            walk_speed = 0.24;
            run_speed = 0.48;
            stop_speed = (min(last_speed, 0.24) + max(0, last_speed - 0.12)) / 2;
        } else {
            walk_speed = 0.2;
            run_speed = 0.4;
            stop_speed = (min(last_speed, 0.2) + max(0, last_speed - 0.12)) / 2;
        }
    }
    if ((eggs[eggid].speed.x != 0 || eggs[eggid].speed.y != 0) && eggs[eggid].holder == -1) { //在地上,考虑加速度
        target = target - eggs[eggid].speed.normalize() * 0.003;
    }
    double dis = distance(nowpos, target);
    Vec2D dir = target - nowpos;
    if (dis > egg_radius + player_radius + run_speed + 0.08) { //跑都不会撞
        game.set_facing_of_player(index, dir);
        if (players[index].speed.mag() >= players[index].standard_speed + 0.6) {
            game.set_status_of_player(index, PlayerMovement::STOPPED);
        } else if (players[index].hp < 1.6 || inside_buf) {
            game.set_status_of_player(index, PlayerMovement::WALKING);
        } else {
            game.set_status_of_player(index, PlayerMovement::RUNNING);
        }
    } else if (walk_speed > stop_speed) { //走更快
        if (dis > egg_radius + player_radius + walk_speed + 0.08) {
            game.set_facing_of_player(index, dir);
            game.set_status_of_player(index, PlayerMovement::WALKING);
        } else { //会撞
            double origin_rad = atan2(dir.y, dir.x);
            double target_dis = egg_radius + player_radius + 0.08;
            double ac = (dis * dis + walk_speed * walk_speed - target_dis * target_dis) / (2 * dis * walk_speed);
            if (abs(ac) <= 0.999 || inside_buf) { //走
                double bias_rad = acos(ac);
                if (distance(nowpos + Vec2D{cos(origin_rad + bias_rad), sin(origin_rad + bias_rad)} * walk_speed,
                             goal_ends[myteam]) <
                    distance(nowpos + Vec2D{cos(origin_rad - bias_rad), sin(origin_rad - bias_rad)} * walk_speed,
                             goal_ends[myteam])) {
                    origin_rad += bias_rad;
                } else {
                    origin_rad -= bias_rad;
                }
                game.set_status_of_player(index, PlayerMovement::WALKING);
                game.set_facing_of_player(index, {cos(origin_rad), sin(origin_rad)});
            } else { //跑
                ac = (dis * dis + run_speed * run_speed - target_dis * target_dis) / (2 * dis * run_speed);
                double bias_rad = acos(min(max(-0.999, ac), 0.999));
                if (distance(nowpos + Vec2D{cos(origin_rad + bias_rad), sin(origin_rad + bias_rad)} * run_speed,
                             goal_ends[myteam]) <
                    distance(nowpos + Vec2D{cos(origin_rad - bias_rad), sin(origin_rad - bias_rad)} * run_speed,
                             goal_ends[myteam])) {
                    origin_rad += bias_rad;
                } else {
                    origin_rad -= bias_rad;
                }
                game.set_status_of_player(index, PlayerMovement::RUNNING);
                game.set_facing_of_player(index, {cos(origin_rad), sin(origin_rad)});
            }
        }
    } else { //停更快
        if (dis > egg_radius + player_radius + stop_speed + 0.08) {
            game.set_facing_of_player(index, dir);
            game.set_status_of_player(index, PlayerMovement::STOPPED);
        } else { //会撞
            double origin_rad = atan2(dir.y, dir.x);
            double target_dis = egg_radius + player_radius + 0.08;
            double ac = (dis * dis + stop_speed * stop_speed - target_dis * target_dis) / (2 * dis * stop_speed);
            if (abs(ac) <= 0.999) { //停
                double bias_rad = acos(ac);
                if (distance(nowpos + Vec2D{cos(origin_rad + bias_rad), sin(origin_rad + bias_rad)} * stop_speed,
                             goal_ends[myteam]) <
                    distance(nowpos + Vec2D{cos(origin_rad - bias_rad), sin(origin_rad - bias_rad)} * stop_speed,
                             goal_ends[myteam])) {
                    origin_rad += bias_rad;
                } else {
                    origin_rad -= bias_rad;
                }
                game.set_status_of_player(index, PlayerMovement::STOPPED);
                game.set_facing_of_player(index, {cos(origin_rad), sin(origin_rad)});
            } else { //跑
                ac = (dis * dis + run_speed * run_speed - target_dis * target_dis) / (2 * dis * run_speed);
                double bias_rad = acos(min(max(-0.999, ac), 0.999));
                if (distance(nowpos + Vec2D{cos(origin_rad + bias_rad), sin(origin_rad + bias_rad)} * run_speed,
                             goal_ends[myteam]) <
                    distance(nowpos + Vec2D{cos(origin_rad - bias_rad), sin(origin_rad - bias_rad)} * run_speed,
                             goal_ends[myteam])) {
                    origin_rad += bias_rad;
                } else {
                    origin_rad -= bias_rad;
                }
                game.set_status_of_player(index, PlayerMovement::RUNNING);
                game.set_facing_of_player(index, {cos(origin_rad), sin(origin_rad)});
            }
        }
    }
}

void get_egg(GameState &game, int index) { //准备抢蛋时的行动
    std::vector<int> near_players = get_near_players(1.5, index);
    std::vector<int> mid_players = get_near_players(5, index);
    Vec2D nowpos = players[index].pos;

    auto res = find_nearest_valuable_egg(index, true, 100);
    int egg_id = res.first;
    double dis = res.second;
    if (egg_id >= 0) {
        dis = distance(eggs[egg_id].pos, nowpos);
    }
    if (egg_id == -1) { //没目标
        game.set_status_of_player(index, PlayerMovement::STOPPED);
        return;
    }
    if (dis > 2.5 && near_players.size() > 0) { //需要回避人
        avoid_move(game, index, near_players, mid_players, eggs[egg_id].pos - nowpos);
    } else {
        eggs[egg_id].chosen = true;
        if (dis < egg_radius + player_radius + 0.1) { //距离足够小，拿
            game.try_grab_egg(index, egg_id);
            game.set_status_of_player(index, PlayerMovement::STOPPED);
            if (inside_regions(nowpos) == myteam) {
                game.set_facing_of_player(index, goal_ends[myteam] - nowpos);
            } else {
                game.set_facing_of_player(index, goal_entries[myteam] - nowpos);
            }
        } else { //距离足够大,走向目标
            go_to_egg(game, index, egg_id);
        }
    }
}

void attack(GameState &game, int index) {
    std::vector<int> near_players = get_near_players(1.5, index);
    std::vector<int> mid_players = get_near_players(5, index);
    Vec2D nowpos = players[index].pos;

    auto res = find_nearest_valuable_egg(index, false, 8);
    int egg_id = res.first;
    double dis = res.second;
    if (egg_id >= 0) {
        dis = distance(eggs[egg_id].pos, nowpos);
    }
    if (egg_id == -1) { //没有近距离目标
        Vec2D target = target_to_pos(players[index].target);
        attack_move(game, index, near_players, mid_players, target);
        return;
    }
    if (dis > 3 && near_players.size() > 0) { //需要回避人
        avoid_move(game, index, near_players, mid_players, eggs[egg_id].pos - nowpos);
    } else {
        eggs[egg_id].chosen = true;
        if (dis < egg_radius + player_radius + 0.1) { //距离足够小，拿
            game.try_grab_egg(index, egg_id);
            game.set_status_of_player(index, PlayerMovement::STOPPED);
            if (inside_regions(nowpos) == myteam) {
                game.set_facing_of_player(index, goal_ends[myteam] - nowpos);
            } else {
                game.set_facing_of_player(index, goal_entries[myteam] - nowpos);
            }
        } else { //距离足够大,走向目标
            go_to_egg(game, index, egg_id);
        }
    }
}

void defence(GameState &game, int index) {
    std::vector<int> near_players = get_near_players(1.5, index);
    std::vector<int> mid_players = get_near_players(7, index);
    Vec2D nowpos = players[index].pos;
    std::pair<int, double> res;
    if (mid_players.size() == 0) {
        res = find_nearest_valuable_egg(index, false, 12);
    } else {
        res = find_nearest_valuable_egg(index, false, 8);
    }
    int egg_id = res.first;
    double dis = res.second;
    if (egg_id >= 0) {
        dis = distance(eggs[egg_id].pos, nowpos);
    }
    if (egg_id == -1) { //没有近距离目标
        Vec2D target = target_to_pos(players[index].target);
        defence_move(game, index, near_players, mid_players, target);
        return;
    }
    eggs[egg_id].chosen = true;
    if (dis < egg_radius + player_radius + 0.1) { //距离足够小，拿
        game.try_grab_egg(index, egg_id);
        game.set_status_of_player(index, PlayerMovement::STOPPED);
        if (inside_regions(nowpos) == myteam) {
            game.set_facing_of_player(index, goal_ends[myteam] - nowpos);
        } else {
            game.set_facing_of_player(index, goal_entries[myteam] - nowpos);
        }
    } else { //距离足够大,走向目标
        go_to_egg(game, index, egg_id);
    }
}

void analysis() {
    players[myteam * 4].role = 0;
    if (scores[myteam] > 100) {
        players[myteam * 4 + 1].role = players[myteam * 4 + 2].role = players[myteam * 4 + 3].role = 2;
        players[myteam * 4 + 3].target = pos_to_target(goal_ends[myteam]);
        players[myteam * 4 + 2].target = pos_to_target(patrols[myteam]);
        players[myteam * 4 + 1].target = pos_to_target(center);
        return;
    }
    if (scores[myteam] == 0 && nowround > 40) {
        players[myteam * 4 + 3].role = 0;
    } else if (scores[myteam] < 40) {
        players[myteam * 4 + 3].role = 2;
        players[myteam * 4 + 3].target = pos_to_target(mids[myteam]);
    } else {
        players[myteam * 4 + 3].role = 2;
        players[myteam * 4 + 3].target = pos_to_target(goal_ends[myteam]);
    }
    players[myteam * 4 + 1].role = players[myteam * 4 + 2].role = 1;
    if (egg_region_count(otherteam[0]) > 0) {
        players[myteam * 4 + 1].target = pos_to_target(goal_ends[otherteam[0]]);
    } else if (egg_sector_count(otherteam[0]) > 20) {
        players[myteam * 4 + 1].target = pos_to_target(patrols[otherteam[0]]);
    } else if (egg_sector_count(otherteam[0]) > 0) {
        players[myteam * 4 + 1].target = pos_to_target(mids[otherteam[0]]);
    } else {
        players[myteam * 4 + 2].role = 0;
    }
    if (egg_region_count(otherteam[1]) > 0) {
        players[myteam * 4 + 2].target = pos_to_target(goal_ends[otherteam[1]]);
    } else if (egg_sector_count(otherteam[1]) > 20) {
        players[myteam * 4 + 2].target = pos_to_target(patrols[otherteam[1]]);
    } else if (egg_sector_count(otherteam[1]) > 0) {
        players[myteam * 4 + 2].target = pos_to_target(mids[otherteam[1]]);
    } else {
        players[myteam * 4 + 2].role = 0;
    }
}

void update() {
    auto &game = GameState::instance();
    std::cerr << nowround << std::endl;
    //初始化
    if (nowround == 0) {
        srand(time(0));
        myteam = game.current_team();
        if (myteam == 0) {
            otherteam[0] = 2;
            otherteam[1] = 1;
        } else if (myteam == 1) {
            otherteam[0] = 0;
            otherteam[1] = 2;
        } else {
            otherteam[0] = 1;
            otherteam[1] = 0;
        }
        goal_entries[0] = {10, 10 * sqrt(3)};
        goal_entries[1] = {-20, 0};
        goal_entries[2] = {10, -10 * sqrt(3)};
        goal_ends[0] = {11.25, 11.25 * sqrt(3)};
        goal_ends[1] = {-22.5, 0};
        goal_ends[2] = {11.25, -11.25 * sqrt(3)};
        patrols[0] = {7.6, 7.6 * sqrt(3)};
        patrols[1] = {-15.2, 0};
        patrols[2] = {7.6, -7.6 * sqrt(3)};
        mids[0] = {5, 5 * sqrt(3)};
        mids[1] = {-10, 0};
        mids[2] = {5, -5 * sqrt(3)};

        for (int i = 0; i < 15; i++) {
            eggs[i].init(i);
        }
        for (int i = 0; i < 12; i++) {
            players[i].init(i, i / 4);
        }
    }

    //更新
    for (int i = 0; i < 15; i++) {
        eggs[i].update(game.get_egg(i));
    }
    for (int i = 0; i < 12; i++) {
        players[i].update(game.get_player(i));
    }

    //算分数
    for (int i = 0; i < 3; i++) {
        scores[i] = 0;
    }
    for (int i = 0; i < 15; i++) {
        if (eggs[i].holder == -1) {
            int goal = inside_goals(eggs[i].pos);
            if (goal >= 0) {
                scores[goal] += eggs[i].score;
            }
        }
    }

    //决定各个队员任务
    analysis();

    //行动
    for (int i = 0; i < 4; i++) {
        int index = myteam * 4 + i;
        if (players[index].status != SLIPPED) {
            if (players[index].holding != -1) { //拿着蛋
                drop_egg(game, index);
            } else {                            //没拿蛋
                if (players[index].role == 0) { //抢
                    int eggid = should_kick(index);
                    if (eggid >= 0) {
                        kick_egg(game, index, eggid);
                    } else {
                        get_egg(game, index);
                    }
                } else if (players[index].role == 1) { //骚扰
                    int eggid = should_kick(index);
                    if (eggid >= 0) {
                        kick_egg(game, index, eggid);
                    } else {
                        attack(game, index);
                    }
                } else { //防守
                    int eggid = should_kick(index);
                    if (eggid >= 0) {
                        kick_egg(game, index, eggid);
                    } else {
                        defence(game, index);
                    }
                }
            }
        } else {
            std::cerr << i << "slip" << std::endl;
        }
    }

    nowround++;
}