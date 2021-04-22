#include <math.h> 
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

#include "message.hpp"

/*** message_t ***/

// Output stream
ostream& operator<< (ostream& os, const message_t& msg) {
    string result = "(p_id:" + to_string(msg.particle_id) + "): [";
    result += VectorUtils::get_string<float>(msg.data, true);
    os << result + ", is_ri: " << (msg.is_ri ? "true" : "false") << "]";
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
    string result = "(p_id:" + to_string(msg.particle_id) + "): [";
    result += VectorUtils::get_string<float>(msg.data, true);
    os << result + ", is_ri: " << (msg.is_ri ? "true" : "false") << "]";
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