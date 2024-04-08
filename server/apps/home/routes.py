# -*- encoding: utf-8 -*-
"""
Copyright (c) 2019 - present AppSeed.us
"""

from apps.home import blueprint
from flask import render_template, request, jsonify
from flask_login import login_required
from jinja2 import TemplateNotFound
from pymongo import MongoClient
import json
from twilio.rest import Client
import os

uri = "mongodb+srv://csc2106:ppijBFqcBxQgFfAk@csc2106.tjdvgts.mongodb.net/?retryWrites=true&w=majority&appName=csc2106"

# Create a new client and connect to the server
client = MongoClient(uri)
mongoDatabase = client["csc2106"]
areaCoordinatesCollection = "area-coordinates"
locationCollection = "location"

m5_hardware_id = ""
elderly = ""
geofenced_area = ""
x = ""
y = ""
floor = ""
timestamp = ""

@blueprint.route('/index')
def index():
    return render_template('home/index.html', segment='index')

@blueprint.route("/elderly-real-time-data")
def realTimeData():

    documents = mongoDatabase[locationCollection].find()
    document = [doc for doc in documents]

    return  render_template("elderlyRealTimeData.html", documents=document, segment='index')

@blueprint.route("/developer-form")
def form():
    return  render_template("developerForm.html", segment='index')

@blueprint.route("/lilygo-data", methods=['POST', 'GET'])
def lilygoData():
    global m5_hardware_id, elderly, geofenced_area, x , y, floor, timestamp
    if request.method == 'POST':
        lilygoData = request.json
        print(lilygoData)
        if lilygoData:
            m5_hardware_id = lilygoData.get('macAddress')
            elderly = lilygoData.get('elderly')
            geofenced_area = lilygoData.get('geofenced_area')
            x = lilygoData.get('x')
            y = lilygoData.get('y')
            floor = lilygoData.get('floor')

            if (x != None and y != None and m5_hardware_id != None):
                if (x > 0 and y > 0):
                    # Create JSON objects for each collection
                    location_data = {
                        'm5_hardware_id': m5_hardware_id,
                        'elderly': elderly,
                        'geofenced_area': geofenced_area,
                        'x': x,
                        'y': y,
                        'floor': floor,
                    }
            
                    # save to mongodb (on every POST request it will save to db, need to optimise)
                    # mongoDatabase[elderlyM5Collection].insert_one(elderly_m5_data)
                    mongoDatabase[locationCollection].insert_one(location_data)

                    # Check whether elderly is in geofenced location
                    if geofenced_area == 'Flat A':
                        if not (x < 10 and y > 10):
                            send_message("+6586686767")

                    elif geofenced_area == 'Flat B':
                        if not (x > 10 and x < 20 and y > 10):
                            send_message("+6586686767")

                    elif geofenced_area == 'Flat C':
                        if not (x > 20 and x < 30 and y > 10):
                            send_message("+6586686767")


            return "data received at server", 200 # return to lilygo
        else:
            return "No JSON data received", 400 # return to lilygo
    
    return  render_template("lilygoData.html", m5_hardware_id=m5_hardware_id, elderly=elderly, geofenced_area=geofenced_area, x=x, y=y, floor=floor, timestamp=timestamp, segment='index')

@blueprint.route("/map")
def map():
    return  render_template("map.html", segment='index')

@blueprint.route("/map-data")
def sample_map_data():

    aggregated_data = {}
    map_documents = mongoDatabase[locationCollection].find().sort("_id", -1)

    for document in map_documents:
        x = document["x"]
        y = document["y"]
        label = document["elderly"]
        floor = document["floor"]

        if label not in aggregated_data:
            aggregated_data[label] = []
            aggregated_data[label].append({'x': int(x), 'y': int(y), 'label': label, 'floor': floor})
    
    return jsonify(aggregated_data)

def send_message(number):
    account_sid = 'AC524e033062d7d1f4a65f7f161ddf63e5'
    auth_token = os.getenv('AUTH_TOKEN')
    client = Client(account_sid, auth_token)

    message = client.messages.create(
        body="Your elderly has left their area! Please find them immediately.",
        from_="+12057828196",
        to=number
    )

    print(message.sid)

@blueprint.route('/<template>')
@login_required
def route_template(template):

    try:

        if not template.endswith('.html'):
            template += '.html'

        # Detect the current page
        segment = get_segment(request)

        # Serve the file (if exists) from app/templates/home/FILE.html
        return render_template("home/" + template, segment=segment)

    except TemplateNotFound:
        return render_template('home/page-404.html'), 404

    except:
        return render_template('home/page-500.html'), 500

# Helper - Extract current page name from request
def get_segment(request):

    try:

        segment = request.path.split('/')[-1]

        if segment == '':
            segment = 'index'

        return segment

    except:
        return None
