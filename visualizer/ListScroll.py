import tkinter as tk

class ListScroll (tk.Frame):

    def __init__ (self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.createWidgets()

    def createWidgets (self):
        self.itemList = tk.Listbox(self, width=25, height=10)
        self.itemList.grid(row=0, column=0, sticky="NSEW")

        self.vScroll = tk.Scrollbar(self, orient="vertical")
        self.vScroll.config(command=self.itemList.yview)
        self.vScroll.grid(row=0, column=1, sticky="NS")

        self.hScroll = tk.Scrollbar(self, orient="horizontal")
        self.hScroll.config(command=self.itemList.xview)
        self.hScroll.grid(row=1, column=0, sticky="EW")

        self.itemList.config(yscrollcommand=self.vScroll.set, xscrollcommand=self.hScroll.set)

    def delete (self, *args, **kwargs):
        self.itemList.delete(*args, **kwargs)

    def insert (self, *args, **kwargs):
        self.itemList.insert(*args, **kwargs)

    def activate (self, *args, **kwargs):
        self.itemList.activate(*args, **kwargs)

    def bind (self, *args, **kwargs):
        self.itemList.bind(*args, **kwargs)

    def curselection (self, *args, **kwargs):
        return self.itemList.curselection(*args, **kwargs)

    def focus_set (self, *args, **kwargs):
        self.itemList.focus_set(*args, **kwargs)