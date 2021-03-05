#ifndef IMPULSE_MESSAGE_HPP
#define IMPULSE_MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct impulse_message_t {
    impulse_message_t () {}
    impulse_message_t (vector<float> i_impulse) : impulse(i_impulse) {}

    int particle_id;
    vector<float> impulse;
};

istream& operator>> (istream& is, impulse_message_t& msg);

ostream& operator<< (ostream& os, const impulse_message_t& msg);

#endif