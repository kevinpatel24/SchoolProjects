import os, csv
from sqlalchemy import create_engine
from sqlalchemy.orm import scoped_session, sessionmaker

# Check for environment variable
if not os.getenv("DATABASE_URL"):
    raise RuntimeError("DATABASE_URL is not set")

# Set up database
engine = create_engine(os.getenv("DATABASE_URL"))
db = scoped_session(sessionmaker(bind=engine))

f = open("books.csv",)
lines = f.readlines()[1:]
f.close()
reader = csv.reader(lines)

db.execute("CREATE TABLE books (isbn VARCHAR PRIMARY KEY UNIQUE NOT NULL, title VARCHAR NOT NULL, author VARCHAR, year INT)")
db.commit()


for isbn, title, author, year in reader:


    db.execute("INSERT INTO books (isbn, title, author, year) VALUES (:isbn, :title, :author, :year)",
                {"isbn":isbn,
                "title":title,
                "author":author,
                "year":year})
    db.commit()
