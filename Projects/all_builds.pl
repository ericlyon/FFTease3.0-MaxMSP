#!/usr/bin/perl                                                                 

# Perl script to compile FFTease 3.0
# USAGE: perl buildall.pl

$here = `pwd`; # local directory
$here =~ tr/\n//d; # strip trailing carriage return

$cleanup = 0; # use at own risk to autoremove compile sub-products
$fullclean = 1; # use only to delete all build directories

while(<*>){
    chomp;
    if(-d ){
		$builddir = $here . "/" . $_ . "/build";
  	  	if($fullclean && ($_ ne "libfftease") ){
  	  		print "CLEANING $_\n";
			`rm -rf \"$builddir\"`;
    	}
		elsif($_ ne "libfftease"){
			print "BUILDING $_\n";
			$proj = $here . "/" .  $_ . "/" . "fftz." . $_ . ".xcodeproj";
			`xcodebuild -project \"$proj\" -configuration Deployment`;
			if($cleanup){
				`rm -rf \"$builddir\"`;
			}
		}
    }
}

