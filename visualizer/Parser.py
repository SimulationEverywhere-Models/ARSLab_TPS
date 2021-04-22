from Snapshot import Snapshot
import re
import json

class Parser:

    def __init__ (self):
        print("WARNING: treat Parser class as static")

    # import data from log file as a list
    @staticmethod
    def importRaw (filename):
        with open(filename, "r") as f:
            return f.readlines()

    # parse log into a dictionary of times and Snapshots
    # args:
    #     contents: list of lines from log
    # return:
    #     dictionary of times and Snapshops
    @staticmethod
    def parseLog (contents):
        result = {}
        time = -1
        for line in range(0, len(contents)):
            try:
                # line represents a time, note the time and skip to the subV line of the log
                time = float(contents[line])
                continue
            except ValueError:
                # line is not a time
                pass

            # if the line is not the state of the subV model
            if (contents[line].find("subV") == -1):
                continue

            # this will override previous entires for a given time (until the time changes)
            # this is desired behaviour
            result[time] = Parser.parseLine(contents[line])

        return result

    # parse line in log into Snapshots
    # args:
    #     line: line from log
    # return:
    #     list of Snapshots
    @staticmethod
    def parseLine (line):
        result = []

        # convert line into a list of strings representing particles
        data = line[line.find("["):].strip("[]").split("][")

        # parse each string
        for particle in data:
            match = re.match(r"\(p_id:(?P<id>\d)\): pos<(?P<pos>.+)>, vel<(?P<vel>.+)>", particle)
            if (match is None):
                print(f"WARNING: no match for: {particle}")
                continue
            result.append(Snapshot(
                    int(match.group("id")),
                    [float(x) for x in match.group("pos").split(" ")],
                    [float(x) for x in match.group("vel").split(" ")]
                )
            )

        return result

    @staticmethod
    def importJSON (filename):
        with open(filename, "r") as f:
            return json.loads(f.read())

    @staticmethod
    def parseParticleProperties (data):
        result = {}
        for pID in data["particles"]:
            currSpecies = data["particles"][pID]["species"]
            result[int(pID)] = {
                "mass" : data["species"][currSpecies]["mass"],
                "radius" : data["species"][currSpecies]["radius"]
            }
        return result

    @staticmethod
    def getData (logFilename, configFilename):
        logData = Parser.parseLog(Parser.importRaw(logFilename))
        configData = Parser.parseParticleProperties(Parser.importJSON(configFilename))
        return {
            "simulation" : logData,
            "properties" : configData
        }

    # scale data by 
    # args:
    #     data: data from log
    #     resultDims: dimensions of simulated space
    #     visualizerDims: dimensions of visualizer
    # return:
    #     scaled log data
    @staticmethod
    def scaleData (data, resultDims, visualizerDims):
        result = {
            "simulation" : {},
            "properties" : {}
        }
        scaleFactor = min(visualizerDims) / max(resultDims)
        # Scale positions and velocities
        for time in data["simulation"]:
            result["simulation"][time] = []
            for snapshot in data["simulation"][time]:
                result["simulation"][time].append(snapshot.makeScaledSnapshot(scaleFactor))
        # Scale radii
        for particle in data["properties"]:
            result["properties"][particle]["radius"] = data["properties"][particle]["radius"] * scaleFactor
            result["properties"][particle]["mass"] = data["properties"][particle]["mass"]
        return result