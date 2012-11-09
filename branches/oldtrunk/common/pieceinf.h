#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#ifndef GLuint
#include "opengl.h"
#endif
#include "algebra.h"

#define LC_PIECE_COUNT            0x01 // Count this piece in the totals ?
#define LC_PIECE_LONGDATA         0x02 // unsigned long/short index
#define LC_PIECE_CCW              0x04 // Use back-face culling
#define LC_PIECE_SMALL            0x10 // scale = 10000
#define LC_PIECE_MEDIUM           0x20 // scale = 1000 (otherwise = 100)
#define LC_PIECE_LONGDATA_RUNTIME 0x40 // If the original data is 16 bits but we expanded to 32 bits

class File;
class Texture;
class lcMesh;

struct CONNECTIONINFO
{
	unsigned char type;
	float center[3];
	float normal[3];
};

struct DRAWGROUP
{
	unsigned short connections[6];
	int NumSections;
};

struct TEXTURE
{
	Texture* texture;
	unsigned char color;
	float vertex[4][3];
	float coords[4][2];
};

unsigned char ConvertColor(int c);

class PieceInfo
{
public:
	PieceInfo();
	~PieceInfo();

	bool IsPatterned() const
	{
		const char* Name = m_strName;

		while (*Name)
		{
			if (*Name < '0' || *Name > '9')
				break;

			Name++;
		}

		if (*Name == 'P')
			return true;

		return false;
	}

	bool IsSubPiece() const
	{
		return (m_strDescription[0] == '~');
	}

	Vector3 GetCenter() const
	{
		return Vector3((m_fDimensions[0] + m_fDimensions[3]) * 0.5f,
		               (m_fDimensions[1] + m_fDimensions[4]) * 0.5f,
		               (m_fDimensions[2] + m_fDimensions[5]) * 0.5f);
	}

	// Operations
	void ZoomExtents(float Fov, float Aspect, float* EyePos = NULL) const;
	void RenderOnce(int nColor);
	void RenderPiece(int nColor);
	void WriteWavefront(FILE* file, unsigned char color, unsigned long* start);
	void WritePOV(FILE* file);

	// Implementation
	lcMesh* GetMesh() const
	{ return m_Mesh; }
	GLuint GetBoxDisplayList()
	{
		if (!m_nBoxList)
			CreateBoxDisplayList();
		return m_nBoxList;
	};
	void LoadIndex(File& file);
	void AddRef();
	void DeRef();

public:
	// Attributes
	char m_strName[9];
	char m_strDescription[65];
	float m_fDimensions[6];
	unsigned long m_nOffset;
	unsigned long m_nSize;

	// Nobody should change these
	unsigned char m_nFlags;
	unsigned short m_nConnectionCount;
	CONNECTIONINFO* m_pConnections;
	unsigned short m_nGroupCount;
	DRAWGROUP* m_pGroups;
	unsigned char m_nTextureCount;
	TEXTURE* m_pTextures;

	// Bounding box / sphere.
	Vector3 m_Center;
	Vector4 m_Dimensions; // xyz = box extents, w = sphere radius

protected:
	int m_nRef;
	GLuint m_nBoxList;
	lcMesh* m_Mesh;

	// Workaround for MSVC6 poor template support.
	template <class T>
	struct TypeToType
	{
		typedef T type;
	};

	template<class T>
	void BuildMesh(TypeToType<T>, void* Data, void* MeshStart, bool LongData);

	void LoadInformation();
	void FreeInformation();
	void CreateBoxDisplayList();
};

#endif // _PIECEINF_H_