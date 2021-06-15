'''
database.py

Handles connecting to the MongoDB database and transferring data.

WARNING: 'client' needs to be configured to the required MongoDB database server
'''

from pydantic import BaseModel
from bson import ObjectId

import pymongo
import json
import sys

# Create a database 
client = pymongo.MongoClient("mongodb://localhost:9000/")
# myclient = pymongo.MongoClient("mongodb+srv://Command:<password>@rover-coordinates.9vrwc.mongodb.net/myFirstDatabase?retryWrites=true&w=majority")

Rover_database= client["Rover_database"]

def check_database_functional():
    # Checks if database is running by listing all databases in the system
    print(client.list_database_names())
    
def create_database_collection(collection_name: str):
    collection_name = Rover_database[collection_name]
    return collection_name

def check_collection_functional():
    # Checks if database collection exists or not
    print(Rover_database.list_collection_names())
    
def insert_record(collection, record):
    # Inserts a JSON-type object into database collection
    inserted = collection.insert_one(record)
    print(inserted.inserted_id) 
    
def find_records(collection):
    # Prints all the content in a particular collection
    for record in collection.find():
        print(record) 
        
def query_database(collection, query):
    # Takes in a 'collection' and 'query' e.g. query = { "map_type": "Obstacle" }, return and print the result of the query
    query_result = collection.find(query)

    for result in query_result:
        print(result) 
    
    return query_result

def delete_record(collection, query):
    # Deletes records based on a query
    deleted = collection.delete_one(query) 
    
    print("Deleted ", deleted)
    
def delete_collection(collection):
    # Deletes entire collection
    deleted = collection.delete_many({})
    
    print(deleted.deleted_count, " deleted.") 
