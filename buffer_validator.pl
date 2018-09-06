#!/usr/bin/perl

use strict;
use warnings;

if (@ARGV < 2) {
	die "Usage: perl $0 [-q|-v] efi_file memory_src_file\n-q : be quite\n-v : be verbose\n";
}

my $quiet = 0;
my $verbose = 0;
if ($ARGV[0] eq "-q") {
	$quiet = 1;
	shift @ARGV;
} elsif ($ARGV[0] eq "-v") {
	$verbose = 1;
	shift @ARGV;
}

my $efi_file = $ARGV[0];
my $memory_src_file = $ARGV[1];

my $mem_buffer_start;

open(MEM, "< $memory_src_file") or die("$memory_src_file open error!\n");
while (my $line = <MEM>) {
	chomp($line);
	if ($line =~ /#define\s+BUFFER_START\s+0[xX]([0-9a-fA-F]+)/) {
		$mem_buffer_start = hex($1);
		last;
	}
}
close(MEM);

unless(defined($mem_buffer_start)) {
	die "failed to get BUFFER_START value!\n";
}

unless ($quiet) { printf "BUFFER_START in source file = 0x%08X\n", $mem_buffer_start; }

open(EFI, "< $efi_file") or die("$efi_file open error!\n");
binmode(EFI);
my $efi_data = "";
while (<EFI>) { $efi_data .= $_; }
close(EFI);

my $efi_length = length($efi_data);
if ($verbose) { printf "EFI file size = 0x%08x\n", $efi_length; }
if ($efi_length < 0x40) { die "EFI too small to have MS-DOS header!\n"; }
my $magic = unpack("v", substr($efi_data, 0x00, 2));
if ($verbose) { printf "MS-DOS header magic = 0x%04x\n", $magic; }
if ($magic != 0x5a4d) { die "EFI MS-DOS header magic error!\n"; }
my $lfanew = unpack("V", substr($efi_data, 0x3c, 4));

if ($verbose) { printf "NT header offset = 0x%08x\n", $lfanew; }
if ($efi_length < $lfanew + 24) { die "EFI too small to have NT header!\n"; }
my $nt_signature = unpack("V", substr($efi_data, $lfanew + 0, 4));
if ($verbose) { printf "NT header signature = 0x%08x\n", $nt_signature; }
if ($nt_signature != 0x00004550) { die "EFI NT header signature error!\n"; }
my $num_sections = unpack("v", substr($efi_data, $lfanew + 6, 2));
my $optheader_size = unpack("v", substr($efi_data, $lfanew + 20, 2));
if ($verbose) {
	printf "number of sections = %d\n", $num_sections;
	printf "size of optional header = 0x%04x\n", $optheader_size;
}

my $optheader_start = $lfanew + 24;
if ($verbose) { printf "optipnal header offset = 0x%08x\n", $optheader_start; }
if ($efi_length < $optheader_start + $optheader_size) {
	die "EFI too small to have optional header!\n";
}
if ($optheader_size < 32) { die "EFI optional header too small to have ImageBase!\n"; }
my $opt_magic = unpack("v", substr($efi_data, $optheader_start + 0, 2));
if ($verbose) { printf "optional header magic = 0x%04x\n", $opt_magic; }
if ($opt_magic != 0x10b && $opt_magic != 0x107) {
	die "EFI optional header magic error!\n";
}
my $image_base = unpack("V", substr($efi_data, $optheader_start + 28, 4));
if ($verbose) { printf "image base = 0x%08x\n", $image_base; }

my $sections_start = $optheader_start + $optheader_size;
if ($verbose) { printf "section table offset = 0x%08x\n", $sections_start; }
if ($efi_length < $sections_start + 40 * $num_sections) {
	die "EFI too small to have section table!\n";
}
my $section_va_max = 0;
for (my $i = 0; $i < $num_sections; $i++) {
	my $sinfo_offset = $sections_start + 40 * $i;
	my $virtual_size = unpack("V", substr($efi_data, $sinfo_offset + 8, 4));
	my $virtual_address = unpack("V", substr($efi_data, $sinfo_offset + 12, 4));
	my $section_end = $image_base + $virtual_address + $virtual_size;
	if ($section_end > $section_va_max) { $section_va_max = $section_end; }
	if ($verbose) {
		my $section_name = substr($efi_data, $sinfo_offset + 0, 8);
		$section_name =~ s/\0+//;
		printf "section table [%d] = %s\n", $i, $section_name;
		printf "  offset       = 0x%08x\n", $sinfo_offset;
		printf "  load address = 0x%08x\n", $virtual_address;
		printf "  load size    = 0x%08x\n", $virtual_size;
		printf "  load end     = 0x%08x\n", $section_end;
	}
}
unless ($quiet) { printf "        highest section end = 0x%08X\n", $section_va_max; }

if ($section_va_max <= $mem_buffer_start) {
	unless ($quiet) { print "collision check OK\n"; }
	exit 0;
} else {
	unless ($quiet) { print "collision detected!\n"; }
	exit 1;
}
