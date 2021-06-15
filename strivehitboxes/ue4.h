#pragma once

#include "struct_util.h"
#include <cmath>
#include <string>

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

class UFont;
class UTexture;

struct FVector2D
{
	float X, Y;

	FVector2D() : X(0), Y(0) {}
	FVector2D(float X, float Y) : X(X), Y(Y) {}
	FVector2D(int X, int Y) : X((float)X), Y((float)Y) {}

	FVector2D Rotate(const float angle)
	{
		const auto ca = cosf(angle);
		const auto sa = sinf(angle);
		return FVector2D(X * ca + Y * -sa, X * -sa + Y * -ca);
	}

	FVector2D operator+(const FVector2D &other) const
	{
		return FVector2D(X + other.X, Y + other.Y);
	}

	FVector2D operator-(const FVector2D &other) const
	{
		return FVector2D(X - other.X, Y - other.Y);
	}
};

struct FVector
{
	float X, Y, Z;

	FVector() : X(0), Y(0), Z(0) {}
	FVector(float X, float Y, float Z) : X(X), Y(Y), Z(Z) {}

	FVector operator+(const FVector &other) const
	{
		return FVector(X + other.X, Y + other.Y, Z + other.Z);
	}

	FVector operator-(const FVector &other) const
	{
		return FVector(X - other.X, Y - other.Y, Z - other.Z);
	}
};

struct FLinearColor
{
	float R, G, B, A;

	FLinearColor() : R(0), G(0), B(0), A(0) {}
	FLinearColor(float R, float G, float B, float A) : R(R), G(G), B(B), A(A) {}
};

struct FString
{
	FString(const std::wstring &s)
	{
		Count = Max = (int)(s.length() + 1);
		Data = s.data();
	}

	const wchar_t *Data;
	int Count, Max;
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

class UCanvas : public UObject
{
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
};

class AHUD : public UObject
{
public:
	FIELD(0x278, UCanvas*, Canvas);
};

extern UWorld **GWorld;
