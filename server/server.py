from flask import Flask, jsonify, render_template, request

app = Flask(__name__)


ipaddress = ""
third = ""
second = ""
first = ""

# @app.route("/", methods=['POST', 'GET'])
# def home():
#     return  render_template("index.html")

@app.route("/", methods=['POST', 'GET'])
def data():
    global ipaddress, third, second, first
    if request.method == 'POST':
        data = request.json
        if data:
            ipaddress = data.get('ipaddr')
            third = data.get('third')
            second = data.get('second')
            first = data.get('first')
            print("Received IP address: ", ipaddress)
            print("Received from third floor lilygo: ", third)
            print("Received from second floor lilygo: ", second)
            print("Received from first floor lilygo: ", first)
            # print(request.form)
            return "data received at server", 200
        else:
            return "No JSON data received", 400

    return  render_template("index.html", ipaddress=ipaddress, third=third, second=second, first=first)



if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=5000)
