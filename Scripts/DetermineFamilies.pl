#!/bin/perl

require 5.0.0.0;
use MAMEoXScriptConstants;


$ROOT_DIR		= MAMEoXScriptConstants::ROOT_DIR;
$MAMEoX_DIR = MAMEoXScriptConstants::MAMEoX_DIR;
$MAME_DIR   = MAMEoXScriptConstants::MAME_DIR;
$MAME_SRC_DIR			= $MAME_DIR."/src";
$MAME_CPU_DIR			= $MAME_SRC_DIR."/cpu";
$MAME_DRIVER_DIR	= $MAME_SRC_DIR."/drivers";
$MAME_MACHINE_DIR = $MAME_SRC_DIR."/machine";
$MAME_SOUND_DIR		= $MAME_SRC_DIR."/sound";
$MAME_SNDHRDW_DIR = $MAME_SRC_DIR."/sndhrdw";
$MAME_VIDHRDW_DIR = $MAME_SRC_DIR."/vidhrdw";


%DependencyHash;
%FilesDependentHash;
RebuildDependencyHash( "Dependencies.txt" ) || die( "Failed to rebuild dependency hash. Run FindDependencies.pl!\n" );


 # Remove anything that won't be sectionized by OldSectionize.pl
foreach $Key ( keys( %DependencyHash ) ) {

		# Make sure the Key is either a CPU, Driver, or there's a driver of
		# the same name
	if( $Key =~ /[machine|sndhrdw|vidhrdw]\/(.*)/ ) {
		if( !defined( $DependencyHash{"drivers/$1"} ) ) {
			#print "Removing $Key\n";
			undef( $DependencyHash{$Key} );
		}		
	}
}


print "\r\n\r\nFiles Dependent\r\n";
foreach $Key ( sort( keys( %FilesDependentHash ) ) ) {
	if( $Key =~ /drivers\/(.*)/ ) {
		local $BaseFile = $1;
		local $ArrayRef = $FilesDependentHash{ $Key };
		local @InterestingDeps;

		foreach $Dependent ( @$ArrayRef ) {
			  # Don't bother printing the obvious matches
			if( $Dependent =~ /[machine|sndhrdw|vidhrdw]\/$BaseFile/ ) {
				next;
			}
			push( @InterestingDeps, $Dependent );
		}

		if( scalar( @InterestingDeps ) > 0 ) {
			print "\r\n$Key\r\n";
			foreach $Dependent ( sort( @InterestingDeps ) ) {
				print ">  $Dependent\r\n";
			}
		}
	}
}
print "\r\nDone\r\n";



#== F U N C T I O N S ===================================================

#------------------------------------------------------------------------
# CopyArray
#------------------------------------------------------------------------
sub CopyArray( $ ) {
	local $ArrayToCopyRef = shift;
	local @Array = @$ArrayToCopyRef;
	return \@Array;
}


#------------------------------------------------------------------------
#	RebuildDependencyHash
#------------------------------------------------------------------------
sub RebuildDependencyHash( $ ) {
	local $DependencyHashFile = shift;
	open( FILE, "<$DependencyHashFile" ) || return false;
	local $KeyValue = undef;
	local @DependencyArray;

	while( <FILE> ) {
		$Line = $_;
		if( $Line =~ /\*\s+$MAME_DIR\/debug_obj\/(.+\/.+)\.obj/ ) {
				# New key value
				#* ../MAME/debug_obj/drivers/namcos22.obj
				# Store the dependency array if we have one
			if( defined( $KeyValue ) && scalar(@DependencyArray) > 0 ) {
				$DependencyHash{"$KeyValue.c"} = CopyArray( \@DependencyArray );
				@DependencyArray = ();
			}

			$KeyValue = $1;

		} elsif( $_ =~ />\s+$MAME_DIR\/debug_obj\/(.+\/.+)\.obj/ ) {
				#> ../MAME/debug_obj/vidhrdw/namcos22.obj
			push( @DependencyArray, "$1.c" );

			if( defined( $FilesDependentHash{ "$1.c" } ) ) {
				local $ArrayRef = $FilesDependentHash{ "$1.c" };
				push( @$ArrayRef, "$KeyValue.c" );
			} else {
				local @FilesDependent = ( "$KeyValue.c" );
				$FilesDependentHash{"$1.c"} = CopyArray( \@FilesDependent );
			}
		}
	}
	close( FILE );

		# Append the last item
	if( scalar( @DependencyArray ) > 0 ) {
		$DependencyHash{"$KeyValue.c"} = CopyArray( \@DependencyArray );
	}
}
