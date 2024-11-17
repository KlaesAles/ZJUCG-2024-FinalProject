#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// The data of rendering target
float vertices[] = {
	//first Triangle
	  0.5f,  0.5f, 0.0f,//right top
	  0.5f, -0.5f, 0.0f,//right bottom
	 -0.5f, -0.5f, 0.0f,//left bottom
	 -0.5f,  0.5f, 0.0f //left top
};

// The indices of the vertices
unsigned int indices[] = {
	0, 1, 3, //first Triangle
	1, 2, 3  //second Triangle
};

// The hard-coded Vertex Shader Write with GLSL
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

// The hard-coded Fragement Shader Write with GLSL
const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\0";

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
		return - 1;
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); //Enable the Vertex Attribute


	//Crate a Shader object
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, & vertexShaderSource, NULL); //Bind the Shader code to the Object
	glCompileShader(vertexShader); // Compile the shader

	// Similar to above
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	//Check whether the compliation is successful 
	int success1;
	char infoLog1[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success1);
	if (!success1)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog1);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog1 << std::endl;
	}

	int success2;
	char infoLog2[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success1);
	if (!success1)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog2);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog2 << std::endl;
	}

	//Create a Shader Program
	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader); //Attach the Shader to the Program
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram); //Link the Program

	//Check whether the linking is successful
	int success3;
	char infoLog3[512];
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success3);
	if (!success3)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog3);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog3 << std::endl;
	}

	//Activate the Shader Program
	glUseProgram(shaderProgram);

	//Delete the Shader Object
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


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
		glUseProgram(shaderProgram);
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
	glDeleteProgram(shaderProgram);



	glfwTerminate();
	return 0;
}