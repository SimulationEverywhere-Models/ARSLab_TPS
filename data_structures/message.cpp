#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "message.hpp"

/*** message_t ***/

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
    for (int i = 0; i < msg.data.size(); ++i) {
        is >> msg.data[i];
    }
    //is >> msg.data;
    return is;
}

/*** tracker_message_t ***/

// Output stream
ostream& operator<<(ostream& os, const tracker_message_t& msg) {
    string result = "id:" + to_string(msg.particle_id) + " ";
    for (auto i : msg.data) {
        result += to_string(i) + " ";
    }
    os << result;
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

// Output stream
template <typename TIME>
ostream& operator<<(ostream& os, const collision_message_t<TIME>& msg) {
    //os << "p1_id:" << msg.p1_id
    //   << ", p2_id:" << msg.p2_id
    //   << ", delta_t:" << msg.time;
    os << "collision_message_t placeholder";
    return os;
}

// Input stream
template <typename TIME>
istream& operator>> (istream& is, collision_message_t<TIME>& msg) {
    //is >> msg.p1_id;
    //is >> msg.p2_id;
    //is >> msg.time;
    return is;
}