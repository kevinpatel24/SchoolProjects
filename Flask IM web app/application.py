import os

import datetime
from flask import Flask, render_template, request, session
from flask_session import Session
from flask_socketio import SocketIO, emit

app = Flask(__name__)
app.config["SECRET_KEY"] = os.getenv("SECRET_KEY")
app.config["SESSION_TYPE"] = "filesystem"

socketio = SocketIO(app)
Session(app)

chats = {}

@app.route("/")
def index():
    return render_template("project2index.html", chats=chats)

@app.route("/newUser", methods=["POST"])
def newUser():
    newDisplayname=request.form.get("displayname")
    session["username"] = newDisplayname
    session["username"].permanent = True

@app.route("/chat/<key>", methods=["POST","GET"])
def chat(key):
    #chats is a dictionary so we can have a seperate empty list for each individual chat with a corresponding key
    if key in chats:
        session["channel"] = key
        return render_template("chatpage.html", chat=key, messages = chats[key])
    else:
        return render_template("project2index.html", chats=chats, message="Please see below for a list of real chats")

@app.route("/addChat", methods=["POST"])
def addChat():
    newChat = request.form.get("newChat")
    if newChat in chats:
        return render_template("project2index.html", chats=chats, message="A chat with that name already exists")
    elif len(newChat) < 4 or len(newChat) > 10:
        return render_template("project2index.html", chats=chats, message="New chat name should be 4-10 characters long")
    else:
        #creating a new chat with an empty list where every message where go into. Each chat stored in chats dictionary
        chats[newChat] = []
        return render_template("project2index.html", chats=chats)

@socketio.on('new message')
def newMessage(newMessageText, happyBirthday):
    time = '{:%H:%M:%S}'.format(datetime.datetime.now())
    newMessage=[session["username"], newMessageText, time]
    chats[session["channel"]].append(newMessage)
    if len(chats[session["channel"]]) > 100:
        #Gets rid of earliest message if a chat has 100 messages in it
        chats[session["channel"]].pop(0)

    emit("message added", {"displayname":session["username"], "addedMessageText": newMessageText, "timestamp":time, "session": session["channel"], "happyBirthday":happyBirthday}, broadcast=True)
