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

uri = "mongodb+srv://csc2106:ppijBFqcBxQgFfAk@csc2106.tjdvgts.mongodb.net/?retryWrites=true&w=majority&appName=csc2106"

# Create a new client and connect to the server
client = MongoClient(uri)
mongoDatabase = client["csc2106"]
areaCoordinatesCollection = "area-coordinates"
elderlyM5Collection = "elderly-m5"
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

    documents = mongoDatabase[elderlyM5Collection].find()
    document = [doc for doc in documents]

    return  render_template("elderlyRealTimeData.html", documents=document, segment='index')

@blueprint.route("/lilygo-data", methods=['POST', 'GET'])
def lilygoData():
    global m5_hardware_id, elderly, geofenced_area, x , y, floor, timestamp
    if request.method == 'POST':
        lilygoData = request.json
        if lilygoData:
            m5_hardware_id = lilygoData.get('m5_hardware_id')
            elderly = lilygoData.get('elderly')
            geofenced_area = lilygoData.get('geofenced_area')
            x = lilygoData.get('x')
            y = lilygoData.get('y')
            floor = lilygoData.get('floor')
            timestamp = lilygoData.get('timestamp')

            # Create JSON objects for each collection
            elderly_m5_data = {
                'm5_hardware_id': m5_hardware_id,
                'elderly': elderly,
                'geofenced_area': geofenced_area,
            }

            location_data = {
                'm5_hardware_id': m5_hardware_id,
                'x': x,
                'y': y,
                'floor': floor,
                'timestamp': timestamp
            }
            
            # save to mongodb (on every POST request it will save to db, need to optimise)
            mongoDatabase[elderlyM5Collection].insert_one(elderly_m5_data)
            mongoDatabase[locationCollection].insert_one(location_data)

            return "data received at server", 200 # return to lilygo
        else:
            return "No JSON data received", 400 # return to lilygo
        
    m5_hardware_id = ""
    elderly = ""
    geofenced_area = ""
    x = ""
    y = ""
    floor = ""
    timestamp = ""
    return  render_template("lilygoData.html", m5_hardware_id=m5_hardware_id, elderly=elderly, geofenced_area=geofenced_area, x=x, y=y, floor=floor, timestamp=timestamp, segment='index')

@blueprint.route("/map")
def map():
    return  render_template("map.html", segment='index')

@blueprint.route("/map-data")
def sample_map_data():

    aggregated_data = {}
    documents = mongoDatabase[locationCollection].find()

    for document in documents:
        x = document["document_id"]["x"]
        y = document["document_id"]["y"]
        label = document["document_id"]["m5_hardware_id"]
        floor = document["document_id"]["floor"]

        if label not in aggregated_data:
            aggregated_data[label] = []

        aggregated_data[label].append({'x': int(x), 'y': int(y), 'label': label, 'floor': floor})
    
    return jsonify(aggregated_data)

@blueprint.route('/send_message')
def send_message():
    account_sid = 'AC2ff910daad2f4262a2707322a0603c61'
    auth_token = 'c931e1d104b2fa9f38045b2b16432b8f'
    client = Client(account_sid, auth_token)

    message = client.messages.create(
        body="test message",
        from_="+15642342977",
        to="+6586686767"
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
