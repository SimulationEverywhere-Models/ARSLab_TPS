#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "../utilities/vector_utils.hpp"  // vector functions

using namespace std;

struct message_t {
    message_t () : data({}), particle_id(-1), is_ri(false) {}
    message_t (vector<float> i_data) : data(i_data), particle_id(-1), is_ri(false) {}
    message_t (vector<float> i_data, int i_particle_id) : data(i_data), particle_id(i_particle_id), is_ri(false) {}
    message_t (vector<float> i_data, int i_particle_id, bool i_is_ri) : data(i_data), particle_id(i_particle_id), is_ri(i_is_ri) {}

    int particle_id;
    vector<float> data;
    bool is_ri;  // used for debugging
};

struct tracker_message_t : message_t {
    tracker_message_t () {}
    tracker_message_t (vector<float> i_data, int i_particle_id, vector<int> i_subV_ids) :
            message_t(i_data, i_particle_id), subV_ids(i_subV_ids) {}
    tracker_message_t (message_t msg, vector<int> i_subV_ids) :
            message_t(msg.data, msg.particle_id, msg.is_ri), subV_ids(i_subV_ids) {}

    vector<int> subV_ids;
};

struct collision_message_t {
    collision_message_t () {}

    map<int, vector<float>> positions;
};

istream& operator>> (istream& is, message_t& msg);
ostream& operator<< (ostream& os, const message_t& msg);

istream& operator>> (istream& is, tracker_message_t& msg);
ostream& operator<< (ostream& os, const tracker_message_t& msg);

istream& operator>> (istream& is, collision_message_t& msg);
ostream& operator<< (ostream& os, const collision_message_t& msg);

#endif