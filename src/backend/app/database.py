from pydantic import BaseModel
from pymongo import MongoClient
from bson import ObjectId

import json
import sys

# client = MongoClient("mongodb+srv://Command:<password>@rover-coordinates.9vrwc.mongodb.net/myFirstDatabase?retryWrites=true&w=majority")

'''
Set up connection to MongoDB database
'''


client = MongoClient("mongodb+srv://Command:ajhMlFhNaELMEoC8>@cluster0.9vrwc.mongodb.net/test?retryWrites=true&w=majority")
db = client.test

print(db)