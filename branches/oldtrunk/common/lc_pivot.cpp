#include "lc_global.h"
#include "lc_pivot.h"

#include "lc_mesh.h"
#include "lc_scene.h"
#include "lc_colors.h"

#define LC_PIVOT_SIZE 0.3f

lcPivot::lcPivot()
	: lcPieceObject(LC_OBJECT_PIVOT)
{
	// fixme: lcPivot
}

lcPivot::~lcPivot()
{
	// fixme: lcPivot
}

void lcPivot::GetPieceList(lcObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const
{
	// fixme: lcPivot
}

void lcPivot::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(LC_PIVOT_SIZE, LC_PIVOT_SIZE, LC_PIVOT_SIZE);
	Box.m_Min = Vector3(-LC_PIVOT_SIZE, -LC_PIVOT_SIZE, -LC_PIVOT_SIZE);

	Vector3 Start = Mul31(Ray->Start, m_ModelWorld);
	Vector3 End = Mul31(Ray->End, m_ModelWorld);

	float Dist;
	if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < Ray->Dist))
	{
		Ray->Object = this;
		Ray->Dist = Dist;
	}

	for (lcObject* Child = m_Children; Child; Child = Child->m_Next)
		Child->ClosestRayIntersect(Ray);
}

bool lcPivot::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// fixme: lcPivot
	return false;
}

void lcPivot::Update(u32 Time)
{
	lcPieceObject::Update(Time);

	for (lcObject* Child = m_Children; Child; Child = Child->m_Next)
		Child->Update(Time);

	// fixme: lcPivot
}

void lcPivot::AddToScene(lcScene* Scene, int Color)
{
	Matrix44 ScaleMatrix(Vector4(LC_PIVOT_SIZE, 0.0f, 0.0f, 0.0f), Vector4(0.0f, LC_PIVOT_SIZE, 0.0f, 0.0f), Vector4(0.0f, 0.0f, LC_PIVOT_SIZE, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));

	// fixme: lcPivot
	lcRenderSection RenderSection;

	RenderSection.Owner = this;
	RenderSection.ModelWorld = Mul(ScaleMatrix, m_ModelWorld);
	RenderSection.Mesh = lcWireframeBoxMesh;
	RenderSection.Section = &lcWireframeBoxMesh->m_Sections[0];
	RenderSection.Color = 1;

	if (IsFocused())
		RenderSection.Color = LC_COLOR_FOCUS;
	else if (IsSelected())
		RenderSection.Color = LC_COLOR_SELECTION;

	Scene->m_OpaqueSections.Add(RenderSection);

	for (lcObject* Child = m_Children; Child; Child = Child->m_Next)
		Child->AddToScene(Scene, Color);
}

void lcPivot::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	if (IsSelected())
		ChangeKey(Time, AddKey, 0, m_ParentPosition + Delta);
}