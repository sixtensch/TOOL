#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



static b8 Near(f32 a, f32 b, f32 tolerance = 1e-5f)
{
	return F32Abs(a - b) <= tolerance;
}

// Clip space to normalized device coordinates.
static v3 Project(m4 projection, v3 point)
{
	v4 clip = projection * v4 { point.x, point.y, point.z, 1.0f };
	return v3 { clip.x / clip.w, clip.y / clip.w, clip.z / clip.w };
}

static void TestDeterminants()
{
	TOOL_ASSERT(M2Determinant(M2Scale(2.0f, 3.0f)) == 6.0f);
	TOOL_ASSERT(M3Determinant(M3Scale(2.0f, 3.0f, 4.0f)) == 24.0f && M3Trace(M3Scale(2.0f, 3.0f, 4.0f)) == 9.0f);
	TOOL_ASSERT(M4Determinant(M4Identity()) == 1.0f);
	TOOL_ASSERT(M4Determinant(M4Scale(2.0f, 3.0f, 4.0f, 5.0f)) == 120.0f);
	TOOL_ASSERT(Near(M4Determinant(M4Translation(3.0f, -2.0f, 7.0f) * M4Rotation(0.3f, 1.1f, -0.7f)), 1.0f));

	// Rows [2 -1 0 3] [1 4 -2 0] [0.5 0 3 1] [-1 2 1 2], stored by column.
	m4 general =
	{
		2.0f, 1.0f, 0.5f, -1.0f,
		-1.0f, 4.0f, 0.0f, 2.0f,
		0.0f, -2.0f, 3.0f, 1.0f,
		3.0f, 0.0f, 1.0f, 2.0f
	};
	TOOL_ASSERT(Near(M4Determinant(general), 107.0f, 1e-4f));
	TOOL_ASSERT(Near(M4Determinant(M4Transpose(general)), 107.0f, 1e-4f));
}

static void TestProjections()
{
	f32 nearClip = 0.5f;
	f32 farClip = 100.0f;
	f32 edge = F32Tan(F32Radians(60.0f) * 0.5f);

	// Perspective: near and far land on the depth range ends, and the frustum's top edge on the top of clip space.
	m4 dx = M4ProjectionPerspective(60.0f, 2.0f, nearClip, farClip, ClipTypeDX);
	m4 vulkan = M4ProjectionPerspective(60.0f, 2.0f, nearClip, farClip, ClipTypeVulkan);
	m4 gl = M4ProjectionPerspective(60.0f, 2.0f, nearClip, farClip, ClipTypeOpenGL);

	TOOL_ASSERT(Near(Project(dx, v3 { 0.0f, 0.0f, nearClip }).z, 0.0f));
	TOOL_ASSERT(Near(Project(dx, v3 { 0.0f, 0.0f, farClip }).z, 1.0f));
	TOOL_ASSERT(Near(Project(gl, v3 { 0.0f, 0.0f, nearClip }).z, -1.0f));
	TOOL_ASSERT(Near(Project(gl, v3 { 0.0f, 0.0f, farClip }).z, 1.0f));

	v3 top = { 0.0f, edge * 10.0f, 10.0f };
	v3 right = { edge * 20.0f, 0.0f, 10.0f };
	TOOL_ASSERT(Near(Project(dx, top).y, 1.0f) && Near(Project(gl, top).y, 1.0f) && Near(Project(vulkan, top).y, -1.0f));
	TOOL_ASSERT(Near(Project(dx, right).x, 1.0f) && Near(Project(vulkan, right).x, 1.0f));

	// Orthographic: the box's corners land on the corners of clip space.
	m4 orthoDX = M4ProjectionOrthographic(16.0f, 9.0f, nearClip, farClip, ClipTypeDX);
	m4 orthoVulkan = M4ProjectionOrthographic(16.0f, 9.0f, nearClip, farClip, ClipTypeVulkan);
	m4 orthoGL = M4ProjectionOrthographic(16.0f, 9.0f, nearClip, farClip, ClipTypeOpenGL);

	v3 corner = Project(orthoDX, v3 { 8.0f, 4.5f, nearClip });
	TOOL_ASSERT(Near(corner.x, 1.0f) && Near(corner.y, 1.0f) && Near(corner.z, 0.0f));
	corner = Project(orthoDX, v3 { -8.0f, -4.5f, farClip });
	TOOL_ASSERT(Near(corner.x, -1.0f) && Near(corner.y, -1.0f) && Near(corner.z, 1.0f));
	TOOL_ASSERT(Near(Project(orthoVulkan, v3 { 8.0f, 4.5f, nearClip }).y, -1.0f));
	TOOL_ASSERT(Near(Project(orthoGL, v3 { 0.0f, 0.0f, nearClip }).z, -1.0f));
	TOOL_ASSERT(Near(Project(orthoGL, v3 { 0.0f, 0.0f, farClip }).z, 1.0f));
}



void TestMatrix()
{
	TestDeterminants();
	TestProjections();
	printf("Matrix: passed\n");
}
