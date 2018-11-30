#ifndef ANIMMESH
#define ANIMMESH


#include <iostream>

#include "GL\glew.h"
#include "glm\glm.hpp"
#include "assimp\Importer.hpp"
#include "Shader.h"

#include <vector>
#include <string>


using namespace std;
typedef unsigned int uint;
#define NUM_BONES_PER_VEREX 4

struct AnimVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 text_coords;
};

struct AnimTexture
{
	GLuint id;
	string type;
	aiString path;
};

struct BoneMatrix
{
	aiMatrix4x4 offset_matrix;
	aiMatrix4x4 final_world_transform;

};

struct VertexBoneData
{
	uint ids[NUM_BONES_PER_VEREX];   // we have 4 bone ids for EACH vertex & 4 weights for EACH vertex
	float weights[NUM_BONES_PER_VEREX];

	VertexBoneData()
	{
		memset(ids, 0, sizeof(ids));    // init all values in array = 0
		memset(weights, 0, sizeof(weights));
	}

	void addBoneData(uint bone_id, float weight);
};

class AnimMesh
{
public:
	AnimMesh(vector<AnimVertex> vertic, vector<GLuint> ind, vector<AnimTexture> textur, vector<VertexBoneData> bone_id_weights);
	AnimMesh() {};
	~AnimMesh();

	// Render mesh
	void Draw(GLuint shaders_program);
	void DrawVMesh(Shader shader)
	{

		// Draw mesh
		glBindVertexArray(this->VVAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Always good practice to set everything back to defaults once configured.
		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

	}
	void DrawDMesh(Shader shader)
	{
		// Bind appropriate textures
		GLuint diffuseNr = 1;
		GLuint specularNr = 1;

		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit before binding
											  // Retrieve texture number (the N in diffuse_textureN)
			stringstream ss;
			string number;
			string name = this->textures[i].type;

			if (name == "texture_diffuse")
			{
				ss << diffuseNr++; // Transfer GLuint to stream
			}
			else if (name == "texture_specular")
			{
				ss << specularNr++; // Transfer GLuint to stream
			}

			number = ss.str();
			// Now set the sampler to the correct texture unit
			glUniform1i(glGetUniformLocation(shader.Program, (name + number).c_str()), i);
			// And finally bind the texture
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}


		// Draw mesh
		glBindVertexArray(this->DVAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Always good practice to set everything back to defaults once configured.
		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

private:
	//Mesh data
	vector<AnimVertex> vertices;
	vector<GLuint> indices;
	vector<AnimTexture> textures;
	vector<VertexBoneData> bones_id_weights_for_each_vertex;

	//buffers
	GLuint VAO;
	GLuint VBO_vertices;
	GLuint VBO_bones;
	GLuint EBO_indices;

	GLuint VVAO, VVBO, VEBO;

	GLuint DVAO, DVBO, DEBO;


	void setupVMesh()
	{
		// Create buffers/arrays
		glGenVertexArrays(1, &this->VVAO);
		glGenBuffers(1, &this->VVBO);
		glGenBuffers(1, &this->VEBO);

		glBindVertexArray(this->VVAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->VVBO);
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(AnimVertex), &this->vertices[0], GL_STATIC_DRAW);


		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->VEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimVertex), (GLvoid *)0);

		glBindVertexArray(0);
	}

	void setupDMesh()
	{
		// Create buffers/arrays
		glGenVertexArrays(1, &this->DVAO);
		glGenBuffers(1, &this->DVBO);
		glGenBuffers(1, &this->DEBO);

		glBindVertexArray(this->DVAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->DVBO);
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(AnimVertex), &this->vertices[0], GL_STATIC_DRAW);


		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->DEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimVertex), (GLvoid *)0);

		// Vertex Texture Coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AnimVertex), (GLvoid *)offsetof(AnimVertex, normal));

		// Vertex Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(AnimVertex), (GLvoid *)offsetof(AnimVertex, text_coords));


		glBindVertexArray(0);
	}
	//inititalize buffers
	void SetupMesh();
};

#endif // !MESH