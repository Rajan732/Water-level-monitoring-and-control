import datetime as dt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import pyrebase as pb

firebaseConfig = {
  'apiKey': "AIzaSyDMp_uzbqG3UyocJ-aSETAeLKGv-M86PaU",
  'authDomain': "ii-project-8ff73.firebaseapp.com",
  'databaseURL': "https://ii-project-8ff73-default-rtdb.firebaseio.com",
  'projectId': "ii-project-8ff73",
  'storageBucket': "ii-project-8ff73.appspot.com",
  'messagingSenderId': "585899683289",
  'appId': "1:585899683289:web:f547d1da0ad541f0473dc8",
  'measurementId': "G-S0LGHY8SSQ"
}
firebase=pb.initialize_app(firebaseConfig)
database=firebase.database()

# Create figure for plotting
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
xs = []
ys = []
sp =[]

def animate(i, xs, ys):

    c = database.get()
    # Add x and y to lists
    xs.append(dt.datetime.now().strftime('%H:%M:%S.%f'))
   
    dic = c.val()
    ys.append(dic['Distance'])
    sp.append(dic['sp'])

    # Limit x and y lists to 20 items
    xs = xs[-20:]
    ys = ys[-20:]
    if len(sp)>20:
        sp.pop()

    # Draw x and y lists
    ax.clear()
    ax.plot(xs, ys)
    ax.plot(xs,sp)
    # Format plot
    plt.xticks(rotation=45, ha='right')
    plt.subplots_adjust(bottom=0.30)
    plt.title('Count vs time')
    plt.ylabel('Count')

# Set up plot to call animate() function periodically
ani = animation.FuncAnimation(fig, animate, fargs=(xs, ys), interval=1000)
plt.show()

