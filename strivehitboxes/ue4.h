#pragma once

#include "struct_util.h"
#include <cmath>
#include <string>

class UFont;
class UTexture;
class FTexture;

using EngineMemAlign_t = void*(*)(size_t size, int alignment);
extern const EngineMemAlign_t EngineMemAlign;

using EngineFree_t = void*(*)(void *ptr);
extern const EngineFree_t EngineFree;

extern class UWorld **GWorld;

extern FTexture **GWhiteTexture;

enum EBlendMode {
	BLEND_Opaque         = 0,
	BLEND_Masked         = 1,
	BLEND_Translucent    = 2,
	BLEND_Additive       = 3,
	BLEND_Modulate       = 4,
	BLEND_AlphaComposite = 5,
	BLEND_AlphaHoldout   = 6,
	BLEND_MAX            = 7,
};

enum ESimpleElementBlendMode
{
	SE_BLEND_Opaque = 0,
	SE_BLEND_Masked,
	SE_BLEND_Translucent,
	SE_BLEND_Additive,
	SE_BLEND_Modulate,
	SE_BLEND_MaskedDistanceField,
	SE_BLEND_MaskedDistanceFieldShadowed,
	SE_BLEND_TranslucentDistanceField,
	SE_BLEND_TranslucentDistanceFieldShadowed,
	SE_BLEND_AlphaComposite,
	SE_BLEND_AlphaHoldout,
	// Like SE_BLEND_Translucent, but modifies destination alpha
	SE_BLEND_AlphaBlend,
	// Like SE_BLEND_Translucent, but reads from an alpha-only texture
	SE_BLEND_TranslucentAlphaOnly,
	SE_BLEND_TranslucentAlphaOnlyWriteAlpha,

	SE_BLEND_RGBA_MASK_START,
	SE_BLEND_RGBA_MASK_END = SE_BLEND_RGBA_MASK_START + 31, //Using 5bit bit-field for red, green, blue, alpha and desaturation

	SE_BLEND_MAX
};

struct FVector2D {
	float X, Y;

	FVector2D() : X(0), Y(0) {}
	FVector2D(float X, float Y) : X(X), Y(Y) {}
	FVector2D(int X, int Y) : X((float)X), Y((float)Y) {}

	FVector2D Rotate(const float angle) const
	{
		const auto ca = cosf(angle);
		const auto sa = sinf(angle);
		return FVector2D(X * ca + Y * -sa, X * sa + Y * ca);
	}

	float SizeSquared() const
	{
		return X * X + Y * Y;
	}

	float Size() const
	{
		return sqrt(SizeSquared());
	}

	FVector2D operator+(const FVector2D &other) const
	{
		return FVector2D(X + other.X, Y + other.Y);
	}

	FVector2D operator-(const FVector2D &other) const
	{
		return FVector2D(X - other.X, Y - other.Y);
	}

	FVector2D operator*(float scalar) const
	{
		return FVector2D(X * scalar, Y * scalar);
	}

	FVector2D operator/(float scalar) const
	{
		return FVector2D(X / scalar, Y / scalar);
	}
};

struct FVector {
	float X, Y, Z;

	FVector() : X(0), Y(0), Z(0) {}
	FVector(float X, float Y, float Z) : X(X), Y(Y), Z(Z) {}

	float SizeSquared() const
	{
		return X * X + Y * Y + Z * Z;
	}

	float Size() const
	{
		return sqrt(SizeSquared());
	}

	FVector operator+(const FVector &other) const
	{
		return FVector(X + other.X, Y + other.Y, Z + other.Z);
	}

	FVector operator-(const FVector &other) const
	{
		return FVector(X - other.X, Y - other.Y, Z - other.Z);
	}

	FVector operator*(float scalar) const
	{
		return FVector(X * scalar, Y * scalar, Z * scalar);
	}

	FVector operator/(float scalar) const
	{
		return FVector(X / scalar, Y / scalar, Z / scalar);
	}
};

struct FLinearColor {
	float R, G, B, A;

	FLinearColor() : R(0), G(0), B(0), A(0) {}
	FLinearColor(float R, float G, float B, float A) : R(R), G(G), B(B), A(A) {}
};

struct FString {
	FString(const std::wstring &s)
	{
		Count = Max = (int)(s.length() + 1);
		Data = s.data();
	}

	const wchar_t *Data;
	int Count, Max;
};

template<typename T>
struct TArray {
	template<size_t N>
	TArray(T (&elems)[N])
	{
		Data = (T*)EngineMemAlign(sizeof(T) * N, 8);

		for (auto i = 0; i < N; i++)
			Data[i] = elems[i];

		ArrayNum = ArrayMax = N;
	}

	template<size_t N>
	TArray(std::array<T, N> elems)
	{
		Data = (T*)EngineMemAlign(sizeof(T) * N, 8);

		for (auto i = 0; i < N; i++)
			Data[i] = elems[i];

		ArrayNum = ArrayMax = N;
	}

