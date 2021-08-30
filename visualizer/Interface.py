# Carleton University - ARSLab
# Thomas Roller
# Interface, the GUI used to access and view data

import tkinter as tk
import tkinter.scrolledtext as st
from ListScroll import ListScroll
import pprint

# https://stackoverflow.com/questions/17985216/draw-circle-in-tkinter-python
# create a circle
def _create_circle(self, x, y, r, **kwargs):
    return self.create_oval(x - r, y - r , x + r, y + r, **kwargs)

# add _create_circle to tkinter's Canvas object
tk.Canvas.create_circle = _create_circle

class Interface(tk.Frame):

    def __init__ (self, master=None, data={}, simDims=[], visDims=[]):
        super().__init__(master)
        self.master = master
        self.master.title("TPS Diagnostic Visualizer")
        self.eventData = data["events"]  # only lists which particles have changed
        self.propData = data["properties"]
        self.step = 0
        self.times = list(self.eventData)
        self.particleDict = self.buildParticleList()  # lists the position of every particle for every time step
        self.visDims = visDims
        self.scaleFactor = (min(visDims) / max(simDims)) * 0.3  # used to scale elements
        self.visDims[1] *= -1  # allow proper viewing (related to the flip on the y-axis

        self.canvasColours = ["azure2", "white", "light grey", "dark grey"]
        self.currCanvasColourIndex = 0

        self.pack()
        self.createWidgets()
        self.updateApplication()
        self.populateEventList()

        self.canvasScaleFactor = 1  # used to keep track of canvas zooming
        self.canvasPosition = [self.canvas.xview()[0], self.canvas.yview()[0]]
        print(f"Canvas position: {self.canvasPosition}")
        self.resetCanvas_CB()

        #pprint.pprint(self.simData)

    # create GUI elements
    # args:
    #     N/A
    # return:
    #     N/A
    def createWidgets (self):
        # Side panel (invisible)
        self.leftFrame = tk.Frame(self)
        self.leftFrame.pack(side="left", fill="y")

        # Step controls
        self.controlFrame = tk.LabelFrame(self.leftFrame, text="Controls")
        self.controlFrame.pack(side="top", padx=5, pady=5)

        self.stepForward_button = tk.Button(self.controlFrame)
        self.stepForward_button["text"] = "===>"
        self.stepForward_button["command"] = self.stepForward_CB
        if (self.step == len(self.times) - 1): self.stepForward_button["state"] = "disable"
        self.stepForward_button.pack(side="right", padx=5, pady=5)

        self.stepBackward_button = tk.Button(self.controlFrame)
        self.stepBackward_button["text"] = "<==="
        self.stepBackward_button["command"] = self.stepBackward_CB
        if (self.step == 0): self.stepBackward_button["state"] = "disable"
        self.stepBackward_button.pack(side="left", padx=5, pady=5)

        # Information panel
        self.informationFrame = tk.LabelFrame(self.leftFrame, text="Information")
        self.informationFrame.pack(side="top", padx=5, pady=5, ipadx=5, ipady=5, fill="x")

        self.currTime_sv = tk.StringVar()
        self.currTime_sv.set(f"Time: {self.times[self.step]}")
        self.currTime_label = tk.Label(self.informationFrame, textvariable=self.currTime_sv)
        self.currTime_label.pack(side="top", padx=0, pady=0)

        self.currEvent_sv = tk.StringVar()
        self.currEvent_sv.set(self.getEventString())
        self.currTime_label = tk.Label(self.informationFrame, textvariable=self.currEvent_sv)
        self.currTime_label.pack(side="top", padx=0, pady=0)

        # Lists
        self.listsFrame = tk.LabelFrame(self.leftFrame, text="")
        self.listsFrame.pack(side="top", padx=5, pady=5)

        self.numParticles_sv = tk.StringVar()
        self.numParticles_sv.set(f"Particles ({len(self.particleDict[self.times[self.step]])})")
        self.particleData_label = tk.Label(self.listsFrame, textvariable=self.numParticles_sv)
        self.particleData_label.pack(side="top", padx=5, pady=0)

        self.particleData_list = ListScroll(self.listsFrame)
        self.particleData_list.pack(side="top", padx=5, pady=5)

        self.currEventNum_sv = tk.StringVar()
        self.currEventNum_sv.set(f"Event Times ({self.step + 1} of {len(self.times)})")
        self.events_label = tk.Label(self.listsFrame, textvariable=self.currEventNum_sv)
        self.events_label.pack(side="top", padx=5, pady=0)

        self.events_list = ListScroll(self.listsFrame)
        self.events_list.pack(side="top", padx=5, pady=5)
        self.events_list.bind("<<ListboxSelect>>", self.selectEvent_CB)

        # Other buttons
        self.exit_button = tk.Button(self.leftFrame)
        self.exit_button["text"] = "Exit"
        self.exit_button["command"] = self.exit_CB
        self.exit_button.pack(side="bottom", fill="x")

        self.canvasReset_button = tk.Button(self.leftFrame)
        self.canvasReset_button["text"] = "Reset Canvas"
        self.canvasReset_button["command"] = self.resetCanvas_CB
        self.canvasReset_button.pack(side="bottom", fill="x")

        self.canvasColour_button = tk.Button(self.leftFrame)
        self.canvasColour_button["text"] = "Change Colour"
        self.canvasColour_button["command"] = self.canvasColour_CB
        self.canvasColour_button.pack(side="bottom", fill="x")

        # Canvas
        #self.canvas = tk.Canvas(self, bg=self.canvasColours[self.currCanvasColourIndex % len(self.canvasColours)], width=self.visDims[0], height=self.visDims[1])
        print(f"VisDims: {self.visDims}")
        self.canvas = tk.Canvas(self, bg=self.canvasColours[self.currCanvasColourIndex % len(self.canvasColours)], width=900, height=700)
        self.canvas.pack(side="right", padx=5, pady=5)
        self.canvas.xview_moveto(0)
        self.canvas.yview_moveto(-700)

        # https://stackoverflow.com/questions/41656176/tkinter-canvas-zoom-move-pan
        self.canvas.bind("<ButtonPress-1>", lambda event : self.canvas.scan_mark(event.x, event.y))
        self.canvas.bind("<B1-Motion>", lambda event : self.canvas.scan_dragto(event.x, event.y, gain=1))
        self.canvas.bind("<MouseWheel>", self.zoomCanvas_CB)

    # update the canvas with elements (removes all elements and redraws them)
    # args:
    #     N/A
    # return:
    #     N/A
    def updateCanvas (self):
        self.canvas.delete("all")
        for snapshot in self.particleDict[self.times[self.step]].values():
            x = (snapshot.getPos()[0] * self.scaleFactor) + (self.visDims[0] / 2)
            y = (snapshot.getPos()[1] * self.scaleFactor) + (self.visDims[1] / 2)
            y *= -1  # flip to represent particles with +y on top (as opposed to tkinter's +y on bottom)
            colour = "black"
            if (len(snapshot.getPos()) == 3):
                colour = Interface.getColour(snapshot.getPos()[2], -3, 3)
            radius = self.propData[snapshot.getID()]["radius"] * self.scaleFactor
            self.canvas.create_circle(x, y, radius, width=2, outline=colour)
            self.canvas.create_text(x, y, text=str(snapshot.getID()))

            directionLine = Interface.getDirectionLine(snapshot.getVel(), radius)
            self.canvas.create_line(x, y, x + directionLine[0], y - directionLine[1])  # subtract (instead of add) from y to flip

    def buildParticleList (self):
        result = {}
        for step in range(0, len(self.times)):
            if (step != 0):
                result[self.times[step]] = result[self.times[step - 1]].copy()  # bring forward the old data to be modified
            for snapshot in self.eventData[self.times[step]]["snapshots"]:
                if (self.times[step] not in result):  # if the time is not in the result...
                    result[self.times[step]] = {}     # add it as a dictionary
                result[self.times[step]][snapshot.getID()] = snapshot  # update the old data for the current step
        return result

    # update the GUI element that lists particles an their information
    # args:
    #     N/A
    # return:
    #     N/A
    def updateParticleList (self):
        self.particleData_list.delete(0, "end")
        for snapshot in self.particleDict[self.times[self.step]].values():
            self.particleData_list.insert("end", self.getParticleListItem(snapshot))

    # update which element is selected in the GUI element that lists the events of the simulation
    # args:
    #     N/A
    # return:
    #     N/A
    def updateEventSelection (self):
        self.events_list.activate(self.step)
        self.events_list.focus_set()  # set focus here so that the graphic updates

    # updates major GUI elements (calls several other update functions)
    # args:
    #     N/A
    # return:
    #     N/A
    def updateApplication (self):
        self.updateCanvas()
        self.updateParticleList()
        self.updateEventSelection()
        self.checkProgressButtonsStatus()
        self.currTime_sv.set(f"Time: {self.times[self.step]}")
        self.currEventNum_sv.set(f"Event Times ({self.step + 1} of {len(self.times)})")
        self.currEvent_sv.set(self.getEventString())
        self.numParticles_sv.set(f"Particles ({len(self.particleDict[self.times[self.step]])})")

    # check whether or not the user is at the first or last element and set the button states accordingly
    # args:
    #     N/A
    # return:
    #     N/A
    def checkProgressButtonsStatus (self):
        if (self.step <= 0):
            print("First event reached")
            self.stepBackward_button["state"] = "disable"
        else:
            self.stepBackward_button["state"] = "normal"

        if (self.step >= len(self.times) - 1):
            print("Last event reached")
            self.stepForward_button["state"] = "disable"
        else:
            self.stepForward_button["state"] = "normal"

    # construct the string which represents a particle (used in GUI element that lists particles and their information)
    # args:
    #     snapshot: a Snapshot that should be made into a string
    # return:
    #     string representing a particle
    def getParticleListItem (self, snapshot):
        result = "ID: " + str(snapshot.getID()) + ", "
        result += "m: " + str(self.propData[snapshot.getID()]["mass"]) + ", "
        result += "v: " + str(snapshot.getVel()) + ", "
        result += "p: " + str(snapshot.getPos())
        return result

    # populate the GUI element that lists the events of the simulation
    # args:
    #     N/A
    # return:
    #     N/A
    def populateEventList (self):
        self.events_list.delete(0, "end")
        for time in self.eventData:
            self.events_list.insert("end", time)

    # determine an event's type and the particle(s) involved
    # args:
    #     N/A
    # return:
    #     string detailing the relevant information
    def getEventString (self):
        if (self.step == 0):
            return "Type: N/A"
        event = None
        time = self.times[self.step]
        try:
            event = self.eventData[time]
        except KeyError:
            print(f"Invalid key: {time}")
            return "Error"
        eventType = event["type"]
        eventIDs = [x.getID() for x in event["snapshots"]]
        return f"Type: {eventType} (IDs: {eventIDs})"

    # get the vector (tail at origin) representing the direction that a particle is moving
    # args:
    #     desiredLength: the length that the resulting line should be
    # return:
    #     a vector (list) representing the line
    @staticmethod
    def getDirectionLine (vel, desiredLength):
        result = []
        velLen = 0
        for comp in vel:
            velLen += comp ** 2
        velLen = velLen ** 0.5
        if (velLen == 0):
            velLen = 1
        for comp in vel:
            result.append((comp / velLen) * desiredLength)
        return result

    # calculate the colour that a particle should be based on its position in the 3rd dimension
    # args:
    #     z: the position of the particle in the 3rd dimension
    #     minZ: the position which will get one of the extreme colours
    #     maxZ: the position which will get the other of the extreme colours
    # return:
    #     hexidecimal representation of the calculated colour
    @staticmethod
    def getColour (z, minZ, maxZ):
        greyScale = int(Interface.calcShade(z, minZ, maxZ, 0, 255))
        return Interface.convertColour(greyScale, greyScale, greyScale)

    # calculate the shade of grey that is desired
    # args:
    #     z: the position of the particle in the 3rd dimension
    #     minZ: the position which will get one of the extreme colours
    #     maxZ: the position which will get the other of the extreme colours
    #     minC: the minimum value for a colour (likely in [0, 255))
    #     minC: the maximum value for a colour (likely in (0, 255])
    # return:
    #     value representing the shade for the given z value
    @staticmethod
    def calcShade (z, minZ, maxZ, minC, maxC):
        shade = minC + ((maxC - minC) * ((z - minZ) / (maxZ - minZ)))
        #shade = maxC - shade  # make lower z-values lighter instead of darker
        if (shade < minC):
            return minC
        elif (shade > maxC):
            return maxC
        return shade

    # convert RGB values into hexidecimal
    # args:
    #     r: red component of the colour
    #     g: green component of the colour
    #     b: blue component of the colour
    # return:
    #     hexidecimal representation of the colour
    @staticmethod
    def convertColour (r, g, b):
        return "#%02x%02x%02x" % (r, g, b)

    # the initial function for the GUI
    # args:
    #     data: data required for visualization
    #     simDims: the dimensions of the simulated area
    #     visDims: the dimensions of the visualization space
    # return:
    #     N/A
    @staticmethod
    def start (data, simDims, visDims):
        root = tk.Tk()
        app = Interface(master=root, data=data, simDims=simDims, visDims=visDims)
        app.mainloop()

    # ==================
    # Callback functions
    # ==================

    # GUI element: "===>" button
    # move forward by an event
    def stepForward_CB (self):
        print("Stepping forward")
        self.step += 1
        self.updateApplication()

    # GUI element: "<===" button
    # move backward by an event
    def stepBackward_CB (self):
        print("Stepping backward")
        self.step -= 1
        self.updateApplication()

    # GUI element: event list
    # update the current event internally and update the GUI's canvas
    def selectEvent_CB (self, event):
        try:
            self.step = self.events_list.curselection()[0]
            self.updateApplication()
        except IndexError:
            print("WARNING: possible list mismatch (ignoring)")

    # GUI element: canvas
    # respond to mouse scoll wheel by zooming in/out
    def zoomCanvas_CB (self, event):
        factor = 1.001 ** event.delta
        self.canvasScaleFactor *= factor  # track scaling
        self.canvas.scale(tk.ALL, event.x, event.y, factor, factor)

    # GUI element: "Change Colour" button
    # cycle through possible background colours
    def canvasColour_CB (self):
        self.currCanvasColourIndex += 1
        self.canvas.configure(bg=self.canvasColours[self.currCanvasColourIndex % len(self.canvasColours)])

    # GUI element: "Reset Canvas" button
    # reset the canvas zoom and position
    def resetCanvas_CB (self):
        print(f"Old coordinates: {[self.canvas.xview()[0], self.canvas.yview()[0]]}")
        self.canvas.scale(tk.ALL, self.visDims[0] / 2, self.visDims[1] / 2, 1 / self.canvasScaleFactor, 1 / self.canvasScaleFactor)
        self.canvasScaleFactor = 1
        self.canvas.xview_moveto(self.canvasPosition[0])
        self.canvas.yview_moveto(self.canvasPosition[1])
        self.currCanvasColourIndex = 0
        self.canvas.configure(bg=self.canvasColours[self.currCanvasColourIndex % len(self.canvasColours)])

    # GUI element: "Exit" button
    # exit the application
    def exit_CB (self):
        self.master.destroy()