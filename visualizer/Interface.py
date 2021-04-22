import tkinter as tk
import tkinter.scrolledtext as st
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
        self.propData = data["properties"]
        self.step = 0
        self.times = list(self.simData)
        self.visDims = visDims
        self.scaleFactor = (min(visDims) / max(simDims)) * 0.3
        self.pack()
        self.createWidgets()
        self.updateApplication()
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
        self.stepForward_button.pack(side="right", padx=5, pady=5)

        self.stepBackward_button = tk.Button(self.controlFrame)
        self.stepBackward_button["text"] = "<==="
        self.stepBackward_button["command"] = self.stepBackward_CB
        self.stepBackward_button.pack(side="left", padx=5, pady=5)

        # Information panel
        self.informationFrame = tk.LabelFrame(self.leftFrame, text="Information")
        self.informationFrame.pack(side="top", padx=5, pady=5)

        self.numParticles_label = tk.Label(self.informationFrame, text=f"# Particles: {len(self.simData[self.times[self.step]])}")
        self.numParticles_label.pack(side="top", padx=5, pady=5)

        self.currTime_sv = tk.StringVar()
        self.currTime_sv.set(f"Time: {self.times[self.step]}")
        self.currTime_label = tk.Label(self.informationFrame, textvariable=self.currTime_sv)
        self.currTime_label.pack(side="top", padx=5, pady=5)

        # Particle data
        self.particleListFrame = tk.Frame(self.leftFrame)
        self.particleListFrame.pack(side="top", padx=5, pady=5)

        self.particleData_list = tk.Listbox(self.particleListFrame, width=25, height=10)
        self.particleData_list.grid(row=0, column=0, sticky="NSEW")

        self.particleData_vScroll = tk.Scrollbar(self.particleListFrame, orient="vertical")
        self.particleData_vScroll.config(command=self.particleData_list.yview)
        self.particleData_vScroll.grid(row=0, column=1, sticky="NS")

        self.particleData_hScroll = tk.Scrollbar(self.particleListFrame, orient="horizontal")
        self.particleData_hScroll.config(command=self.particleData_list.xview)
        self.particleData_hScroll.grid(row=1, column=0, sticky="EW")

        self.particleData_list.config(yscrollcommand=self.particleData_vScroll.set, xscrollcommand=self.particleData_hScroll.set)

        # Other buttons
        self.exit_button = tk.Button(self.leftFrame)
        self.exit_button["text"] = "Exit"
        self.exit_button["command"] = self.exit_CB
        self.exit_button.pack(side="bottom", fill="x")

        # Canvas
        self.canvas = tk.Canvas(self, bg="light grey", height=self.visDims[0], width=self.visDims[1])
        self.canvas.pack(side="right", padx=5, pady=5)

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

    def updateApplication (self):
        self.updateCanvas()
        self.updateParticleList()

    def getParticleListItem (self, snapshot):
        result = "ID: " + str(snapshot.getID()) + ", "
        result += "m: " + str(self.propData[snapshot.getID()]["mass"]) + ", "
        result += "v: " + str(snapshot.getVel()) + ", "
        result += "p: " + str(snapshot.getPos())
        return result

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
        if (self.step >= len(self.times) - 1):
            print("Cannot step forward")
            return
        self.step += 1
        self.currTime_sv.set(f"Time: {self.times[self.step]}")
        self.updateApplication()
        print("Stepping forward")

    def stepBackward_CB (self):
        if (self.step <= 0):
            print("Cannot step backward")
            return
        self.step -= 1
        self.currTime_sv.set(f"Time: {self.times[self.step]}")
        self.updateApplication()
        print("Stepping backward")

    def exit_CB (self):
        self.master.destroy()