#pragma once

namespace Math
{
	struct Vector3
	{
		float x, y, z;
		
	public:

		Vector3()
			:x(0.0f)
			,y(0.0f)
			,z(0.0f)
		{

		}

		Vector3(float inX, float inY, float inZ)
			:x(inX)
			,y(inY)
			,z(inZ)
		{
		}


	};
}