#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <shader.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// The data of rendering target
float vertices[] = {
	//first Triangle    //Color
	  0.0f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f,//middle top
	  0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,//right bottom
	 -0.5f, -0.5f, 0.0f,-1.0f,-1.0f,-1.0f,//left bottom
};

// The indices of the vertices
unsigned int indices[] = {
	0, 1, 2, //first Triangle
};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Create a window object
	GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "FAiled to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Filed to initAlize GLaD" << std::endl;
		return -1;
	}

	//Create element buffer object
	unsigned int EBO;
	glGenBuffers(1, &EBO);

	// Define the Vertex Buffer Objects
	unsigned int VBO;
	glGenBuffers(1, &VBO);

	//Create a Vertex Array Object
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	//Bind the VAO
	glBindVertexArray(VAO);
	//Copy the vertices array in a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Bind the VBO to GL_ARRAY_BUFFER
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Write the data to the VBO
	//Copy the indice array in a buffer for OpenGL to use
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Bind the EBO to GL_ELEMENT_ARRAY_BUFFER
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Write the data to the EBO
	//set up vertex data and configure vertex attributes
	//location data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); //Enable the Vertex Attribute
	//color data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1); 

	Shader ourShader("./shader/shader.vs", "./shader/shader.fs");

	glViewport(0, 0, 800, 600);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Rendering Loop
	while (!glfwWindowShouldClose(window))
	{
		// Input
		processInput(window);

		//Rendering Command
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //State setting function
		glClear(GL_COLOR_BUFFER_BIT); //State usage function

		//Draw the Triangle
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//Wireframe mode

		ourShader.use();

		// refresh the color of the triangle
		float timeValue = glfwGetTime();
		float redValue = sin(timeValue);
		float greenValue = cos(timeValue);
		float blueValue = sin(timeValue - 3.14/2);
		int vertexColorLocation = glGetUniformLocation(ourShader.ID, "ourColor");
		glUseProgram(ourShader.ID);
		glUniform4f(vertexColorLocation, redValue, greenValue, blueValue, 1.0f);

		float offset = (sin(timeValue) / 2.0f);
		ourShader.setFloat("xOffset", offset);

		glBindVertexArray(VAO);//Bind the needed VAO

		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// glBindVertexArray(0);//Unbind the VAO (optional)

		//Check and invoke the event, interchange the buffer
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//Delete the VAO, VBO, EBO, shaderProblem
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);



	glfwTerminate();
	return 0;
}