# Carleton University - ARSLab
# Thomas Roller
# Parser, prepares data for use

from Snapshot import Snapshot
import re
import json

class Parser:

    def __init__ (self):
        print("WARNING: treat Parser class as static")
        raise Exception

    # import data from log file as a list
    @staticmethod
    def importRaw (filename):
        with open(filename, "r") as f:
            return f.readlines()

    # import JSON
    @staticmethod
    def importJSON (filename):
        with open(filename, "r") as f:
            return json.loads(f.read())

    # get and prepare data for the visualizer
    # args:
    #     stateLogFilename: name of state log file
    #     messageLogFilename: name of message log file
    #     configFilename: name of (Cadmium model's) configuration file
    #     stateTransient: whether or not to optimize for storage (True is recommended)
    # return:
    #     dictionary containing data for visualization
    @staticmethod
    def getData (stateLogFilename, messageLogFilename, configFilename, stateTransient=True):
        simulation = None
        if (stateTransient):
            simulation = Parser.FileIndex(stateLogFilename)
        else:
            simulation = Parser.StateParser.parseLog(Parser.importRaw(stateLogFilename))
        return {
            "simulation" : simulation,
            "events" : Parser.MessageParser.parseLog(Parser.importRaw(messageLogFilename)),
            "properties" : Parser.ConfigParser.parseParticleProperties(Parser.importJSON(configFilename))
        }

    # ------------------------------------------
    # Parse information related to configuration
    #-------------------------------------------

    class ConfigParser:

        def __init__ (self):
            print("WARNING: treat ConfigParser class as static")

        # parse particle properties
        # args:
        #     data: dictionary representation of model's configuration file
        # return:
        #     dictionary of pIDs and related data
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

    # ------------------------------------------------
    # Parse information related to Cadmium's state log
    # ------------------------------------------------

    class StateParser:

        def __init__ (self):
            print("WARNING: treat StateParser class as static")
            raise Exception

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
                    float(contents[line])
                    time = contents[line].rstrip("\n")
                    continue
                except ValueError:
                    # line is not a time
                    pass

                # if the line is not the state of the subV model
                if (contents[line].find("subV") == -1):
                    continue

                # this will override previous entires for a given time (until the time changes)
                # this is desired behaviour
                result[time] = Parser.StateParser.parseLine(contents[line])

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
                match = re.match(r"\(p_id:(?P<id>\d+)\): pos<(?P<pos>.+)>, vel<(?P<vel>.+)>", particle)
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

        # scale data by a calculated factor
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

    # ------------------------------------------------
    # Index and access information related to Cadmium's state log
    # Avoid storing more of the file than what is required
    # ------------------------------------------------

    class FileIndex:

        def __init__ (self, filename):
            self.filename = filename
            self.index = Parser.FileIndex.indexFile(filename)  # of format: {{key, loc}, {key, loc}, ...}

        # define the [] operator
        def __getitem__ (self, key):
            if (key not in self.index):
                print(f"KeyError: FileIndex could not find key: {key} ({type(key)})")
                return KeyError

            line = ""
            with open(self.filename) as f:
                f.seek(self.index[key], 0)
                line = f.readline()

            return Parser.StateParser.parseLine(line)

        # for iteration
        def __iter__ (self):
            return iter(self.index)

        # for iteration
        def __next__ (self):
            return next(self.index)

        # index the important lines of a file
        # args:
        #     filename: name of the file to be indexed
        # return:
        #     dictionary of times and positions in the file
        @staticmethod
        def indexFile (filename):
            result = {}
            with open(filename, "r") as f:
                line = "_"
                time = -1
                while (line):
                    position = f.tell()
                    line = f.readline()

                    try:
                        float(line)
                        time = line.rstrip("\n")
                        continue
                    except ValueError:
                        pass

                    if (line.find("subV") >= 0):
                        result[time] = position

            return result

    # --------------------------------------------------
    # Parse information related to Cadmium's message log
    # --------------------------------------------------

    class MessageParser:

        def __init__ (self):
            print("WARNING: treat MessageParser class as static")
            raise Exception

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
                    float(contents[line])
                    time = contents[line].rstrip("\n")
                    continue
                except ValueError:
                    # line is not a time
                    pass

                if (time not in result):
                    temp = Parser.MessageParser.parseLine(contents[line])
                    if (temp is not None):
                        result[time] = temp

            return result

        # parse line in log into Snapshots
        # args:
        #     line: line from log
        # return:
        #     list of Snapshots
        @staticmethod
        def parseLine (line):
            result = {
                "type" : "",
                "particles" : []
            }

            if (line.find("RandomImpulse") >= 0):
                result["type"] = "impulse"

            elif (line.find("SubV") >= 0):
                result["type"] = "collision"

            else:
                return None

            for section in line.split("]["):
                match = re.search(r"p_id:(?P<id>\d+)", section)
                if (match is None):
                    print(f"WARNING: no match for: {section}")
                    continue
                result["particles"].append(int(match.group("id")))

            if (len(result["particles"]) == 0):
                print(f"WARNING: no particle IDs found for line: {line}")
                return None

            return result