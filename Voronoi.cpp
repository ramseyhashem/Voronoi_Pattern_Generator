/**
 * \file Voronoi.cpp
 *
 * Code for generating Poisson Disk Points from:
 * Poisson Disk Points Generator example
 *
 * \version 1.1.3
 * \date 10/03/2016
 * \author Sergey Kosarevsky, 2014-2016
 * \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
 */

#include <vector>
#include <iostream>
#include <fstream>
#include <memory.h>

#include <string>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define POISSON_PROGRESS_INDICATOR 1
#include "PoissonGenerator.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <iterator>

using namespace std;

vector<vector<float>> color_wheel;		// color choices for the diagram

///////////////// CGAL typedefs ////////////////////////////////////////////

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point_2;
typedef K::Iso_rectangle_2 Iso_rectangle_2;
typedef K::Segment_2 Segment_2;
typedef K::Ray_2 Ray_2;
typedef K::Line_2 Line_2;
typedef CGAL::Delaunay_triangulation_2<K>  Delaunay_triangulation_2;

///////////////// User selectable parameters ///////////////////////////////

int   ImageSize   = 512;			// generate RGB image [ImageSize x ImageSize]
int   NumPoints   = ImageSize / 2;	// minimal number of points to generate
int   LineWidth   = 5;				// default line width
vector<float> cell_color = {0.0f,0.0f,0.0f};	// color of the voronoi cells
vector<float> edge_color = {1.0f,1.0f,1.0f};	// color of the voronoi edges

////////////////////////////////////////////////////////////////////////////

/* Much of the following Delaunay and Voronoi code from CGAL. More specifically,
 * the 2D Triangulation package was utilized. 
 * 
 * The code of this package is the result of a long development process. Here follows 
 * a tentative list of people who added their stone to this package: Jean-Daniel Boissonnat, 
 * Hervé Brönnimann, Olivier Devillers, Andreas Fabri, Frédéric Fichel, Julia Flötotto, 
 * Monique Teillaud and Mariette Yvinec. 
 *
 * https://doc.cgal.org/latest/Triangulation_2/index.html
 */

//A class to recover Voronoi diagram from stream.
//Rays, lines and segments are cropped to a rectangle
//so that only segments are stored
struct Voronoi_Diagram{
  std::vector<Segment_2> segments;
  Iso_rectangle_2 world;
  
  Voronoi_Diagram(const Iso_rectangle_2& bbox) :
  	world(bbox) {}
  
  template <class RSL>
  void crop_and_extract_segment(const RSL& rsl){
    CGAL::Object obj = CGAL::intersection(rsl,world);
    const Segment_2* s=CGAL::object_cast<Segment_2>(&obj);
    if (s) segments.push_back(*s);
  }
  
  void operator<<(const Ray_2& ray)    { crop_and_extract_segment(ray); }
  void operator<<(const Line_2& line)  { crop_and_extract_segment(line); }
  void operator<<(const Segment_2& seg){ crop_and_extract_segment(seg); }
};

float* g_DensityMap = nullptr;

#if defined( __GNUC__ )
#	define GCC_PACK(n) __attribute__((packed,aligned(n)))
#else
#	define GCC_PACK(n) __declspec(align(n))
#endif // __GNUC__

#pragma pack(push, 1)
struct GCC_PACK( 1 ) sBMPHeader
{
   // BITMAPFILEHEADER
   unsigned short    bfType;
   uint32_t          bfSize;
   unsigned short    bfReserved1;
   unsigned short    bfReserved2;
   uint32_t          bfOffBits;
   // BITMAPINFOHEADER
   uint32_t          biSize;
   uint32_t          biWidth;
   uint32_t          biHeight;
   unsigned short    biPlanes;
   unsigned short    biBitCount;
   uint32_t          biCompression;
   uint32_t          biSizeImage;
   uint32_t          biXPelsPerMeter;
   uint32_t          biYPelsPerMeter;
   uint32_t          biClrUsed;
   uint32_t          biClrImportant;
};
#pragma pack(pop)

