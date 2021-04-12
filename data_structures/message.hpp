#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct message_t {
    message_t () : data({}), particle_id(-1) {}
    message_t (vector<float> i_data) : data(i_data), particle_id(-1) {}
    message_t (vector<float> i_data, int i_particle_id) : data(i_data), particle_id(i_particle_id) {}

    int particle_id;
    vector<float> data;
};

struct tracker_message_t : message_t {
    tracker_message_t () {}
    tracker_message_t (vector<float> i_data, int i_particle_id, vector<int> i_subV_ids) :
            message_t(i_data, i_particle_id), subV_ids(i_subV_ids) {}
    tracker_message_t (message_t msg, vector<int> i_subV_ids) :
            message_t(msg.data, msg.particle_id), subV_ids(i_subV_ids) {}

    vector<int> subV_ids;
};

template <typename TIME>
struct collision_message_t {
    collision_message_t () {}

    map<int, vector<float>> positions;
};

istream& operator>> (istream& is, message_t& msg);
ostream& operator<< (ostream& os, const message_t& msg);

istream& operator>> (istream& is, tracker_message_t& msg);
ostream& operator<< (ostream& os, const tracker_message_t& msg);

// Output stream
template <typename TIME>
ostream& operator<< (ostream& os, const collision_message_t<TIME>& msg) {
    os << "collision_message_t placeholder";
    return os;
}

// Input stream
template <typename TIME>
istream& operator>> (istream& is, collision_message_t<TIME>& msg) {
    // incomplete
    return is;
}

#endif