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
    message_t () : data({}), particle_ids({}), purpose("n/a") {}
    message_t (vector<float> i_data) : data(i_data), particle_ids({}), purpose("n/a") {}
    message_t (vector<float> i_data, vector<int> i_particle_ids) : data(i_data), particle_ids(i_particle_ids), purpose("n/a") {}
    message_t (vector<float> i_data, vector<int> i_particle_ids, string i_purpose) : data(i_data), particle_ids(i_particle_ids), purpose(i_purpose) {}

    vector<float> data;
    vector<int> particle_ids;
    string purpose;
};

struct tracker_message_t : message_t {
    tracker_message_t () {}
    tracker_message_t (vector<float> i_data, vector<int> i_particle_ids, vector<int> i_subV_ids) :
            message_t(i_data, i_particle_ids), subV_ids(i_subV_ids) {}
    tracker_message_t (message_t msg, vector<int> i_subV_ids) :
            message_t(msg.data, msg.particle_ids, msg.purpose), subV_ids(i_subV_ids) {}

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