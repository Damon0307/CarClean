import http.server
import socketserver
import json

PORT = 8888
HOST = '192.168.1.110'

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        request_body = self.rfile.read(content_length)
        print(request_body.decode('utf-8'))
        json_data = json.loads(request_body.decode('utf-8'))
        with open('icp_report.json', 'w') as f:
            json.dump(json_data, f)
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b'OK')

with socketserver.TCPServer((HOST, PORT), MyHandler) as httpd:
    print('serving at port', PORT)
    httpd.serve_forever()