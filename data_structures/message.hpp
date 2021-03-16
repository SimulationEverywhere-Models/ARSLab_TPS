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

istream& operator>> (istream& is, message_t& msg);

ostream& operator<< (ostream& os, const message_t& msg);

#endif