void SaveBMP( const char* FileName, const void* RawBGRImage, int Width, int Height )
{
	sBMPHeader Header;

	int ImageSize = Width * Height * 3;

	Header.bfType          = 0x4D * 256 + 0x42;
	Header.bfSize          = ImageSize + sizeof( sBMPHeader );
	Header.bfReserved1     = 0;
	Header.bfReserved2     = 0;
	Header.bfOffBits       = 0x36;
	Header.biSize          = 40;
	Header.biWidth         = Width;
	Header.biHeight        = Height;
	Header.biPlanes        = 1;
	Header.biBitCount      = 24;
	Header.biCompression   = 0;
	Header.biSizeImage     = ImageSize;
	Header.biXPelsPerMeter = 6000;
	Header.biYPelsPerMeter = 6000;
	Header.biClrUsed       = 0;
	Header.biClrImportant  = 0;

	std::ofstream File( FileName, std::ios::out | std::ios::binary );

	File.write( (const char*)&Header, sizeof( Header ) );
	File.write( (const char*)RawBGRImage, ImageSize );

	std::cout << "Saved " << FileName << std::endl;
}

unsigned char* LoadBMP( const char* FileName, int* OutWidth, int* OutHeight )
{
	sBMPHeader Header;

	std::ifstream File( FileName, std::ifstream::binary );

	File.read( (char*)&Header, sizeof( Header ) );

	*OutWidth  = Header.biWidth;
	*OutHeight = Header.biHeight;

	size_t DataSize = 3 * Header.biWidth * Header.biHeight;

	unsigned char* Img = new unsigned char[ DataSize ];

	File.read( (char*)Img, DataSize );

	return Img;
}

void LoadDensityMap( const char* FileName )
{
	std::cout << "Loading density map " << FileName << std::endl;

	int W, H;
	unsigned char* Data = LoadBMP( FileName, &W, &H );

	std::cout << "Loaded ( " << W << " x " << H << " ) " << std::endl;

	if ( W != ImageSize || H != ImageSize )
	{
		std::cout << "ERROR: density map should be " << ImageSize << " x " << ImageSize << std::endl;		

		exit( 255 );
	}

	g_DensityMap = new float[ W * H ];

	for ( int y = 0; y != H; y++ )
	{
		for ( int x = 0; x != W; x++ )
		{
			g_DensityMap[ x + y * W ] = float( Data[ 3 * (x + y * W) ] ) / 255.0f;
		}
	}

	delete[]( Data );
}

void PrintBanner()
{
	std::cout << "Poisson disk points generator" << std::endl;
	std::cout << "Version " << PoissonGenerator::Version << std::endl;
	std::cout << "Sergey Kosarevsky, 2014-2016" << std::endl;
	std::cout << "support@linderdaum.com http://www.linderdaum.com http://blog.linderdaum.com" << std::endl;
	std::cout << std::endl;
	std::cout << "Usage: Poisson [density-map-rgb24.bmp]" << std::endl;
	std::cout << std::endl;
}

/* Ramsey's Code
 * Sets up the user's desired resolution for the window and texture
 */
bool setImageSize() {
	int size = -1;
	bool valid = false;
	do {
		cout << "Enter Desired Image Resolution (ex. '512' for 512x512 image). (Integer Value Between 256-4096): ";
		cin >> size;
		if(cin.fail() || size < 256 || size > 4096) {
			cout << "\nPlease enter a valid resolution." << endl;
			cin.clear();
			cin.ignore(999,'\n'); // the first parameter is just some arbitrarily large value, the second param being the character to ignore till
		} else {
			valid = true;
		}
	} while (!valid);

	assert(size > 255 && size < 4097);
	ImageSize = size;

	return valid;
}

/* Ramsey's Code
 * Sets up the user's desired size of the voronoi cells
 */
