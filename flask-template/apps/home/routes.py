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
first = ""

@blueprint.route('/index')
def index():

    return render_template('home/index.html', segment='index')
"mongodb+srv://csc2106:ppijBFqcBxQgFfAk@csc2106.tjdvgts.mongodb.net/?retryWrites=true&w=majority&appName=csc2106"


@blueprint.route("/test", methods=['POST', 'GET'])
def data():
    global m5_hardware_id, elderly, geofenced_area, first
    if request.method == 'POST':
        data = request.json
        if data:
            m5_hardware_id = data.get('m5_hardware_id')
            elderly = data.get('elderly')
            geofenced_area = data.get('geofenced_area')
            first = data.get('first')
            print("m5_hardware_id: ", m5_hardware_id)
            print("elderly: ", elderly)
            print("geofenced_area: ", geofenced_area)
            # print(request.form)
            
            # save to mongodb (on every POST request it will save to db, need to optimise)
            mongoDatabase[elderlyM5Collection].insert_one(data)

            return "data received at server", 200
        else:
            return "No JSON data received", 400

    return  render_template("test.html", m5_hardware_id=m5_hardware_id, elderly=elderly, geofenced_area=geofenced_area, segment='index')

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

        if label not in aggregated_data:
            aggregated_data[label] = []

        aggregated_data[label].append({'x': int(x), 'y': int(y), 'label': label})

    data = {
        "Elderly 1": [
            {'x': 100, 'y': 100, 'label': 'Elderly 1'},
        ],
        "Elderly 2": [
            {'x': 200, 'y': 500, 'label': 'Elderly 1'},
        ],
        "Elderly 3": [
            {'x': 400, 'y': 200, 'label': 'Elderly 1'},
        ]
    }
    
    return jsonify(aggregated_data)

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
