#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "message.hpp"

// Output stream
ostream& operator<<(ostream& os, const message_t& msg) {
    string result = "id:" + to_string(msg.particle_id) + " ";
    for (auto i : msg.data) {
        result += to_string(i) + " ";
    }
    os << result;
    return os;
}

// Input stream
istream& operator>> (istream& is, message_t& msg) {
    int dim = 3;
    for (int i = 0; i < dim; ++i) {
        is >> msg.data[i];
    }
    //is >> msg.data;
    return is;
}