bool setCellSize() {
	int cellSize = -1;
	bool valid = false;
	do {
		cout << "Enter Cell Size (Integer Value Between 1-1000): ";
		cin >> cellSize;
		if(cin.fail() || cellSize < 1 || cellSize > 1000) {
			cout << "\nPlease enter a valid cell size." << endl;
			cin.clear();
			cin.ignore(999,'\n'); 
		} else {
			valid = true;
		}		
	} while (!valid);

	assert(cellSize > 0 && cellSize < 1001);
	double result = (1001 - cellSize) / 500.0 * ImageSize + 3;
	NumPoints = static_cast<int>(result);
	return valid;
}

/* Ramsey's Code
 * Sets up the user's desired line width for the voronoi diagram
 */
bool setLineWidth() {
	int lineWidth = -1;
	bool valid = false;
	do {
		cout << "Enter Line Width (Integer Between 1-30 pixels): ";
		cin >> lineWidth;
		if(cin.fail() || lineWidth < 1 || lineWidth > 30) {
			cout << "\nPlease enter a valid line width." << endl;
			cin.clear();
			cin.ignore(999,'\n');
		} else {
			valid = true;
		}
	} while (!valid);

	assert(lineWidth > 0 && lineWidth < 31);
	LineWidth = lineWidth;
	return valid;
}

/* Ramsey's Code
 * Sets up the user's desired color for the voronoi diagram
 */
vector<float> setUserColor(int type) {
	float red = 0.0;
	float green = 0.0;
	float blue = 0.0;
	float alpha = 0.0;

	int choice = -1;

	bool valid = false;
	do {
		cout << "Choose a color from the following list and enter the corresponding number for it:\n";
		cout << "0: white\n1: black\n2: darkRed\n3: red\n4: pink\n5: deepPink\n6: coral\n7: orange\n8: gold\n9: yellow\n10: lavendar\n11: violet\n";
		cout << "12: blueViolet\n13: purple\n14: indigo\n15: lime\n16: springGreen\n17: seaGreen\n18: green\n19: darkGreen\n20: olive\n21: teal\n22: cyan\n";
		cout << "23: aquamarine\n24: turquoise\n25: steelBlue\n26: skyBlue\n27: royalBlue\n28: blue\n29: navy\n30: wheat\n31: tan\n32: chocolate\n33: sienna\n";
		cout << "34: brown\n35: honeydew\n36: azure\n37: beige\n38: silver\n39: gray\n40: dimgray\n";

		cin >> choice;

		if(cin.fail() || choice < 0 || choice > 40) {
			cout << "\nPlease enter a valid color." << endl;
			cin.clear();
			cin.ignore(999,'\n');
		} else {
			valid = true;
		}
	} while (!valid);

	assert (choice > -1 && choice < 41);
	vector<float> color = color_wheel[choice];

	if(type == 0) {
		// Cell color
		cell_color = color;
	} else if(type == 1) {
		// Edge color
		edge_color = color;
	}
	return color;	
}

/* Ramsey's Code
 * Creates the color wheel, which contains colors the user can easily use for their voronoi diagram
 */
