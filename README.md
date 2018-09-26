Name: Ramsey Hashem
UT EID: rah3823
cs login: rhashem

This is my final project for CS354. It implements procedural texture generation using
voronoi diagrams, which outputs to .bmp files that can then be loaded into the Project 1
framework from class.

My code can be found in Voronoi.cpp. The "Final Paper" file has a more in-depth description
along with various examples of what this program can do.

Poisson.bmp is a file containing points generated using a Poisson distribution, which 
was used for the Delaunay graph. Poisson.txt contains the vertices for this.

Voronoi.bmp contains an image of the points for the voronoi diagram. voronoi_diagram.txt 
contains these points.

voronoi_texture.bmp is the final rendered texture image.

In order to run, exectue the commands:
make
./executable

_____________________________________________________________________________

Code for generating Poisson Disk Points:

**Poisson Disk Points Generator**

(C) Sergey Kosarevsky, 2014-2016

@corporateshark sk@linderdaum.com

http://www.linderdaum.com

http://blog.linderdaum.com

=============================

Poisson disk points generator in a single file header-only C++11 library.

=============================

To compile:
	*gcc Poisson.cpp -std=c++11 -lstdc++*

=============================

Algorithm description can be found in "Fast Poisson Disk Sampling in Arbitrary Dimensions"
http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf

Implementation is based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/

=============================

Much of the following Delaunay and Voronoi code from CGAL. More specifically, 
the 2D Triangulation package was utilized. 
 
The code of this package is the result of a long development process. Here follows 
a tentative list of people who added their stone to this package: Jean-Daniel Boissonnat, 
Hervé Brönnimann, Olivier Devillers, Andreas Fabri, Frédéric Fichel, Julia Flötotto, 
Monique Teillaud and Mariette Yvinec. 

https://doc.cgal.org/latest/Triangulation_2/index.html
 

