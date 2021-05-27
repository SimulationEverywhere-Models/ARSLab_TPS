import random
import json

# minumum: float
# maximum: float
def randomNum (minimum, maximum):
    return (random.random() * (maximum - minimum)) + minimum

# position: list
# velocity: list
# species: string
def createParticle (position, velocity, species):
    return {
        "position" : position,
        "velocity" : velocity,
         "species" : species
    }

# p1: list
# p2: list
def distance (p1, p2):
    if (len(p1) != len(p2)):
        return -1

    result = 0
    for i in range(0, len(p1)):
        result += (p2[i] - p1[i]) ** 2
    return result ** 0.5

# newPos: list
# newSpecies: string
# existingParticles: dict (result["particles"])
# allSpecies: dict
def isValidPos (newPos, newSpecies, existingParticles, allSpecies):
    for pID in existingParticles:
        dist = distance(existingParticles[pID]["position"], newPos)
        r1 = allSpecies[existingParticles[pID]["species"]]["radius"]
        r2 = allSpecies[newSpecies]["radius"]
        if (dist < r1 + r2):
            return False
    return True

# dim: int
# minumum: float
# maximum: float
def genList (dim, minimum, maximum):
    return [randomNum(minimum, maximum) for i in range(0, dim)]

# dim: int
# maxParticles: int
# allSpecies: dict
# minPos: float
# maxPos: float
# minVel: float
# maxVel: float
# maxAttempts: int
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

result = None
with open("config_template.json", "r") as f:
    result = json.loads(f.read())

species = result["species"]

#dim = 2
#maxParticles = 1000
#posRange = [-150, 150]
#velRange = [-3, 3]

dim = 2
maxParticles = 2000
posRange = [-150, 150]
velRange = [-3, 3]
result["particles"] = genParticles(dim, maxParticles, species, posRange[0], posRange[1],
                                   velRange[0], velRange[1])

output = json.dumps(result)

with open("output.json", "w") as f:
    f.write(output)