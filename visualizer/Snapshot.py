# A class representing a particle at a given point in time

class Snapshot:

    def __init__ (self, pID=-1, pos=[], vel=[]):
        self.__id = pID
        self.__pos = pos
        self.__vel = vel

    def getID (self):
        return self.__id

    def getPos (self):
        return self.__pos

    def getVel (self):
        return self.__vel

    def makeScaledSnapshot (self, factor):
        return Snapshot(
            self.__id,
            [(x * factor) for x in self.__pos],
            [(x * factor) for x in self.__vel]
        )

    def __str__ (self):
        return f"ID: {self.__id}, pos: {self.__pos}, vel: {self.__vel}"

    def __eq__ (self, other):
        return self.__id == other.__id and self.__pos == other.__pos and self.__vel == other.__vel