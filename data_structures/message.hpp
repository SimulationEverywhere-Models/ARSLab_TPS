#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct message_t {
    message_t () {}
    message_t (vector<float> i_data) : data(i_data), particle_id(-1) {}
    message_t (vector<float> i_data, int i_particle_id) : data(i_data), particle_id(i_particle_id) {}

    int particle_id;
    vector<float> data;
};

struct tracker_message_t : message_t {
    tracker_message_t () {}
    tracker_message_t (vector<float> i_data, int i_particle_id, int i_subV_id) :
            message_t(i_data, i_particle_id), subV_id(i_subV_id) {}
    tracker_message_t (message_t msg, int i_subV_id) :
            message_t(msg.data, msg.particle_id), subV_id(i_subV_id) {}

    int subV_id;
};

istream& operator>> (istream& is, message_t& msg);
ostream& operator<< (ostream& os, const message_t& msg);

istream& operator>> (istream& is, tracker_message_t& msg);
ostream& operator<< (ostream& os, const tracker_message_t& msg);

#endif