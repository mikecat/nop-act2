#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

my $block_width = 8;
my $block_height = 16;
my $top_header_block = 1;
my $left_header_block = 1;

if (@ARGV < 2) {
	warn sprintf("Usage: perl %s input_file output_file\n", basename($0, ""));
	exit 1;
}

open(FILE, "< $ARGV[0]") or die ("input file $ARGV[0] open error\n");
binmode(FILE);
$/ = undef; # no delimiter : read whole file at once
my $img = <FILE>;
close(FILE);

if (substr($img, 0, 2) ne "BM") { die("invalid signature\n"); }
if (length($img) < 18) { die ("file too small\n"); }

my $img_offset = unpack("L", substr($img, 10, 4));
my $header_size = unpack("L", substr($img,  14, 4));
if ($header_size < 20) { die ("header too short\n"); }
if (length($img) < 14 + $header_size) { die ("header truncated\n"); }

my ($img_width, $img_height, $planes, $bitcount, $compression) =
	unpack("LLSSL", substr($img, 18, 16));
if ($img_height < 0 || $planes != 1 || $bitcount != 24 || $compression != 0) {
	die ("unsupported format\n");
}

if ($img_width != $block_width * (16 + $left_header_block) ||
$img_height != $block_height * (16 + $top_header_block)) {
	die ("image size mismatch\n");
}

my $img_byte_width = (($img_width * 3 + 3) >> 2) * 4;
if (length($img) < $img_offset + $img_byte_width * $img_height) {
	die ("image truncated\n");
}

open(SRC, "> $ARGV[1]") or die("output file $ARGV[1] open error\n");
binmode(SRC);

print SRC "#include \"font.h\"\n";
print SRC "\n";
print SRC "unsigned char font_data[256][16] = {\n\t";

my $format = sprintf("0x%%0%dx", (($block_width + 3) >> 2));
for (my $i = 0; $i < 256; $i++) {
	if ($i > 0) { print SRC ", "; }
	print SRC sprintf("{ %s/* %02X %c */",
		$i == 0 ? "   " : "", $i, 0x20 <= $i && $i < 0x7f ? $i : 0x20);
	for (my $j = 0; $j < $block_height; $j++) {
		my $c = 0;
		print "----- $i $j\n";
		for (my $k = 0; $k < $block_width; $k++) {
			my $y = $img_height - 1 - (($i >> 4) + $top_header_block) * $block_height - $j;
			my $x = ($i % 16 + $left_header_block) * $block_width + ($block_width - 1 - $k);
			my ($b, $g, $r) = unpack("CCC", substr($img, $img_offset + $y * $img_byte_width + $x * 3, 3));
			if ($r < 128 && $g < 128 && $b < 128) { $c |= (1 << $k); }
			print "$k $x $y\n";
		}

		if ($j % 8 == 0) {
			if ($j > 0) { print SRC ","; }
			print SRC "\n\t\t";
		} else {
			print SRC ", ";
		}
		print SRC sprintf($format, $c);
	}
	print SRC "\n\t}";
}

print SRC "\n};\n";

close(SRC);
