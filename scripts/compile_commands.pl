#!/usr/bin/env perl

my $usage = <<USAGE;
Usage:  $0 [-m MASK]

Options:
    -m MASK  Generate compile_commands JSON with platform mask MASK.
USAGE

use FindBin '$RealBin';

use lib "$RealBin/../tools/perl";

use Compile::Driver;
use Compile::Driver::Catalog;
use Compile::Driver::Configuration;
use Compile::Driver::Options;

use warnings FATAL => 'all';
use strict;

use Getopt::Std;
use JSON::XS;

sub HELP_MESSAGE {
  my $stat = shift;

  print $usage;

  exit ($stat eq 1 ? 1 : 0);
}

my @db;
my %opts;
if (not getopts('hm:', \%opts)) {
    HELP_MESSAGE(1);
}

my $mask;
Compile::Driver::Options::set_options( ( "-S" ) );
my $config = Compile::Driver::Configuration::->new(
    Compile::Driver::Options::specs
);
my %catalog = %{ Compile::Driver::Catalog::catalog() };

if (exists $opts{h}) {
    HELP_MESSAGE(0);
}

if (exists $opts{m}) {
    $mask = $opts{m};
} else {
    $mask = $config->platform_mask;
}

sub get_module {
    my ( $name ) = @_;

    my $desc = Compile::Driver::Catalog::find_project( $name, $mask );

    if ( !$desc ) {
        $desc = { NAME => $name, MEMO => { PREREQS => [] } };
    }

    return Compile::Driver::Module::->new( $config, $desc );
}

sub get_job {
    my ( $job ) = @_;

    return if !$job->is_buildable;
}

sub process_jobs {
    my ( @jobs ) = @_;

    my $job;
    for $job (@jobs) {
        if ( $job->{TYPE} eq 'CC' ) {
            ;

            my $entry = {
                          directory => $job->{FROM}->tree,
                          command   => join( " ", $job->command ),
                          file      => $job->input_files
                        };

            push @db, $entry;
        }
    }
}

my $name;
for $name (keys %catalog) {
    my $module = get_module($name);

    my @jobs = Compile::Driver::jobs_for($module);

    process_jobs(@jobs);
}
print JSON::XS::encode_json \@db;
