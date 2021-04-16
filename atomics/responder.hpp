#ifndef RESPONDER_HPP
#define RESPONDER_HPP

/*
Current functionality:
- Relay impulse messages from the impulse_in port through the responder_out port.

Notes:
- Positions in this module are not guarenteed to be up to date if the RI module is active since the
  responder cannot calculate positions on its own and subV does not report back to this module.
*/

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <stdlib.h>
#include <vector>
#include <random>
#include <math.h>
#include <map>
#include <nlohmann/json.hpp>

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct Responder_defs {
    //struct transition_in : public in_port<___> {};
    struct impulse_in : public in_port<message_t> {};
    struct collision_in : public in_port<collision_message_t> {};
    //struct attachment_out : public out_port<___> {};  // not applicable for now (tethering)
    //struct detachment_out : public out_port<___> {};  // not applicable for now (tethering)
    //struct impulse_out : public out_port<message_t> {};  // unsure of purpose (message type may be wrong)
    //struct loading_out : public out_port<___> {};
    //struct restitution_out : public out_port<___> {};
    struct response_out : public out_port<message_t> {};
};

template<typename TIME> class Responder {
    public:
        // temporary assignments
        //

        // ports definition
        /*
        using input_ports = tuple<typename Responder_defs::transition_in,
                                  typename Responder_defs::impulse_in,
                                  typename Responder_defs::collision_in>;
        using output_ports = tuple<typename Responder_defs::attachment_out,
                                   typename Responder_defs::detachment_out,
                                   typename Responder_defs::impulse_out,
                                   typename Responder_defs::loading_out,
                                   typename Responder_defs::restitution_out,
                                   typename Responder_defs::response_out>;
        */
        using input_ports = tuple<typename Responder_defs::impulse_in,
                                  typename Responder_defs::collision_in>;
        using output_ports = tuple<typename Responder_defs::response_out>;

        struct state_type {
            // particle mass, position, velocity
            //map
            json particle_data;
            TIME next_internal;
            vector<message_t> messages;
        };
        state_type state;

        Responder () {
            //
        }

        Responder (json j) {
            if (DEBUG_RE) cout << "Responder constructor received JSON: " << j << endl;
            state.particle_data = j;
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_RE) cout << "resp internal transition called" << endl;
            // TODO: simulate particle decay (possibly in another module)
            state.messages.clear();  // can be done here since output is called before internal transition in Cadmium
            state.next_internal = numeric_limits<TIME>::infinity();  // the response module should not have internal events
            if (DEBUG_RE) cout << "resp internal transition finished" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_RE) cout << "resp external transition called" << endl;
            if (get_messages<typename Responder_defs::impulse_in>(mbs).size() > 1) {
                assert(false && "Responder received more than one concurrent message");
            }
            // Handle impulse messages from RI
            for (const auto &x : get_messages<typename Responder_defs::impulse_in>(mbs)) {
                if (DEBUG_RE) cout << "NOTE: responder received impulse: " << x << endl;

                if (x.particle_id == -1) continue;  // received an uninitialized message

                vector<float> newVelocity;
                float particle_mass = state.particle_data[to_string(x.particle_id)]["mass"];
                newVelocity = VectorUtils::element_op(state.particle_data[to_string(x.particle_id)]["velocity"],
                                                      VectorUtils::element_dist(x.data, particle_mass, VectorUtils::divide),
                                                      VectorUtils::add);

                state.particle_data[to_string(x.particle_id)]["velocity"] = newVelocity;  // record velocity change in resp particle model
                state.messages.push_back(message_t(newVelocity, x.particle_id, true));  // boolean sets is_ri flag
            }

            // Handle collision messages
            for (const auto &x : get_messages<typename Responder_defs::collision_in>(mbs)) {
                if (DEBUG_RE) cout << "resp external transition: responder received collision: " << x << endl;

                vector<int> p_ids;

                // update positions (must happen before impulses are calculated)
                // (also grab the particle IDs while we're at it)
                for (const auto& [key, val] : x.positions) {
                    state.particle_data[to_string(key)]["position"] = val;
                    p_ids.push_back(key);  // store particle IDs
                }

                if (p_ids.size() != 2) continue;  // received an uninitialized or malformed message

                // calculate impulse
                float p1_mass = state.particle_data[to_string(p_ids[0])]["mass"];
                float p2_mass = state.particle_data[to_string(p_ids[1])]["mass"];
                vector<float> impulse = calc_impulse(p_ids[0], p_ids[1], p1_mass, p2_mass);

                // calculate velocities
                vector<float> p1_vel = VectorUtils::element_op(state.particle_data[to_string(p_ids[0])]["velocity"],
                                                               VectorUtils::element_dist(impulse, p1_mass, VectorUtils::divide),
                                                               VectorUtils::add);
                vector<float> p2_vel = VectorUtils::element_op(state.particle_data[to_string(p_ids[1])]["velocity"],
                                                               VectorUtils::element_dist(impulse, p2_mass, VectorUtils::divide),
                                                               VectorUtils::subtract);

                if (DEBUG_RE) {
                    cout << "impulse calculation and application:" << endl;
                    cout << "| resp external transition: before setting calculated velocities: (p_id: " << p_ids[0] << ") " << VectorUtils::get_string<float>(state.particle_data[to_string(p_ids[0])]["velocity"]) << endl;
                    cout << "| resp external transition: before setting calculated velocities: (p_id: " << p_ids[1] << ") " << VectorUtils::get_string<float>(state.particle_data[to_string(p_ids[1])]["velocity"]) << endl;
                    cout << "| resp external transition: calculated impulse: " << VectorUtils::get_string<float>(impulse) << endl;
                    cout << "| resp external transition: new velocity: (p_id: " << p_ids[0] << ") " << VectorUtils::get_string<float>(p1_vel) << endl;
                    cout << "| resp external transition: new velocity: (p_id: " << p_ids[1] << ") " << VectorUtils::get_string<float>(p2_vel) << endl;
                }

                // set velocities
                state.particle_data[to_string(p_ids[0])]["velocity"] = p1_vel;
                state.particle_data[to_string(p_ids[1])]["velocity"] = p2_vel;

                // prepare messages
                state.messages.push_back(message_t(p1_vel, p_ids[0]));
                state.messages.push_back(message_t(p2_vel, p_ids[1]));
            }

            // set next internal
            state.next_internal = 0;

            if (DEBUG_RE) cout << "resp external transition finish" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_RE) cout << "resp confluence transition called" << endl;
            internal_transition();
            external_transition(e, move(mbs));
            if (DEBUG_RE) cout << "resp confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG_RE) cout << "resp output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<message_t> bag_port_out;
            bag_port_out = state.messages;
            get_messages<typename Responder_defs::response_out>(bags) = bag_port_out;
            if (DEBUG_RE) {
                cout << "resp output sending: ";
                for (auto i : state.messages) {
                    cout << "{" << i << "}";
                }
                cout << endl;
            }
            if (DEBUG_RE) cout << "resp output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG_RE) cout << "resp time advance called/returning" << endl;
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename Responder<TIME>::state_type& i) {
            if (DEBUG_RE) cout << "resp << called" << endl;
            string result = "particles: ";
            for (auto p_id = i.particle_data.begin(); p_id != i.particle_data.end(); ++p_id) {
                result += "[(p_id:" + p_id.key() + "): ";
                result += "pos[" + VectorUtils::get_string<float>(i.particle_data[p_id.key()]["position"]) + "], ";
                result += "vel[" + VectorUtils::get_string<float>(i.particle_data[p_id.key()]["velocity"]) + "]]";
            }
            result += ", velocity messages: ";
            for (auto message : i.messages) {
                result += "[(p_id:" + to_string(message.particle_id) + ") ";
                result += VectorUtils::get_string<float>(message.data) + "] ";
            }
            os << result;
            if (DEBUG_RE) cout << "resp << returning" << endl;
            return os;
        }

    private:

        // get the impulse after a collision
        // masses are arguments to allow for combined masses (loading)
        vector<float> calc_impulse (int p1_id, int p2_id, float p1_mass, float p2_mass) const {
            if (DEBUG_RE) cout << "resp calc_impulse called" << endl;
            vector<float> result;

            float c_restitute = 0.9;  // [0, 1]

            // relative velocity of p1 and p2
            vector<float> v_1_2 = VectorUtils::element_op(state.particle_data[to_string(p2_id)]["velocity"],
                                                          state.particle_data[to_string(p1_id)]["velocity"],
                                                          VectorUtils::subtract);

            // unit vector pointing from p1 to p2
            vector<float> u = VectorUtils::make_unit(VectorUtils::get_vect(state.particle_data[to_string(p2_id)]["position"],
                                                                           state.particle_data[to_string(p1_id)]["position"]));

            // calculate the component of the relative velocity which is along the vector u
            vector<float> v_u = VectorUtils::get_proj(v_1_2, u);

            // get impulse
            result = VectorUtils::element_dist(v_u, (1 / ((1 / p1_mass) + (1 / p2_mass))) * (1 + c_restitute), VectorUtils::multiply);

            if (DEBUG_RE) cout << "resp calc_impulse returning" << endl;
            return result;
        }
};

#endif