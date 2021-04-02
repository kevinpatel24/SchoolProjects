# Project 1 

Web Programming with Python and JavaScript

This Pull request is for Project 1: Books. You will be able to register, login, logout, search for a book and see all the necessary information for the books you search for. You can also post reviews and make API requests. 
For me, this project consists of several files, listed below.
application.py - Main application file
import.py - Execcuting this creates the books table and imports info from books.csv
register.html - Register for site
book.html - Arrive here after clicking on search result to see info on book
project1index.html - Login for site
project1layout.html - Layout for all pages
searchpage.html - Go here upon login to do a search
searchresults.html - Arrive here upon doing a search
books.csv -Base file
README.md -Text explanation
requirements.text -Base file(I had to add -binary to psycopg2 for it to work)
_pycache_ - Base file

I also had 3 SQL tables listed below.
books - (ISBN VARCHAR, Author VARCHAR, Publication year INT, title VARCHAR)
users - (userid SERIAL, username VARCHAR, password VARCHAR)
reviews - (reviewid INT, userid INT, review VARCHAR, bookisbn VARCHAR)

I think this was a very beneficial project as it really drives home a lot of key skills and enjoyed it very much. 

Best,
Kevin
