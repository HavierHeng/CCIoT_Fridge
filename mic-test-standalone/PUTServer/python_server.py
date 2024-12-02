
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib
import urllib.parse

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    
    def do_PUT(self):
        # Extract the file name from the URL
        filename = os.path.basename(urllib.parse.unquote(self.path))
        
        # Set the content length and prepare to read the incoming data
        content_length = int(self.headers['Content-Length'])
        
        # Open a file in the current directory to save the uploaded data
        with open(filename, 'wb') as f:
            # Read the data in chunks and write to the file
            remaining_data = content_length
            while remaining_data > 0:
                chunk_size = min(1024, remaining_data)
                chunk = self.rfile.read(chunk_size)
                f.write(chunk)
                remaining_data -= len(chunk)
        
        # Send a 200 OK response
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'File uploaded successfully')

    def do_GET(self):
        # Extract the file name from the URL path
        filename = os.path.basename(urllib.parse.unquote(self.path))
        
        # Make sure the file exists and is in the current directory or a safe folder
        if not os.path.isfile(filename):
            self.send_response(404)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'File not found')
            return
        
        # If the file exists, serve it
        try:
            with open(filename, 'rb') as f:
                # Get the file's content type based on its extension
                file_extension = os.path.splitext(filename)[1].lower()
                content_type = 'application/octet-stream'  # Default for binary files
                if file_extension == '.txt':
                    content_type = 'text/plain'
                elif file_extension == '.html':
                    content_type = 'text/html'
                elif file_extension == '.jpg' or file_extension == '.jpeg':
                    content_type = 'image/jpeg'
                elif file_extension == '.png':
                    content_type = 'image/png'
                elif file_extension == '.pdf':
                    content_type = 'application/pdf'
                elif file_extension == ".wav":
                    content_type = 'audio/wav'
                # Add other content types as needed

                # Send the response headers
                self.send_response(200)
                self.send_header('Content-type', content_type)
                self.send_header('Content-Length', str(os.path.getsize(filename)))
                self.end_headers()

                # Read and send the file content in chunks
                chunk = f.read(1024)
                while chunk:
                    self.wfile.write(chunk)
                    chunk = f.read(1024)
        except Exception as e:
            self.send_response(500)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'Internal Server Error: ' + str(e).encode())

    # def log_message(self, format, *args):
    #     # Overriding this to prevent unnecessary logging of each request
    #     return

def run(server_class=HTTPServer, handler_class=SimpleHTTPRequestHandler, port=8080):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f"Starting server on port {port}...")
    httpd.serve_forever()

if __name__ == '__main__':
    run()

