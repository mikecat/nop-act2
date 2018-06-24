#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

if (@ARGV < 1) {
	warn sprintf("Usage: perl %s output_file\n", basename($0, ""));
	exit 1;
}

my @seven_seg = (
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 1, 1, 0, 0,
	0, 2, 0, 0, 0, 0, 3, 0,
	0, 2, 0, 0, 0, 0, 3, 0,
	0, 2, 0, 0, 0, 0, 3, 0,
	0, 2, 0, 0, 0, 0, 3, 0,
	0, 0, 4, 4, 4, 4, 0, 0,
	0, 5, 0, 0, 0, 0, 6, 0,
	0, 5, 0, 0, 0, 0, 6, 0,
	0, 5, 0, 0, 0, 0, 6, 0,
	0, 5, 0, 0, 0, 0, 6, 0,
	0, 0, 7, 7, 7, 7, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
);
my @seven_seg_fonts = (
	0xee, 0x48, 0xba, 0xda, 0x5c, 0xd6, 0xf6, 0x4e,
	0xfe, 0xde, 0x7e, 0xf4, 0xb0, 0xf8, 0xb6, 0x36
);

my $img_data = "";
my $width = 8 * 17;
my $height = 16 * 17;

for (my $y = $height - 1; $y >= 0; $y--) {
	for (my $x = 0; $x < $width; $x++) {
		my ($r, $g, $b);
		if ($x < 8 && $y < 16) {
			($r, $g, $b) = (192, 192, 192);
		} elsif ($y < 16) {
			($r, $g, $b) = ($seven_seg_fonts[int($x / 8) - 1] >> $seven_seg[$y * 8 + ($x % 8)]) & 1 ?
				(0, 0, 0) : (192, 192, 192);
		} elsif ($x < 8) {
			($r, $g, $b) = ($seven_seg_fonts[int($y / 16) - 1] >> $seven_seg[($y % 16) * 8 + $x]) & 1 ?
				(0, 0, 0) : (192, 192, 192);
		} elsif (($x + 1) % 8 == 0 || ($y + 1) % 16 == 0) {
			($r, $g, $b) = (192, 192, 192);
		} else {
			($r, $g, $b) = (255, 255, 255);
		}
		$img_data .= pack("CCC", $b, $g, $r);
	}
	while (length($img_data) % 4 != 0) { $img_data .= "\x00"; }
}

my $header = pack("LLSSLLLLLL", $width, $height, 1, 24, 0, length($img_data), 0, 0, 0, 0);
$header = pack("L", length($header) + 4) . $header;

my $data = "BM" . pack("LSSL", 14 + length($header) + length($img_data), 0, 0, 14 + length($header)) .
	$header . $img_data;

open(FILE, "> $ARGV[0]") or die "output file $ARGV[0] open failed\n";
binmode(FILE);
print FILE $data;
close(FILE);