void createColorWheel() {
	vector<float> white = {1.0f,1.0f,1.0f};
	vector<float> black = {0.0f,0.0f,0.0f};
	vector<float> darkRed = {0.545f, 0.000f, 0.000f};
	vector<float> red = {1.0f,0.0f,0.0f};
	vector<float> pink = {1.000f, 0.753f, 0.796f};
	vector<float> deepPink = {1.000f, 0.078f, 0.576f};
	vector<float> coral = {1.000f, 0.498f, 0.314f};
	vector<float> orange = {1.000f, 0.647f, 0.000f};
	vector<float> gold = {1.000f, 0.843f, 0.000f};
	vector<float> yellow = {1.0f,1.0f,0.0f};
	vector<float> lavendar = {0.902f, 0.902f, 0.980f};
	vector<float> violet = {0.933f, 0.510f, 0.933f};
	vector<float> blueViolet = {0.541f, 0.169f, 0.886f};
	vector<float> purple = {0.502f, 0.000f, 0.502f};
	vector<float> indigo = {0.294f, 0.000f, 0.510f};
	vector<float> lime = {0.0f,1.0f,0.0f};	
	vector<float> springGreen = {0.000f, 1.000f, 0.498f};
	vector<float> seaGreen = {0.180f, 0.545f, 0.341f};
	vector<float> green = {0.000f, 0.502f, 0.000f};
	vector<float> darkGreen = {0.000f, 0.392f, 0.000f};
	vector<float> olive = {0.502f, 0.502f, 0.000f};
	vector<float> teal = {0.000f, 0.502f, 0.502f};
	vector<float> cyan = {0.000f, 1.000f, 1.000f};
	vector<float> aquamarine = {0.498f, 1.000f, 0.831f};
	vector<float> turquoise = {0.251f, 0.878f, 0.816f};
	vector<float> steelBlue = {0.275f, 0.510f, 0.706f};		
	vector<float> skyBlue = {0.529f, 0.808f, 0.922f};
	vector<float> royalBlue = {0.255f, 0.412f, 0.882f};
	vector<float> blue = {0.0f, 0.0f, 1.0f};
	vector<float> navy = {0.000f, 0.000f, 0.502f};
	vector<float> wheat = {0.961f, 0.871f, 0.702f};
	vector<float> tan = {0.824f, 0.706f, 0.549f};
	vector<float> chocolate = {0.824f, 0.412f, 0.118};
	vector<float> sienna = {0.627f, 0.322f, 0.176f};
	vector<float> brown = {0.647f, 0.165f, 0.165f};
	vector<float> honeydew = {0.941f, 1.000f, 0.941f};
	vector<float> azure = {0.941f, 1.000f, 1.000f};
	vector<float> beige = {0.961f, 0.961f, 0.863f};
	vector<float> silver = {0.753f, 0.753f, 0.753f};
	vector<float> gray = {0.502f, 0.502f, 0.502f};
	vector<float> dimgray = {0.412f, 0.412f, 0.412f};	

	color_wheel.push_back(white);
	color_wheel.push_back(black);
	color_wheel.push_back(darkRed);
	color_wheel.push_back(red);	
	color_wheel.push_back(pink);
	color_wheel.push_back(deepPink);
	color_wheel.push_back(coral);
	color_wheel.push_back(orange);
	color_wheel.push_back(gold);
	color_wheel.push_back(yellow);
	color_wheel.push_back(lavendar);
	color_wheel.push_back(violet);	
	color_wheel.push_back(blueViolet);
	color_wheel.push_back(purple);
	color_wheel.push_back(indigo);
	color_wheel.push_back(lime);
	color_wheel.push_back(springGreen);
	color_wheel.push_back(seaGreen);
	color_wheel.push_back(green);
	color_wheel.push_back(darkGreen);	
	color_wheel.push_back(olive);
	color_wheel.push_back(teal);
	color_wheel.push_back(cyan);
	color_wheel.push_back(aquamarine);
	color_wheel.push_back(turquoise);
	color_wheel.push_back(steelBlue);
	color_wheel.push_back(skyBlue);
	color_wheel.push_back(royalBlue);	
	color_wheel.push_back(blue);
	color_wheel.push_back(navy);
	color_wheel.push_back(wheat);
	color_wheel.push_back(tan);
	color_wheel.push_back(chocolate);
	color_wheel.push_back(sienna);	
	color_wheel.push_back(brown);
	color_wheel.push_back(honeydew);
	color_wheel.push_back(azure);
	color_wheel.push_back(beige);
	color_wheel.push_back(silver);
	color_wheel.push_back(gray);
	color_wheel.push_back(lavendar);
	color_wheel.push_back(dimgray);		
}

/* Some of Ramsey's Code
 * Actually displays the voronoi diagram and outputs to a .bmp
 */
