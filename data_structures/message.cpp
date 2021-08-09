#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "message.hpp"

/*** message_t ***/

// Output stream
ostream& operator<< (ostream& os, const message_t& msg) {
    string result = "(p_ids:" + VectorUtils::get_string<int>(msg.particle_ids) + "): [";
    result += VectorUtils::get_string<float>(msg.data, true);
    os << result + ", type: " << msg.purpose << "]";
    if (msg.positions.size() > 0) {
        os << ", pos: [";
        for (const auto& [key, val] : msg.positions) {
            os << "[id: " + to_string(key) + ": " + VectorUtils::get_string<float>(val, true) + "]";
        }
        os << "]";
    }
    return os;
}

// Input stream
istream& operator>> (istream& is, message_t& msg) {
    for (unsigned int i = 0; i < msg.data.size(); ++i) {
        is >> msg.data[i];
    }
    //is >> msg.data;
    return is;
}

/*** tracker_message_t ***/

// Output stream
ostream& operator<< (ostream& os, const tracker_message_t& msg) {
    string result = "(p_ids:" + VectorUtils::get_string<int>(msg.particle_ids) + "): [";
    result += VectorUtils::get_string<float>(msg.data, true);
    os << result + ", type: " << msg.purpose << "]";
    return os;
}

// Input stream
istream& operator>> (istream& is, tracker_message_t& msg) {
    for (unsigned int i = 0; i < msg.data.size(); ++i) {
        is >> msg.data[i];
    }
    //is >> msg.data;
    return is;
}

/*** collision_message_t ***/

// Output stream
ostream& operator<< (ostream& os, const collision_message_t& msg) {
    string result = "";
    for (const auto& [key, val] : msg.positions) {
        result += "[(p_id:" + to_string(key) + "): " + VectorUtils::get_string<float>(val, true) + "]";
    }
    os << result;
    return os;
}

// Input stream
istream& operator>> (istream& is, collision_message_t& msg) {
    // incomplete
    return is;
}