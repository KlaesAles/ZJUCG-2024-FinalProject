#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	int boneIDs[MAX_BONE_INFLUENCE];
	float weights[MAX_BONE_INFLUENCE];
};

struct Texture {
	unsigned int id;
	string type;
	string path;
};

class Mesh {
public:
	// mesh data
	vector<Vertex> vertices;
	vector<unsigned int> indices;

	// Texture vectors grouped by texture type
    vector<Texture> diffuseTextures;
    vector<Texture> specularTextures;
    vector<Texture> normalTextures;
    vector<Texture> heightTextures;

	unsigned int VAO;

	// constructor
	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;

		// Group textures by type
        for (const auto& tex : textures) {
            if (tex.type == "texture_diffuse")
                diffuseTextures.push_back(tex);
            else if (tex.type == "texture_specular")
                specularTextures.push_back(tex);
            else if (tex.type == "texture_normal")
                normalTextures.push_back(tex);
            else if (tex.type == "texture_height")
                heightTextures.push_back(tex);
        }

		// now that we have all the required data, set the vertex buffers and its attribute pointers.
		setupMesh();
	}

	void Draw(GLint shader)
	{
		// Bind texture arrays to shader
        unsigned int textureUnit = 0;

        // Set diffuse textures
        for (unsigned int i = 0; i < diffuseTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.diffuse[" + to_string(i) + "]").c_str()), textureUnit);
			glBindTexture(GL_TEXTURE_2D, diffuseTextures[i].id);
            textureUnit++;
        }

        // Set specular textures
        for (unsigned int i = 0; i < specularTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.specular[" + to_string(i) + "]").c_str()), textureUnit);
			glBindTexture(GL_TEXTURE_2D, specularTextures[i].id);
            textureUnit++;
        }

        // Set normal textures
        for (unsigned int i = 0; i < normalTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.normal[" + to_string(i) + "]").c_str()), textureUnit);
			glBindTexture(GL_TEXTURE_2D, normalTextures[i].id);
            textureUnit++;
        }

        // Set height textures
        for (unsigned int i = 0; i < heightTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("height[" + to_string(i) + "]").c_str()), textureUnit);
			glBindTexture(GL_TEXTURE_2D, heightTextures[i].id);
            textureUnit++;
        }

		glUniform1i(glGetUniformLocation(shader, "material.diffuseCount"), diffuseTextures.size());
		glUniform1i(glGetUniformLocation(shader, "material.specularCount"), specularTextures.size());
		glUniform1i(glGetUniformLocation(shader, "material.normalCount"), normalTextures.size());
		glUniform1i(glGetUniformLocation(shader, "heightCount"), heightTextures.size());


        // Draw the mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Reset the active texture unit
        glActiveTexture(GL_TEXTURE0);
	}

private:
	// render data 
	unsigned int VBO, EBO;

	// initializes all the buffer objects/arrays
	void setupMesh()
	{
		// create buffers/arrays
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		// vertex bone ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
		// vertex weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

		glBindVertexArray(0);
	}
};

#endif
