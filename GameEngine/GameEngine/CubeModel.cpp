#include <sb6.h>
#include <assert.h>

#include "CubeModel.h"
#include "ModelFileHeader.h"
#include "File.h"
#include "BoundingSphere.h"
#include "Trace.h"

struct Vert_xyzuvn
{
	float x;
	float y;
	float z;
	float u;
	float v;
	float nx;
	float ny;
	float nz;
};

struct Tri_index
{
	unsigned int v0;
	unsigned int v1;
	unsigned int v2;
};



void CubeModel::privCreateVAO( const char * const modelFileName )
{

	FileHandle fh2;
	FileError  ferror;

	ferror = File::open(fh2, modelFileName, FileMode::READ);
	assert(ferror == FileError::SUCCESS);

	// Read the Hdr
	ModelFileHeader modelHdr;
		ferror = File::read(fh2, &modelHdr, sizeof(ModelFileHeader));
		assert(ferror == FileError::SUCCESS);

	// Using the hdr, allocate the space for the buffers
	this->numVerts = modelHdr.numVerts;
	this->numTris = modelHdr.numTriList;

	// allocate buffers
	Vert_xyzuvn  *cubeData = new Vert_xyzuvn[modelHdr.numVerts];
	Tri_index *triList = new Tri_index[modelHdr.numTriList];

	
	// Read verts
		ferror = File::seek(fh2, FileSeek::BEGIN, modelHdr.vertBufferOffset);
		assert(ferror == FileError::SUCCESS);

		ferror = File::read(fh2, cubeData, sizeof(Vert_xyzuvn) * this->numVerts);
		assert(ferror == FileError::SUCCESS);

	// Read trilist
		ferror = File::seek(fh2, FileSeek::BEGIN, modelHdr.triListBufferOffset);
		assert(ferror == FileError::SUCCESS);

		ferror = File::read(fh2, triList, sizeof(Tri_index) * this->numTris);
		assert(ferror == FileError::SUCCESS);

	// File close
		ferror = File::close(fh2);
		assert(ferror == FileError::SUCCESS);


	// Create a VAO
	glGenVertexArrays(1, &this->vao);
	assert( this->vao != 0 );
	glBindVertexArray(this->vao);

	// Create a VBO
    glGenBuffers(2, &this->vbo[0]);
	assert( this->vbo[0] != 0 );

	// Find Bounding Volume - quick hack
	Sphere sphere;
	Vect *pVerts = new Vect[this->numVerts];
	for (int i = 0; i < this->numVerts; i++)
	{
		pVerts[i].set(cubeData[i].x, cubeData[i].y, cubeData[i].z);
		//Trace::out("%d:  %f %f %f\n",i,pVerts[i][x], pVerts[i][y], pVerts[i][z]);
	}

	RitterSphere(sphere, pVerts, this->numVerts);
//	Trace::out("Ritter: cntr:%f %f %f  rad: %f ", sphere.cntr[x], sphere.cntr[y], sphere.cntr[z], sphere.rad);
	
    // Load the combined data: ---------------------------------------------------------

		glBindBuffer(GL_ARRAY_BUFFER, this->vbo[0]);

	// load the data to the GPU
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vert_xyzuvn) * this->numVerts, cubeData, GL_STATIC_DRAW);
		
	// Define an array of generic vertex attribute data
	// Vert data is location: 0  (used in vertex shader)
	// todo make a table or enum
			void *offsetVert = (void *)((unsigned int)&cubeData[0].x - (unsigned int)cubeData);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert_xyzuvn), offsetVert);
			glEnableVertexAttribArray(0);

	// Texture data is location: 1  (used in vertex shader)
			void *offsetTex = (void *)((unsigned int)&cubeData[0].u - (unsigned int)cubeData);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vert_xyzuvn), offsetTex);
			glEnableVertexAttribArray(1);

       // normals data in location 2 (used in vertex shader
			void *offsetNorm = (void *)((unsigned int)&cubeData[0].nx - (unsigned int)cubeData);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vert_xyzuvn), offsetNorm);
			glEnableVertexAttribArray(2);

	  // Load the index data: ---------------------------------------------------------
	
			/* Bind our 2nd VBO as being the active buffer and storing index ) */
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);

			/* Copy the index data to our buffer */
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Tri_index) * this->numTris, triList, GL_STATIC_DRAW);

	// Free buffers - they were copied into the GPU
		delete[] cubeData;
		delete[] triList;

}


CubeModel::CubeModel( const char * const modelFileName )
	: Model()
{
	this->privCreateVAO( modelFileName );
}

CubeModel::~CubeModel()
{
	// remove anything dynamic here
}