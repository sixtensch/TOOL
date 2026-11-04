#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



// The same rotation, whichever sign, by where it takes each axis
static b8 SameRotation(q4 a, q4 b, f32 tolerance)
{
	const v3 axes[] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
	for (v3 axis : axes)
	{
		v3 difference = a * axis - b * axis;
		if (V3Length(difference) > tolerance)
			return false;
	}
	return true;
}

static void TestEuler()
{
	const v3 axes[] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
	const f32 angles[] = { -F32_PI, -2.5f, -F32_PI * 0.5f, -1.2f, -0.3f, 0.0f, 0.4f, 1.1f, F32_PI * 0.5f, 2.2f, 3.0f };

	for (f32 pitch : angles)
		for (f32 yaw : angles)
			for (f32 roll : angles)
			{
				// Roll, then pitch, then yaw, as M4Rotation
				q4 q = Q4Euler(pitch, yaw, roll);
				q4 composed = Q4Axis(axes[1], yaw) * Q4Axis(axes[0], pitch) * Q4Axis(axes[2], roll);
				TOOL_ASSERT(SameRotation(q, composed, 1e-5f));

				m4 matrix = M4Rotation(pitch, yaw, roll);
				for (v3 axis : axes)
				{
					v4 turned = matrix * v4 { axis.x, axis.y, axis.z, 0.0f };
					TOOL_ASSERT(V3Length(q * axis - v3 { turned.x, turned.y, turned.z }) <= 1e-5f);
				}

				// Read back as angles that make the same rotation, straight up and down included
				v3 euler = Q4GetEuler(q);
				TOOL_ASSERT(euler.x >= -F32_PI * 0.5f - 1e-5f && euler.x <= F32_PI * 0.5f + 1e-5f);
				TOOL_ASSERT(SameRotation(Q4Euler(euler), q, 1e-3f));
			}

	// Within range, the angles themselves come back
	v3 euler = Q4GetEuler(Q4Euler(0.3f, -2.5f, 1.1f));
	TOOL_ASSERT(F32Abs(euler.x - 0.3f) <= 1e-5f && F32Abs(euler.y + 2.5f) <= 1e-5f && F32Abs(euler.z - 1.1f) <= 1e-5f);

	// Past straight up, pitch folds back and yaw and roll turn half way round
	euler = Q4GetEuler(Q4Euler(2.0f, 0.0f, 0.0f));
	TOOL_ASSERT(F32Abs(euler.x - (F32_PI - 2.0f)) <= 1e-5f && F32Abs(F32Abs(euler.y) - F32_PI) <= 1e-5f && F32Abs(F32Abs(euler.z) - F32_PI) <= 1e-5f);
}



void TestQuaternion()
{
	TestEuler();
	printf("Quaternion: passed\n");
}
