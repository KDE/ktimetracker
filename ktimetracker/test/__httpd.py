'''Runs an HTTP server on port 8000 (or the first command line argument).'''

import BaseHTTPServer
import SimpleHTTPServer
import sys
import os.path

class MyHandler( SimpleHTTPServer.SimpleHTTPRequestHandler ):

  def do_PUT( self ):
    '''Just enough to work with karm.'''
    path = self.translate_path(self.path)

    rval = 200
    if not os.path.exists( path ): rval = 201

    f = file( path, "w" )
    lines = []
    while 1:
      line = self.rfile.readline()
      lines.append( line  )
      if line == '\r\n' or line == '\n' or line == '':
          break
    f.writelines( lines )
    self.send_response( rval )


DEFAULT_PORT = 8000

if sys.argv[1:]: port = int(sys.argv[1])
else: port = DEFAULT_PORT
server_address = ('', port)

SimpleHTTPServer.SimpleHTTPRequestHandler.protocol_version = "HTTP/1.0"
httpd = BaseHTTPServer.HTTPServer(server_address, MyHandler )

sa = httpd.socket.getsockname()
print "Serving HTTP on", sa[0], "port", sa[1], "..."
httpd.serve_forever()
