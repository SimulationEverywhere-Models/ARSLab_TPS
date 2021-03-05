#ifndef SPECIES_HPP
#define SPECIES_HPP

#include <map>
#include <string>

// This map should be used to initialize particle properties
// This can be done on initialization for the modules
// This will likely be moved to where initialization occurs

// Perhaps this data could be stored in a JSON file

// Properties
// - mass
// - radius

map<string, map<string, float>> species;  // species_id, <property_id, property>
species["default"] = map<string, float> {{"mass", 1}, {"radius", 5}};
species["small"] = map<string, float> {{"mass", 0.1}, {"radius", 1}};

#endif