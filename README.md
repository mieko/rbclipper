Synopsis
==========
These are Ruby 1.9.2+ bindings to Clipper, Angus Johnson's Polygon clipping
library. Because Clipper is not readily packaged, and is so beautifully
self-contained, I've included the two required files in the package.

This release contains version 2.521 of Clipper.

[Clipper Homepage](http://angusj.com/delphi/clipper.php)

To install:

    ruby ./extconf.rb
    make
    make install

Simple Usage:
===========
    require 'clipper'

    a = [[0, 0], [0, 100], [100, 100], [100, 0]]
    b = [[-5, 50], [200, 50], [100, 5]]

    c = Clipper::Clipper.new

    c.add_subject_polygon(a)
    c.add_clip_polygon(b)
    c.union :non_zero, :non_zero

    => [[[100.0, 0.0], [0.0, 0.0], [0.0, 47.85714326530613], [-4.999999, 50.0],
         [0.0, 50.0], [0.0, 100.0], [100.0, 100.0], [100.0, 50.0],
         [200.0, 50.0], [100.0, 5.0]]]

  Check out the main Clipper examples for other clip types, winding rules, etc.

