#!/usr/bin/perl -w
use strict;
$|++;

use JSON;

my @sps;
my @pps;
my %stat;

sub usage
{
  my $me=`basename $0`;
  chomp $me;
  print "Usage: $me [avcc_parser_output] ...\n";
  exit 0;
}

sub process_json_text
{
  my $json_text = shift || return;

  my $ref = decode_json $json_text;
  if ($ref) {
    my $type = $ref->{"type"};
    if ($type) {
      if ($type eq "sequence_parameter_set") {
        push @sps, $ref;
      } elsif ($type eq "picture_parameter_set") {
        push @pps, $ref;
      }
      print STDERR "JSON object of type $type found.\n";
    } else {
      print STDERR "!! JSON object of unknown type found.\n";
    }
  } else {
    print STDERR "!! Failed to decode JSON text: $json_text\n";
  }
}

sub process_file
{
  my $file = shift || return;
  print STDERR "Processing $file...\n";

  open F,"<$file" or die "Unable to open $file: $!"; 

  my $json_text = "";
  my $in_json = 0;

  foreach (<F>) {
    chomp;
    if (/^{\s*$/) {
      $in_json = 1;
    } 

    if ($in_json) {
      $json_text .= $_;
    }
    
    if (/^}\s*$/) {
      $in_json = 0;
      process_json_text $json_text;
      $json_text = "";
    }
  }
  close F;
}

sub process_result_json
{
  my $name = shift || "";
  my $ref = shift || return;

  if (ref($ref) ne "HASH") {
    die "Result object is not hash?";
  }

  my $subname = $ref->{'type'} || "object";
  my $fullname = $name ? "$name.$subname" : $subname;

  foreach my $key (keys %{$ref}) {
    my $value = $ref->{$key};
    if (ref($value) eq "HASH") {
      process_result_json($fullname, $value);
    } elsif (ref($value) eq "ARRAY") {
      $stat{"$fullname.$key"}->{"[" . join ',',@{$value} . "]"}++;
    } else {
      $stat{"$fullname.$key"}->{"$value"}++;
    }
  }
}

sub process_result
{
  my $name = shift || "";
  my $ref = shift || return;
  
  if (ref($ref) ne "ARRAY") {
    die "Result argument is not an array.";
  }

  if (scalar @{$ref} == 0) {
    print STDERR "Nothing in the results to process.\n";
  }

  foreach my $json_ref (@{$ref}) {
    process_result_json($name, $json_ref);
  }
}

sub process_stat
{
  foreach my $key (keys %stat) {
    print "$key: ";

    my $ref=$stat{$key};
    foreach my $item (keys %{$ref}) {
      my $value = $ref->{"$item"};
      print "$item=$value ";
    }
    print "\n";
  }
}

### Main

if (scalar @ARGV == 0) {
  usage();
}

foreach my $file (@ARGV) {
  process_file($file);
}

process_result("", \@pps);
process_result("", \@sps);

process_stat();

exit 0;

