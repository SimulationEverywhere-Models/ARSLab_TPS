#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "message.hpp"

/*** message_t ***/

// Output stream
ostream& operator<< (ostream& os, const message_t& msg) {
    string result = "id:" + to_string(msg.particle_id) + " [";
    for (auto i : msg.data) {
        result += to_string(i) + " ";
    }
    os << result + "]";
    return os;
}

// Input stream
istream& operator>> (istream& is, message_t& msg) {
    for (int i = 0; i < msg.data.size(); ++i) {
        is >> msg.data[i];
    }
    //is >> msg.data;
    return is;
}

/*** tracker_message_t ***/

// Output stream
ostream& operator<< (ostream& os, const tracker_message_t& msg) {
    string result = "id:" + to_string(msg.particle_id) + " [";
    for (auto i : msg.data) {
        result += to_string(i) + " ";
    }
    os << result + "]";
    return os;
}

// Input stream
istream& operator>> (istream& is, tracker_message_t& msg) {
    for (int i = 0; i < msg.data.size(); ++i) {
        is >> msg.data[i];
    }
    //is >> msg.data;
    return is;
}

/*** collision_message_t ***/

// Defined in header as it is a template