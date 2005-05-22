#!/usr/bin/perl -w

# This script requires the following Perl modules:
#
#  $ perl -MCPAN -e 'shell'
#  cpan> install Net::DAV::Server
#  cpan> install Filesys::Virtual::Plain
#  cpan> install File::Find::Rule::Filesys::Virtual
#  cpan> install XML::LibXML	
#  
#  The last Perl modules needs the libxml2 development libraries installed
#  (the libxml2-dev package on Debian).

use Net::DAV::Server;
use Filesys::Virtual::Plain;
use HTTP::Daemon;

# If 1, output request and response headers
my $DEBUG=0;

my $filesys = Filesys::Virtual::Plain->new();
$filesys->root_path('/tmp');
$filesys->cwd('/tmp');
#print foreach ($filesys->list('/'));

my $webdav = Net::DAV::Server->new();
$webdav->filesys($filesys);

my $d = new HTTP::Daemon
  LocalAddr => 'localhost',
  LocalPort => 4242,
  ReuseAddr => 1 || die;
print "Please contact me at: ", $d->url, "\n";
while (my $c = $d->accept) {
  while (my $request = $c->get_request) {
    if ( $DEBUG ) {
      print qq|------------------------------------------------------------
REQUEST
------------------------------------------------------------\n|;
      while ( ($k,$v) = each %{$request} ) {
        print "  $k => $v\n";
      }
    }
    my $response = $webdav->run($request);
    if ( $DEBUG ) {
      print qq|------------------------------------------------------------
RESPONSE
------------------------------------------------------------\n|;
      while ( ($k,$v) = each %{$response} ) {
        print "  $k => $v\n";
      }
    }
    $c->send_response ($response);
  }
  $c->close;
  undef($c);
}
