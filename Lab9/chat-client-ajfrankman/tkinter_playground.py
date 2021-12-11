from tkinter import *

# send function
def sendMessage():
    message = messageEntry.get()
    try:
        definition = my_compdictionary[message]
    except:
        definition = "oops"
    
    listbox.insert(END, definition)


###### main:
window = Tk()
window.title("Mqtt Chat Client")
window.configure(background='black')


#create message label
Label(window, text='Enter your message: ', bg = 'black', fg = 'white', font = 'none 12 bold').grid(row = 1, column = 0, sticky = W)
#create a text entry box
messageEntry = Entry(window, width = 20, bg = "white")
messageEntry.grid(row=2, column=0, sticky=W)

#add send button
Button(window, text = "Send Message", width = 13, command = sendMessage).grid(row = 4, column = 1, sticky = W)

#create a list box
listbox = Listbox(window, width = 75, height = 25)
listbox.grid(row = 6, column = 0, columnspan = 3, sticky = W)

# dictionary
my_compdictionary = {"test" : "this is a test", "test2" : "this is a test2"}

###### runs the main loop
window.mainloop()