#  
# Copyright (C) 2016-2017 Laurent GAUTHIER <laurent.gauthier@soccasys.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
#

import sys

class PatternGenerator:
    def __init__(self, size):
        self.size = size
        self.margin = 50

    def print_circle(self, x, y):
        print """<circle cx="%d" cy="%d" r="%d" stroke="black" stroke-width="0" fill="rgba(0,0,0,255)" />""" % (x, y, self.size/2)
    def print_capsule_zero(self, x, y):
        print """<circle cx="%d" cy="%d" r="%d" stroke="black" stroke-width="%d" fill="none" />""" % (x, y, self.size, self.size)
    def print_capsule_one(self, x, y):
        print """<circle cx="%d" cy="%d" r="%d" stroke="black" stroke-width="%d" fill="none" />""" % (x, y, self.size*2, self.size)
        self.print_circle(x, y)
    def print_capsule_two(self, x1, y1, x2, y2):
        print """<path d="M%d %d
           A %d %d 0 0 1 %d %d
           L %d %d
           A %d %d 0 0 1 %d %d
           L %d %d Z" stroke="black" fill="none" stroke-width="%d" />""" % (x1-2*self.size, y1, \
                                                                              2*self.size, 2*self.size, x1+2*self.size, y1, \
                                                                              x1+2*self.size, y2, \
                                                                              2*self.size, 2*self.size, x2-2*self.size, y2, \
                                                                              x1-2*self.size, y1, self.size)
        self.print_circle(x1, y1)
        self.print_circle(x2, y2)
    def print_capsule_three(self, x1, y1, x2, y2, x3, y3):
        print """<path d="M%d %d
           A %d %d 0 0 1 %d %d
           L %d %d
           A %d %d 0 0 1 %d %d
           L %d %d Z" stroke="black" fill="none" stroke-width="%d" />""" % (x1, y1-2*self.size, \
                                                                              2*self.size, 2*self.size, x1, y1+2*self.size, \
                                                                              x3, y1+2*self.size, \
                                                                              2*self.size, 2*self.size, x3, y3-2*self.size, \
                                                                              x1, y1-2*self.size, self.size)
        self.print_circle(x1, y1)
        self.print_circle(x2, y2)
        self.print_circle(x3, y3)
    def print_line(self, x1, y1, x2, y2):
        print """<line x1="%d" y1="%d" x2="%d" y2="%d" style="stroke:rgb(0,0,0);stroke-width:%d" />""" % (x1, y1, x2, y2, self.size)
    def pprint(self, id):
        header = """<?xml version="1.0" standalone="yes"?>
          <svg version="1.1"
               baseprofile="full"
               xmlns="http://www.w3.org/2000/svg"
               xmlns:xlink="http://www.w3.org/1999/xlink"
               xmlns:ev="http://www.w3.org/2001/xml-events"
               height="1000" width="1000">
          """
        print header
        self.print_capsule_zero(self.margin+(self.size*5)/2, 1000-self.margin-(self.size*5)/2)
        self.print_line(self.margin+(self.size*5)/2, 1000-self.margin-(self.size*7)/2,
                                     self.margin+(self.size*5)/2, self.margin+(self.size*9)/2)
        self.print_capsule_one(self.margin+(self.size*5)/2, self.margin+(self.size*5)/2)
        self.print_line(self.margin+(self.size*9)/2, self.margin+(self.size*5)/2,
                                     1000-self.margin-(self.size*9)/2, self.margin+(self.size*5)/2)
        self.print_capsule_two(1000-self.margin-(self.size*5)/2, self.margin+(self.size*5)/2, 1000-self.margin-(self.size*5)/2, self.margin+(self.size*9)/2)
        self.print_line(1000-self.margin-(self.size*5)/2, self.margin+(self.size*13)/2,
                                     1000-self.margin-(self.size*5)/2, 1000-self.margin-(self.size*9)/2)
        y = 1000-self.margin-(self.size*5)/2
        x = 1000-self.margin-(self.size*5)/2
        self.print_capsule_three(x, y, x-self.size*2,y, x-self.size*4, y)
        self.print_line(self.margin+(self.size*7)/2, 1000-self.margin-(self.size*5)/2,
                                     1000-self.margin-(self.size*17)/2, 1000-self.margin-(self.size*5)/2)
        if (id & 1) != 0:
            self.print_circle(self.margin+(self.size*12)/2, self.margin+(self.size*8)/2)
        if (id & 2) != 0:
            self.print_circle(self.margin+(self.size*12)/2, self.margin+(self.size*12)/2)
        if (id & 4) != 0:
            self.print_circle(self.margin+(self.size*8)/2, self.margin+(self.size*12)/2)
        print "</svg>"

generator = PatternGenerator(int(sys.argv[1]))
generator.pprint(int(sys.argv[2]))
