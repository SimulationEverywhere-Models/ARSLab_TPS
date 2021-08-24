#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "../utilities/vector_utils.hpp"  // vector functions

using namespace std;

/*
Used to transport impulses (typically only when sent from the random impulse module) or velocities.
Members:
- data: impulse or velocity
- particle_ids: vector of particles the data applies to
- purpose: informs on the purpose of the message (ex. "load", "rest", "ri")
*/
struct message_t {
    message_t () : data({}), particle_ids({}), purpose("n/a") {}
    message_t (vector<float> i_data) : data(i_data), particle_ids({}), purpose("n/a") {}
    message_t (vector<float> i_data, vector<int> i_particle_ids) : data(i_data), particle_ids(i_particle_ids), purpose("n/a") {}
    message_t (vector<float> i_data, vector<int> i_particle_ids, string i_purpose) : data(i_data), particle_ids(i_particle_ids), purpose(i_purpose) {}

    vector<float> data;  // impulse or vecocity
    vector<int> particle_ids;
    string purpose;
};

/*
Used to label standard message_t messages when being sent from the responder to the detector.
Members:
- subV_ids: the sub-volumes that the message is relevant to
*/
struct tracker_message_t : message_t {
    tracker_message_t () {}
    tracker_message_t (vector<float> i_data, vector<int> i_particle_ids, vector<int> i_subV_ids) :
            message_t(i_data, i_particle_ids), subV_ids(i_subV_ids) {}
    tracker_message_t (message_t msg, vector<int> i_subV_ids) :
            message_t(msg.data, msg.particle_ids, msg.purpose), subV_ids(i_subV_ids) {}

    vector<int> subV_ids;
};

/*
Used to inform the responder of collisions between particles.
Members:
- positions: the IDs and positions of the particles that are colliding (should always have exactly two elements)
*/
struct collision_message_t {
    collision_message_t () {}

    map<int, vector<float>> positions;
};

/*
Used to log the velocities and positions of particles that have been updated.
Members:
- subV_id: the sub-volume from which the message originates
- particle_id: the particle that the velocity and position belongs to
- vecolity: the particle's velocity
- position: the particle's position
*/
struct logging_message_t {
    logging_message_t () : subV_id(-1), particle_id(-1), velocity({}), position({}) {}
    logging_message_t (int i_subV_id, int i_particle_id, vector<float> i_velocity, vector<float> i_position, string i_purpose) :
        subV_id(i_subV_id), particle_id(i_particle_id), velocity(i_velocity), position(i_position), purpose(i_purpose) {}

    int subV_id;
    int particle_id;
    vector<float> velocity;
    vector<float> position;
    string purpose;
};

istream& operator>> (istream& is, message_t& msg);
ostream& operator<< (ostream& os, const message_t& msg);

istream& operator>> (istream& is, tracker_message_t& msg);
ostream& operator<< (ostream& os, const tracker_message_t& msg);

istream& operator>> (istream& is, collision_message_t& msg);
ostream& operator<< (ostream& os, const collision_message_t& msg);

istream& operator>> (istream& is, logging_message_t& msg);
ostream& operator<< (ostream& os, const logging_message_t& msg);

#endif