	~TArray()
	{
		EngineFree(Data);
	}

	TArray<T> &operator=(const TArray<T> &other)
	{
		if (this == &other)
			return *this;

		EngineFree(Data);

		Data = (T*)EngineMemAlign(sizeof(T) * other.ArrayNum, 8);
		for (auto i = 0; i < other.ArrayNum; i++)
			Data[i] = other.Data[i];

		ArrayNum = other.ArrayNum;
		ArrayMax = other.ArrayMax;

		return *this;
	}

	TArray<T> &operator=(TArray<T> &&other)
	{
		if (this == &other)
			return *this;

		EngineFree(Data);

		Data = other.Data;
		ArrayNum = other.ArrayNum;
		ArrayMax = other.ArrayMax;

		other.Data = nullptr;
		other.ArrayNum = other.ArrayMax = 0;

		return *this;
	}

	T *Data;
	int ArrayNum;
	int ArrayMax;
};

struct FCanvasUVTri {
	FVector2D V0_Pos;
	FVector2D V0_UV;
	FLinearColor V0_Color;
	FVector2D V1_Pos;
	FVector2D V1_UV;
	FLinearColor V1_Color;
	FVector2D V2_Pos;
	FVector2D V2_UV;
	FLinearColor V2_Color;
};

class FCanvasItem {
protected:
	FCanvasItem() = default;

public:

	// Virtual function wrapper
	void Draw(class FCanvas *InCanvas)
	{
		using Draw_t = void(*)(FCanvasItem*, class FCanvas*);
		((Draw_t)(*(void***)this)[3])(this, InCanvas);
	}

	FIELD(0x14, ESimpleElementBlendMode, BlendMode);
};

class FCanvasTriangleItem : public FCanvasItem {
public:
	FCanvasTriangleItem(
		const FVector2D &InPointA,
		const FVector2D &InPointB,
		const FVector2D &InPointC,
		const FTexture *InTexture);

	~FCanvasTriangleItem()
	{
		TriangleList.~TArray<FCanvasUVTri>();
	}

	FIELD(0x50, TArray<FCanvasUVTri>, TriangleList);

private:
	char pad[0x60];
};

static_assert(sizeof(FCanvasTriangleItem) == 0x60);

class FCanvas {
public:
	enum ECanvasDrawMode {
		CDM_DeferDrawing,
		CDM_ImmediateDrawing
	};

	FIELD(0xA0, ECanvasDrawMode, DrawMode);

	void Flush_GameThread(bool bForce = false);
	void DrawItem(FCanvasItem &Item);
};

class UObject {
public:
	FIELD(0x10, class UClass*, Class);

	// Check whether this is an instance of the given class or its subclasses
	bool IsA(UClass *ParentClass) const;

	template<class T>
	bool IsA() const
	{
		return IsA(T::StaticClass());
	}
};

struct FStructBaseChain {
	FStructBaseChain **StructBaseChainArray;
	int NumStructBasesInChainMinusOne;
};

class UField : public UObject {
};

class UStruct : public UField {
public:
	// Actually a superclass and not a field
	FIELD(0x30, FStructBaseChain, StructBaseChain);
};

class UClass : public UStruct {
};

class UWorld : public UObject {
public:
	FIELD(0x130, class AGameState*, GameState);
};

class AGameState : public UObject {
};

class UCanvas : public UObject {
public:
	void K2_DrawLine(FVector2D ScreenPositionA, FVector2D ScreenPositionB, float Thickness, const FLinearColor &RenderColor);
	FVector K2_Project(const FVector &WorldPosition) const;

	void K2_DrawText(
		UFont *RenderFont,
		const FString &RenderText,
		FVector2D ScreenPosition,
		const FLinearColor &RenderColor,
		float Kerning,
		const FLinearColor &ShadowColor,
		FVector2D ShadowOffset,
		bool bCentreX,
		bool bCentreY,
		bool bOutlined,
		const FLinearColor &OutlineColor);

	void K2_DrawTexture(
		UTexture *RenderTexture,
		FVector2D ScreenPosition,
		FVector2D ScreenSize,
		FVector2D CoordinatePosition,
		FVector2D CoordinateSize=FVector2D(1.f, 1.f),
		const FLinearColor &RenderColor=FLinearColor(1.f, 1.f, 1.f, 1.f),
		EBlendMode BlendMode=BLEND_Translucent,
		float Rotation=0.f,
		FVector2D PivotPoint=FVector2D(0.5f,0.5f));

	void K2_DrawTriangle(UTexture *RenderTexture, TArray<FCanvasUVTri> *Triangles);

	FIELD(0x260, FCanvas*, Canvas);
};

class AHUD : public UObject {
public:
	FIELD(0x278, UCanvas*, Canvas);
};