int display(int resolution) {
    GLFWwindow *window;
    
    // Initialize the library
    if ( !glfwInit( ) )
    {
        return -1;
    }
    
    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow( resolution, resolution, "Voronoi Diagram", NULL, NULL );
    
    if ( !window )
    {
        glfwTerminate( );
        return -1;
    }
    
    // Make the window's context current
    glfwMakeContextCurrent( window );
    glViewport( 0.0f, 0.0f, resolution, resolution ); // specifies the part of the window to which OpenGL will draw (in pixels), convert from normalised to pixels
    glMatrixMode( GL_PROJECTION ); // projection matrix defines the properties of the camera that views the objects in the world coordinate frame. Here you typically set the zoom factor, aspect ratio and the near and far clipping planes
    glLoadIdentity( ); // replace the current matrix with the identity matrix and starts us a fresh because matrix transforms such as glOrpho and glRotate cumulate, basically puts us at (0, 0, 0)
    glOrtho( 0, resolution, 0, resolution, 0, 1 ); // essentially set coordinate system
    glMatrixMode( GL_MODELVIEW ); // (default matrix mode) modelview matrix defines how your objects are transformed (meaning translation, rotation and scaling) in your world
    glLoadIdentity( ); // same as above comment


    // Read in the voronoi_diagram.txt file, which contains the segments of the voronoi diagram, and use those to create lines.
    string s;
    ifstream myfile("voronoi_diagram.txt");
    int numSegments;

    getline(myfile, s);
    istringstream iss(s);
    iss >> numSegments;

    GLfloat lineVertices[numSegments * 6];

    // create the array of line segment vertices
    double x1, y1, x2, y2;
    int count = 0;
    while(getline(myfile, s)) {
        istringstream sin(s);
        sin >> x1 >> y1 >> x2 >> y2;            
        lineVertices[count]     = (int)(x1 * resolution);
        lineVertices[count + 1] = (int)(y1 * resolution);
        lineVertices[count + 2] = 0;
        lineVertices[count + 3] = (int)(x2 * resolution);
        lineVertices[count + 4] = (int)(y2 * resolution); 
        lineVertices[count + 5] = 0;                   
        count += 6;
    }
    myfile.close();   
           
    // Loop until the user closes the window
    while ( !glfwWindowShouldClose( window ) )
    {
        glClear( GL_COLOR_BUFFER_BIT );
        
        // Do rendering

        // Get the cell color that the user selected
        float cell_r = cell_color[0];
        float cell_g = cell_color[1];
        float cell_b = cell_color[2];
		glClearColor(cell_r, cell_g, cell_b, 0.0f);	// cells
        glEnable( GL_LINE_SMOOTH );
        glPushAttrib( GL_LINE_BIT );
        glLineWidth( LineWidth );
        glEnableClientState( GL_VERTEX_ARRAY );

        //Get the edge color that the user selected
        float edge_r = edge_color[0];
        float edge_g = edge_color[1];
        float edge_b = edge_color[2];        
        glColor4f(edge_r,edge_g,edge_b,0.0f);		// edges
        glVertexPointer( 3, GL_FLOAT, 0, lineVertices );
        glDrawArrays( GL_LINES, 0, numSegments * 2);	// Draw the voronoi diagram
        glDisableClientState( GL_VERTEX_ARRAY );
        glPopAttrib( );
        glDisable( GL_LINE_SMOOTH );

        // Swap front and back buffers
        glfwSwapBuffers( window );
        
        // Poll for and process events
        glfwPollEvents( );
       
    }

    // Output the voronoi diagram to voronoi_texture.bmp
	unsigned char image[resolution * resolution * 3];
	glReadPixels(0, 0, resolution, resolution, GL_BGR, GL_UNSIGNED_BYTE, image);

	SaveBMP( "voronoi_texture.bmp", image, resolution, resolution );

    glfwTerminate( );

	return 0;    
}

/* Largely Ramsey's Code
 * Main program execution
 */
