// Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>

// Time class header
#include <NDTime.hpp>

// Message structures
#include "../data_structures/message.hpp"

// Atomic model headers
#include <cadmium/basic_model/pdevs/iestream.hpp>  // atomic model for inputs
#include "../atomics/random_impulse.hpp"
#include "../atomics/responder.hpp"
#include "../atomics/tracker.hpp"
#include "../atomics/subV.hpp"

// C++ libraries
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>  // Used to parse JSON files and manipulate the resulting data
#include <fstream>  // Used to read from files
#include <map>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using json = nlohmann::json;
using TIME = float;

/*** Forward References ***/
json prepParticlesJSON (json&, vector<string>, vector<string>);

/*** Define input ports for coupled models ***/
struct detector_response_in : public in_port<message_t>{};
struct lattice_response_in : public in_port<tracker_message_t>{};

/*** Define output ports for coupled models ***/
struct top_out : public out_port<message_t>{};
struct lattice_collision_out : public out_port<collision_message_t>{};
struct detector_collision_out : public out_port<collision_message_t>{};

int main () {
    // Get initial particle information prepared
    ifstream ifs("../input/config.json");
    json configJson = json::parse(ifs);
    int dim = configJson["particles"][configJson["particles"].begin().key()]["position"].size();  // get number of dimensions
    json ri_particles = prepParticlesJSON(configJson, {}, {"mass", "tau", "shape", "mean"});
    json re_particles = prepParticlesJSON(configJson, {"position", "velocity"}, {"mass"});
    json de_particles = prepParticlesJSON(configJson, {"position", "velocity"}, {"radius"});

    /*** RI atomic model instantiation ***/
    shared_ptr<dynamic::modeling::model> random_impulse;
    random_impulse = dynamic::translate::make_dynamic_atomic_model<RandomImpulse, TIME, json, int>
            ("random_impulse", move(ri_particles), move(dim));

    /*** Responder atomic model instantiation ***/
    shared_ptr<dynamic::modeling::model> responder;
    responder = dynamic::translate::make_dynamic_atomic_model<Responder, TIME, json>("responder", move(re_particles));

    /*** Tracker atomic model instantiation ***/
    shared_ptr<dynamic::modeling::model> tracker;
    tracker = dynamic::translate::make_dynamic_atomic_model<Tracker, TIME>("tracker");

    /*** SubV atomimc model instantiation ***/
    shared_ptr<dynamic::modeling::model> subV;
    subV = dynamic::translate::make_dynamic_atomic_model<SubV, TIME, json>("subV", move(de_particles));

    /*** LATTICE COUPLED MODEL ***/
    // TODO: (2nd iteration) add several subV into a lattice
    dynamic::modeling::Ports iports_lattice;
    iports_lattice = {typeid(lattice_response_in)};
    dynamic::modeling::Ports oports_lattice;
    oports_lattice = {typeid(lattice_collision_out)};
    dynamic::modeling::Models submodels_lattice;
    submodels_lattice = {subV};
    dynamic::modeling::EICs eics_lattice;  // external input couplings
    eics_lattice = {
        dynamic::translate::make_EIC<lattice_response_in, SubV_defs::response_in>("subV")  // lattice -> subV
    };
    dynamic::modeling::EOCs eocs_lattice;
    eocs_lattice = {
        dynamic::translate::make_EOC<SubV_defs::collision_out, lattice_collision_out>("subV"),  // subV -> lattice
    };
    dynamic::modeling::ICs ics_lattice;
    ics_lattice = {};  // (2nd iteration) will have several subV connections
    shared_ptr<dynamic::modeling::coupled<TIME>> lattice;
    lattice = make_shared<dynamic::modeling::coupled<TIME>>(
        "lattice", submodels_lattice, iports_lattice, oports_lattice, eics_lattice, eocs_lattice, ics_lattice
    );

    /*** DETECTOR COUPLED MODEL ***/
    dynamic::modeling::Ports iports_detector;
    iports_detector = {typeid(detector_response_in)};
    dynamic::modeling::Ports oports_detector;
    oports_detector = {typeid(detector_collision_out)};
    dynamic::modeling::Models submodels_detector;
    submodels_detector = {tracker, lattice};
    dynamic::modeling::EICs eics_detector;  // external input couplings
    eics_detector = {
        dynamic::translate::make_EIC<detector_response_in, Tracker_defs::response_in>("tracker")  // detector -> tracker
    };
    dynamic::modeling::EOCs eocs_detector;
    eocs_detector = {
        dynamic::translate::make_EOC<lattice_collision_out, detector_collision_out>("lattice"),  // lattice -> detector
    };
    dynamic::modeling::ICs ics_detector;
    ics_detector = {
        dynamic::translate::make_IC<Tracker_defs::response_out, lattice_response_in>("tracker", "lattice")
    };
    shared_ptr<dynamic::modeling::coupled<TIME>> detector;
    detector = make_shared<dynamic::modeling::coupled<TIME>>(
        "detector", submodels_detector, iports_detector, oports_detector, eics_detector, eocs_detector, ics_detector
    );

    /*** TOP MODEL ***/
    dynamic::modeling::Ports iports_TOP;
    iports_TOP = {};
    dynamic::modeling::Ports oports_TOP;
    oports_TOP = {typeid(top_out)};
    dynamic::modeling::Models submodels_TOP;
    submodels_TOP = {random_impulse, responder, detector};
    dynamic::modeling::EICs eics_TOP;  // external input couplings
    eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP;
    eocs_TOP = {
        dynamic::translate::make_EOC<Responder_defs::response_out, top_out>("responder")
    };
    dynamic::modeling::ICs ics_TOP;
    ics_TOP = {
        dynamic::translate::make_IC<RandomImpulse_defs::impulse_out, Responder_defs::impulse_in>("random_impulse", "responder"),
        dynamic::translate::make_IC<Responder_defs::response_out, detector_response_in>("responder", "detector"),
        dynamic::translate::make_IC<detector_collision_out, Responder_defs::collision_in>("detector", "responder"),
    };
    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*** Loggers ***/
    static ofstream out_messages("../simulation_results/iter_1_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){
            return out_messages;
        }
    };
    static ofstream out_state("../simulation_results/iter_1_test_output_state.txt");
    struct oss_sink_state{
        static ostream& sink(){
            return out_state;
        }
    };

    using state=logger::logger<logger::logger_state, dynamic::logger::formatter<TIME>, oss_sink_state>;
    using log_messages=logger::logger<logger::logger_messages, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_mes=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_state>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;

    /*** Runner call ***/
    dynamic::engine::runner<TIME, logger_top> r(TOP, {0});
    //r.run_until(NDTime("00:05:00:000"));
    r.run_until(TIME(5));
    return 0;
}

// args: config JSON, necessary particle element names, necessary species element names
// return: JSON of format {id : {elements}, id : {elements}, ...}
json prepParticlesJSON (json& j, vector<string> particle_elements, vector<string> species_elements) {
    json result;
    string currSpecies;
    for (auto it = j["particles"].begin(); it != j["particles"].end(); ++it) {
        // prune particle information
        for (auto element : particle_elements) {
            result[it.key()][element] = j["particles"][it.key()][element];
        }
        // add necessary species information
        for (auto element : species_elements) {
            currSpecies = j["particles"][it.key()]["species"];
            result[it.key()][element] = j["species"][currSpecies][element];
        }
    }
    return result;
}