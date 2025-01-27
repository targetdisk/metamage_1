package Compile::Driver::Job::Link::Archive;

use     Compile::Driver::Job ;
@ISA = 'Compile::Driver::Job';

use warnings FATAL => 'all';
use strict;


*lib_filename = \&Compile::Driver::Job::lib_filename;


sub new
{
	my $class = shift;
	
	my %self = @_;
	
	my $module = $self{ FROM };
	
	$self{ PATH } = lib_filename( $module->name );
	
	return bless \%self, $class;
}

sub input_files
{
	my $self = shift;
	
	my $objs = $self->{OBJS};
	
	return @$objs;
}

sub tool_name
{
	my $self = shift;
	
	my $module = $self->{FROM};
	
	my $conf = $module->{CONF};
	
	return $conf->is_retro68 ? $conf->arch_option . "-apple-macos-ar" : "ar";
}

sub command
{
	my $self = shift;
	
	my $dest = $self->{DEST};
	
	unlink( $dest );
	
	return $self->tool_name, 'rcs', $dest;
}

1;