int main( int argc, char** argv )
{
	PrintBanner();

	// Get user inputs
	createColorWheel();
	setImageSize();
	setCellSize();
	setLineWidth();
	cout << "\nChoose a color for the cells.\n";
	setUserColor(0);
	cout << "\nChoose a color for the edges.\n";
	setUserColor(1);

	if ( argc > 1 )
	{
		LoadDensityMap( argv[1] );
	}

	PoissonGenerator::DefaultPRNG PRNG;

	const auto Points = PoissonGenerator::GeneratePoissonPoints( NumPoints, PRNG );		// Generate poisson points

	// Output poisson points to Poisson.bmp
	size_t DataSize = 3 * ImageSize * ImageSize;

	unsigned char* Img = new unsigned char[ DataSize ];

	memset( Img, 0, DataSize );

	int x, y, Base;
	for ( auto i = Points.begin(); i != Points.end(); i++ )
	{
		x = int( i->x * ImageSize );
		y = int( i->y * ImageSize );
		if ( g_DensityMap )
		{
			// dice
			float R = PRNG.RandomFloat();
			float P = g_DensityMap[ x + y * ImageSize ];
			if ( R > P ) continue;
		}
		Base = 3 * (x + y * ImageSize);
		Img[ Base+0 ] = Img[ Base+1 ] = Img[ Base+2 ] = 255;
	}

	SaveBMP( "Poisson.bmp", Img, ImageSize, ImageSize );

	delete[]( Img );

	// Output points to Poisson.txt
	std::ofstream File( "Poisson.txt", std::ios::out );	

	File << "NumPoints = " << Points.size() << std::endl;

	// Create the delaunay graph from the poisson points
	std::vector<Point_2> delaunay_points;

	for ( const auto& p : Points )
	{
		File << p.x << " " << p.y << std::endl;
		delaunay_points.push_back(Point_2(p.x, p.y));
	}

	Delaunay_triangulation_2 dt;

	dt.insert(delaunay_points.begin(), delaunay_points.end());

	Iso_rectangle_2 bbox(0,0,1,1);
	Voronoi_Diagram voronoi_diagram(bbox);

	// Create the voronoi diagram by taking the dual of the delaunay graph
	dt.draw_dual(voronoi_diagram);

	// Write the voronoi diagram to voronoi_diagram.txt
	ofstream voronoi_file;
	voronoi_file.open("voronoi_diagram.txt");

	voronoi_file << voronoi_diagram.segments.size() << endl;

	for(int j = 0; j < voronoi_diagram.segments.size(); ++j) {
		voronoi_file << voronoi_diagram.segments[j] << endl;
	}

	voronoi_file.close();

	// Output Voronoi points to Voronoi.bmp
	size_t DataSize2 = 3 * ImageSize * ImageSize;

	unsigned char* Img2 = new unsigned char[ DataSize2 ];

	memset( Img2, 0, DataSize2 );

	int x1, x2, y1, y2, Base1, Base2;
	// Each segment contains the x and y coords for two points, source, and target.
	// coords are in [0,1] and therefore must be scaled by ImageSize.
	for ( int k = 0; k < voronoi_diagram.segments.size(); ++k )
	{
		x1 = int( voronoi_diagram.segments[k].source().x() * ImageSize );
		y1 = int( voronoi_diagram.segments[k].source().y() * ImageSize );
		Base1 = 3 * (x1 + y1 * ImageSize);
		Img2[ Base1+0 ] = Img2[ Base1+1 ] = Img2[ Base1+2 ] = 255;

		x2 = int( voronoi_diagram.segments[k].target().x() * ImageSize );
		y2 = int( voronoi_diagram.segments[k].target().y() * ImageSize );		
		Base2 = 3 * (x2 + y2 * ImageSize);
		Img2[ Base2+0 ] = Img2[ Base2+1 ] = Img2[ Base2+2 ] = 255;

	}

	SaveBMP( "Voronoi.bmp", Img2, ImageSize, ImageSize );
	delete[]( Img2 );

	// Display the voronoi diagram
	display(ImageSize);	


	return 0;
}
