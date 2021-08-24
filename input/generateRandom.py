# Thomas Roller
# Carleton University - ARSLab
# A script to create random particles for the Tethered Particle System

import random
import json
import sys

# Random number between minimum and maximum
# minumum: float
# maximum: float
# return: float
def randomNum (minimum, maximum):
    return (random.random() * (maximum - minimum)) + minimum

# Organize provided data into a dictionary
# position: list
# velocity: list
# species: string
# return: dict
def createParticle (position, velocity, species):
    return {
        "position" : position,
        "velocity" : velocity,
         "species" : species
    }

# Distance between position p1 and position p2
# p1: list
# p2: list
# return: float
def distance (p1, p2):
    if (len(p1) != len(p2)):
        return -1

    result = 0
    for i in range(0, len(p1)):
        result += (p2[i] - p1[i]) ** 2
    return result ** 0.5

# Check that a new particle does not overlap existing particles
# newPos: list
# newSpecies: string
# existingParticles: dict (result["particles"])
# allSpecies: dict
# return: bool
def isValidPos (newPos, newSpecies, existingParticles, allSpecies):
    for pID in existingParticles:
        dist = distance(existingParticles[pID]["position"], newPos)
        r1 = allSpecies[existingParticles[pID]["species"]]["radius"]
        r2 = allSpecies[newSpecies]["radius"]
        if (dist < r1 + r2):
            return False
    return True

# create a list of random values between minumum and maximum of length dim
# dim: int
# minumum: float
# maximum: float
# return: list
def genList (dim, minimum, maximum):
    return [randomNum(minimum, maximum) for i in range(0, dim)]

# generate particles based on provided arguments
# dim: int
# maxParticles: int
# allSpecies: dict
# minPos: float
# maxPos: float
# minVel: float
# maxVel: float
# maxAttempts: int
# return: dict
def genParticles (dim, maxParticles, allSpecies, minPos, maxPos, minVel, maxVel, maxAttempts=10):
    result = {}
    for i in range (1, maxParticles + 1):
        failures = 0
        newPosition = None
        newSpecies = None
        for j in range(0, maxAttempts):
            newPosition = genList(dim, minPos, maxPos)
            newSpecies = random.choice(list(allSpecies))
            if (isValidPos(newPosition, newSpecies, result, allSpecies)):
                result[str(i)] = createParticle(newPosition, genList(dim, minVel, maxVel), newSpecies)
                break
            print(f"NOTE: Invalid position calculated (conflict for pID: {i})")
            failures = j
        if (failures >= maxAttempts - 1):
            print(f"WARNING: Unable to find a valid spot after {maxAttempts} tries for pID: {i}")
            break
    return result

# Check to see if the user provided a configuration file
configTemplate = "config_template.json"
if (len(sys.argv) == 2):
    configTemplate = sys.argv[1]

# Import and store the configuration template
result = None
with open("config_template.json", "r") as f:
    result = json.loads(f.read())

# Note the possible particle species
species = result["species"]

# Configure and execute the script
dim = 2  # the dimension that the particles exist in
maxParticles = 10  # number of particle that the script will attempt to create
posRange = [-15, 15]  # each component of a particle's position will be between these values
velRange = [-3, 3]  # each component of a particle's velocity will be between these values
result["particles"] = genParticles(dim, maxParticles, species,
                                   posRange[0], posRange[1],
                                   velRange[0], velRange[1])

# Create a string that represents the dictionary used in the script
output = json.dumps(result)

# Export JSON string
with open("output.json", "w") as f:
    f.write(output)