#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "impulse_message.hpp"

// Output stream
ostream& operator<<(ostream& os, const impulse_message_t& msg) {
    string result = "id:" + to_string(msg.particle_id) + " ";
    for (auto i : msg.impulse) {
        result += to_string(i) + " ";
    }
    os << result;
    return os;
}

// Input stream
istream& operator>> (istream& is, impulse_message_t& msg) {
    int dim = 3;
    for (int i = 0; i < dim; ++i) {
        is >> msg.impulse[i];
    }
    //is >> msg.impulse;
    return is;
}
