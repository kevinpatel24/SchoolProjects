import os, requests

from flask import Flask, session, render_template, request, jsonify
from flask_session import Session
from sqlalchemy import create_engine
from sqlalchemy.orm import scoped_session, sessionmaker
from werkzeug.security import generate_password_hash, check_password_hash

app = Flask(__name__)

# Check for environment variable
if not os.getenv("DATABASE_URL"):
    raise RuntimeError("DATABASE_URL is not set")

# Configure session to use filesystem
app.config["SESSION_PERMANENT"] = False
app.config["SESSION_TYPE"] = "filesystem"
app.config['JSON_SORT_KEYS'] = False
Session(app)

# Set up database
engine = create_engine(os.getenv("DATABASE_URL"))
db = scoped_session(sessionmaker(bind=engine))


@app.route("/")
def index():
    session.clear()
    return render_template("project1index.html")

@app.route("/login", methods=["POST"])
def login():
    #session.clear is for logout functionality when user clicks app logo
    session.clear()
    username = request.form.get("username")
    password = request.form.get("password")
    claimed_user = db.execute("SELECT * FROM users WHERE username = :username", {"username":username}).fetchone()

    if not claimed_user:
        return render_template("project1index.html", message="Username not found", color="red")

    else:
        if check_password_hash(claimed_user[2], password):
            session["userid"] = claimed_user[0]
            session["username"] = claimed_user[1]
            return render_template("searchpage.html", username=session["username"])

        else:
            return render_template("project1index.html", message="Incorrect Password", color="red")

@app.route("/register")
def register():
    return render_template("register.html")

@app.route("/registered", methods=["POST"])
def registered():
    username = request.form.get("username")
    password = request.form.get("password")

    if len(username) <= 3  or len(username) >= 16:
        return render_template("register.html", message="You must enter a username between 4-15 characters long", color="red")

    if len(password) <= 7  or len(password) >= 26:
        return render_template("register.html", message="You must enter a password between 8-25 characters long", color="red")

    if db.execute("SELECT * FROM users WHERE username = :username", {"username":username}).fetchone():
        return render_template("register.html", message="Username already exists", color="red")

    else:
        secure_password = generate_password_hash(password)
        db.execute("INSERT INTO users (username, password) VALUES(:username, :password)", {"username":username, "password":secure_password})
        db.commit()

    return render_template("project1index.html", message="Registration Successful! Please login", color="green")

@app.route("/search", methods=["POST"])
def search():
    query = request.form.get("query")
    query = ("%" + query + "%").title()
    rows = db.execute("Select * FROM books WHERE \
                        isbn LIKE :query OR \
                        title LIKE :query OR \
                        author LIKE :query",
                        {"query": query})
    results = rows.fetchall()

    if results:
        return render_template("searchresults.html", results=results)

    else:
        return render_template("searchpage.html", username=session["username"], message="No Result found. Please try a different search", color="red")

@app.route("/book/<isbn>", methods=["POST","GET"])
def book(isbn):
    key = "QdMmJmt0Oqj72WxROvUaxQ"
    book = db.execute("Select * FROM books WHERE isbn = :isbn", {"isbn":isbn}).fetchone()
    reviews = db.execute("Select username, bookreview FROM reviews JOIN users ON users.id = reviews.userid WHERE bookisbn = :isbn", {"isbn":isbn}).fetchall()
    res = requests.get("https://www.goodreads.com/book/review_counts.json", params={"key": key, "isbns": isbn})

    if res.status_code != 200:
        return render_template("book.html", book=book, reviews=reviews, ratings_count="Unavilable", average_rating="Unavailable")

    else:
        data = res.json()
        ratings_count = data['books'][0]['work_ratings_count']
        average_rating = data['books'][0]['average_rating']
        return render_template("book.html", book=book, reviews=reviews, ratings_count=ratings_count, average_rating=average_rating)

@app.route("/review/<isbn>", methods=["POST"])
def review(isbn):
    review = request.form.get("review")
    current_user = session["userid"]
    review_check = db.execute("Select * FROM reviews WHERE userid = :userid AND bookisbn = :bookisbn", {"userid":current_user, "bookisbn":isbn}).fetchone()
    key = "QdMmJmt0Oqj72WxROvUaxQ"
    book = db.execute("Select * FROM books WHERE isbn = :isbn", {"isbn":isbn}).fetchone()
    reviews = db.execute("Select username, bookreview FROM reviews JOIN users ON users.id = reviews.userid WHERE bookisbn = :isbn", {"isbn":isbn}).fetchall()
    res = requests.get("https://www.goodreads.com/book/review_counts.json", params={"key": key, "isbns": isbn})
    data = res.json()

    if res.status_code != 200:
        ratings_count = "Unavailable"
        average_rating = "Unavailable"

    else:
        ratings_count = data['books'][0]['work_ratings_count']
        average_rating = data['books'][0]['average_rating']

    if review_check:
        return render_template("book.html", book=book, reviews=reviews, ratings_count=ratings_count, average_rating=average_rating, message="You've already reviewed this book", color="red")

    else:
        if len(review) < 10:
           return render_template("book.html", book=book, reviews=reviews, ratings_count=ratings_count, average_rating=average_rating, message="Your review must be at least 10 characters long", color="red")

        else:
           db.execute("INSERT INTO reviews (userid, bookisbn, bookreview) VALUES(:userid, :bookisbn, :bookreview)", {"userid":current_user, "bookisbn":isbn, "bookreview":review})
           db.commit()
           reviews = db.execute("Select username, bookreview FROM reviews JOIN users ON users.id = reviews.userid WHERE bookisbn = :isbn", {"isbn":isbn}).fetchall()
           return render_template("book.html", book=book, reviews=reviews, ratings_count=ratings_count, average_rating=average_rating, message="Review Added!", color="green")

@app.route("/api/<isbn>", methods=["GET"])
def book_api(isbn):
    book = db.execute("Select * FROM books WHERE isbn = :isbn", {"isbn":isbn}).fetchone()
    reviews = db.execute("Select username, bookreview FROM reviews JOIN users ON users.id = reviews.userid WHERE bookisbn = :isbn", {"isbn":isbn}).fetchall()
    review_count = len(reviews)

    if book is None:
        return jsonify({"Error": "No book found"}), 404

    else:
        return jsonify({
                "title": book[1],
                "author": book[2],
                "year": book[3],
                "isbn": book[0],
                "review_count": review_count
        })
