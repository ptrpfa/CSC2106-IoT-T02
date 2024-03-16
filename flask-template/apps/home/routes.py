# -*- encoding: utf-8 -*-
"""
Copyright (c) 2019 - present AppSeed.us
"""

from apps.home import blueprint
from flask import render_template, request
from flask_login import login_required
from jinja2 import TemplateNotFound

m5_hardware_id = ""
elderly = ""
geofenced_area = ""
first = ""

@blueprint.route('/index')
def index():

    return render_template('home/index.html', segment='index')



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
            return "data received at server", 200
        else:
            return "No JSON data received", 400

    return  render_template("test.html", m5_hardware_id=m5_hardware_id, elderly=elderly, geofenced_area=geofenced_area, segment='index')

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
