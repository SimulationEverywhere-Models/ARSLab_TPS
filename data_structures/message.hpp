#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct message_t {
    message_t () {}
    message_t (vector<float> i_data) : data(i_data), particle_id(-1) {}
    message_t (vector<float> i_data, int i_particle_id) : data(i_data), particle_id(i_particle_id), is_ri(false) {}

    int particle_id;
    vector<float> data;
    bool is_ri;
};

struct tracker_message_t : message_t {
    tracker_message_t () {}
    tracker_message_t (vector<float> i_data, int i_particle_id, vector<int> i_subV_id) :
            message_t(i_data, i_particle_id), subV_id(i_subV_id) {}
    tracker_message_t (message_t msg, vector<int> i_subV_id) :
            message_t(msg.data, msg.particle_id), subV_id(i_subV_id) {}

    vector<int> subV_ids;
};

template <typename TIME>
struct collision_message_t {
    collision_message_t () {}
    //collision_message_t (int i_p1_id, int i_p2_id, TIME i_time) {} :
    //        p1_id(i_p1_id), p2_id(i_p2_id), time(i_time), p1_pos({}), p2_pos({}) {}

    map<int, vector<float>> positions;
    TIME time;

    /*
    int p1_id;
    vector<float> p1_pos;  // updated position
    int p2_id;
    vector<float> p2_pos;  // updated position
    TIME time;
    */
};

istream& operator>> (istream& is, message_t& msg);
ostream& operator<< (ostream& os, const message_t& msg);

istream& operator>> (istream& is, tracker_message_t& msg);
ostream& operator<< (ostream& os, const tracker_message_t& msg);

template <typename TIME>
istream& operator>> (istream& is, collision_message_t<TIME>& msg);
template <typename TIME>
ostream& operator<< (ostream& os, const collision_message_t<TIME>& msg);

#endif