#shader vertex
#version 330 core

layout(location = 0) in vec4 position;

void main()
{
	gl_Position = position;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_Color;
uniform vec4 u_holeColor;
uniform int u_sqrSize;
uniform float u_gapSize;
uniform float u_holeSize;

in vec4 gl_FragCoord;

void main()
{
	float gridX = mod(((float(gl_FragCoord.x)) / float(u_sqrSize)), 1);
	float gridY = mod(((float(gl_FragCoord.y)) / float(u_sqrSize)), 1);

	float holeX = gridX - 0.5;
	float holeY = gridY - 0.5;

	//Gap between squares
	if (
		(gridX < (u_gapSize / u_sqrSize)) ||
		((1.0 - gridX) < (u_gapSize / u_sqrSize)) ||
		(gridY < (u_gapSize / u_sqrSize)) ||
		((1.0 - gridY) < (u_gapSize / u_sqrSize))
		)
	{
		gl_FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		return;
	}
	
	//Hole in the middle of the square
	if (
		((holeX) < (u_holeSize / u_sqrSize)) &&
		((-holeX) < (u_holeSize / u_sqrSize)) &&
		((holeY) < (u_holeSize / u_sqrSize)) &&
		((-holeY) < (u_holeSize / u_sqrSize))
		)
	{
		//gl_FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		gl_FragColor = u_holeColor;
		return;
	}
	gl_FragColor = u_Color;
};