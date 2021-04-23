import tkinter as tk
import tkinter.scrolledtext as st
from ListScroll import ListScroll
import pprint

# https://stackoverflow.com/questions/17985216/draw-circle-in-tkinter-python
def _create_circle(self, x, y, r, **kwargs):
    return self.create_oval(x - r, y - r , x + r, y + r, **kwargs)

tk.Canvas.create_circle = _create_circle

class Interface(tk.Frame):

    def __init__ (self, master=None, data={}, simDims=[], visDims=[]):
        super().__init__(master)
        self.master = master
        self.master.title("TPS Diagnostic Visualizer")
        self.simData = data["simulation"]
        self.eventData = data["events"]
        self.propData = data["properties"]
        self.step = 0
        self.times = list(self.simData)
        self.visDims = visDims
        self.scaleFactor = (min(visDims) / max(simDims)) * 0.3  # used to scale elements

        self.pack()
        self.createWidgets()
        self.updateApplication()
        self.populateEventList()

        self.canvasScaleFactor = 1  # used to keep track of canvas zooming
        self.canvasPosition = [self.canvas.xview()[0], self.canvas.yview()[0]]  # keep track of original canvas position

        pprint.pprint(self.simData)

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

        self.particleData_label = tk.Label(self.listsFrame, text=f"Particles ({len(self.simData[self.times[self.step]])})")
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

        # Canvas
        self.canvas = tk.Canvas(self, bg="light grey", height=self.visDims[0], width=self.visDims[1])
        self.canvas.pack(side="right", padx=5, pady=5)

        # https://stackoverflow.com/questions/41656176/tkinter-canvas-zoom-move-pan
        self.canvas.bind("<ButtonPress-1>", lambda event : self.canvas.scan_mark(event.x, event.y))
        self.canvas.bind("<B1-Motion>", lambda event : self.canvas.scan_dragto(event.x, event.y, gain=1))
        self.canvas.bind("<MouseWheel>", self.zoomCanvas_CB)

    def updateCanvas (self):
        self.canvas.delete("all")
        for snapshot in self.simData[self.times[self.step]]:
            # only handle 2D for now
            x = (snapshot.getPos()[0] * self.scaleFactor) + (self.visDims[0] / 2)
            y = (snapshot.getPos()[1] * self.scaleFactor) + (self.visDims[1] / 2)
            radius = self.propData[snapshot.getID()]["radius"] * self.scaleFactor
            self.canvas.create_circle(x, y, radius)
            self.canvas.create_text(x, y, text=str(snapshot.getID()))

            directionLine = Interface.getDirectionLine(snapshot.getVel(), radius)
            self.canvas.create_line(x, y, x + directionLine[0], y + directionLine[1])

    def updateParticleList (self):
        self.particleData_list.delete(0, "end")
        for snapshot in self.simData[self.times[self.step]]:
            self.particleData_list.insert("end", self.getParticleListItem(snapshot))

    def updateEventSelection (self):
        self.events_list.activate(self.step)
        self.events_list.focus_set()  # set focus here so that the graphic updates

    def updateApplication (self):
        self.updateCanvas()
        self.updateParticleList()
        self.updateEventSelection()
        self.checkProgressButtonsStatus()
        self.currTime_sv.set(f"Time: {self.times[self.step]}")
        self.currEventNum_sv.set(f"Event Times ({self.step + 1} of {len(self.times)})")
        self.currEvent_sv.set(self.getEventString())

    def getParticleListItem (self, snapshot):
        result = "ID: " + str(snapshot.getID()) + ", "
        result += "m: " + str(self.propData[snapshot.getID()]["mass"]) + ", "
        result += "v: " + str(snapshot.getVel()) + ", "
        result += "p: " + str(snapshot.getPos())
        return result

    def populateEventList (self):
        self.events_list.delete(0, "end")
        for time in self.simData:
            self.events_list.insert("end", time)

    def getEventString (self):
        if (self.step == 0):
            return "Type: N/A"
        event = self.eventData[self.times[self.step]]
        eventType = event["type"]
        eventIDs = event["particles"]
        return f"Type: {eventType} (IDs: {eventIDs})"

    @staticmethod
    def getDirectionLine (vel, desiredLength):
        result = []
        velLen = 0
        for comp in vel:
            velLen += comp ** 2
        velLen = velLen ** 0.5
        for comp in vel:
            result.append((comp / velLen) * desiredLength)
        return result

    @staticmethod
    def start (data, simDims, visDims):
        root = tk.Tk()
        app = Interface(master=root, data=data, simDims=simDims, visDims=visDims)
        app.mainloop()

    # ==================
    # Callback functions
    # ==================

    def stepForward_CB (self):
        print("Stepping forward")
        self.step += 1
        self.updateApplication()

    def stepBackward_CB (self):
        print("Stepping backward")
        self.step -= 1
        self.updateApplication()

    def selectEvent_CB (self, event):
        try:
            self.step = self.events_list.curselection()[0]
            self.updateApplication()
        except IndexError:
            print("WARNING: possible list mismatch (ignoring)")

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

    def zoomCanvas_CB (self, event):
        factor = 1.001 ** event.delta
        self.canvasScaleFactor *= factor  # track scaling
        self.canvas.scale(tk.ALL, event.x, event.y, factor, factor)

    def resetCanvas_CB (self):
        self.canvas.scale(tk.ALL, self.visDims[0] / 2, self.visDims[1] / 2, 1 / self.canvasScaleFactor, 1 / self.canvasScaleFactor)
        self.canvasScaleFactor = 1
        self.canvas.xview_moveto(self.canvasPosition[0])
        self.canvas.yview_moveto(self.canvasPosition[1])

    def exit_CB (self):
        self.master.destroy()