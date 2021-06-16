#include "ue4.h"
#include "sigscan.h"

const EngineMemAlign_t EngineMemAlign = (EngineMemAlign_t)get_rip_relative(
	sigscan::get().scan("\x33\xD2\x8D\x4A\x14\xE8", "xxxxxx") + 6);

const EngineFree_t EngineFree = (EngineFree_t)get_rip_relative(
	sigscan::get().scan("\x48\x8B\x4F\x20\x48\x85\xC9\x74\x0D\xE8", "xxxxxxxxxx") + 10);

using FCanvasTriangleItem_ctor_t = void(*)(
	FCanvasTriangleItem*,
	const FVector2D&,
	const FVector2D&,
	const FVector2D&,
	const FTexture*);
const auto FCanvasTriangleItem_ctor = (FCanvasTriangleItem_ctor_t)(
	sigscan::get().scan("\x48\x89\x4B\x10\x88\x4B\x18", "xxxxxxx") - 0x34);

using FCanvas_Flush_GameThread_t = void(*)(FCanvas *, bool);
const auto FCanvas_Flush_GameThread = (FCanvas_Flush_GameThread_t)(
	sigscan::get().scan("\x75\x08\x84\xD2\x0F\x84\x34\04", "xxxxxxxx") - 0x24);

using UCanvas_K2_DrawLine_t = void(*)(UCanvas*, FVector2D, FVector2D, float, const FLinearColor&);
const auto UCanvas_K2_DrawLine = (UCanvas_K2_DrawLine_t)(
	sigscan::get().scan("\x0F\x2F\xC8\x0F\x86\x94", "xxxxxx") - 0x51);

using UCanvas_K2_Project_t = void(*)(const UCanvas*, FVector*, const FVector&);
const auto UCanvas_K2_Project = (UCanvas_K2_Project_t)(
	sigscan::get().scan("\x48\x8B\x89\x68\x02\x00\x00\x48\x8B\xDA", "xxxxxxxxxx") - 0x12);

using UCanvas_K2_DrawText_t = void(*)(
	UCanvas*,
	UFont*,
	const FString&,
	FVector2D,
	const FLinearColor&,
	float,
	const FLinearColor&,
	FVector2D,
	bool,
	bool,
	bool,
	const FLinearColor&);
UCanvas_K2_DrawText_t UCanvas_K2_DrawText;

using UCanvas_K2_DrawTexture_t = void(*)(
	UCanvas*,
	UTexture*,
	FVector2D,
	FVector2D,
	FVector2D,
	FVector2D,
	const FLinearColor&,
	EBlendMode,
	float,
	FVector2D);
const auto UCanvas_K2_DrawTexture = (UCanvas_K2_DrawTexture_t)(
	sigscan::get().scan("\x0F\x2F\xF5\xF2\x0F\x11", "xxxxxx") - 0x1F);

using UCanvas_K2_DrawTriangle_t = void(*)(UCanvas*, UTexture*, TArray<FCanvasUVTri>*);
const auto UCanvas_K2_DrawTriangle = (UCanvas_K2_DrawTriangle_t)(
	sigscan::get().scan("\x48\x81\xEC\x90\x00\x00\x00\x41\x83\x78\x08\x00", "xxxxxxxxxxxx") - 6);

UWorld **GWorld = (UWorld**)get_rip_relative(
	sigscan::get().scan("\x0F\x85\xFC\x00\x00\x00\x48\x8B\x05", "xxxxxxxxx") + 9);

FTexture **GWhiteTexture = (FTexture**)get_rip_relative((uintptr_t)UCanvas_K2_DrawTriangle + 0x3A);

FCanvasTriangleItem::FCanvasTriangleItem(
	const FVector2D &InPointA,
	const FVector2D &InPointB,
	const FVector2D &InPointC,
	const FTexture *InTexture)
{
	FCanvasTriangleItem_ctor(this, InPointA, InPointB, InPointC, InTexture);
}

void FCanvas::Flush_GameThread(bool bForce)
{
	FCanvas_Flush_GameThread(this, bForce);
}

void FCanvas::DrawItem(FCanvasItem &Item)
{
	Item.Draw(this);

	if (DrawMode == CDM_ImmediateDrawing)
		Flush_GameThread();
}

bool UObject::IsA(UClass *ParentClass) const
{
	const auto &Chain = Class->StructBaseChain;
	const auto &ParentChain = ParentClass->StructBaseChain;
	return
		ParentChain.NumStructBasesInChainMinusOne <= Chain.NumStructBasesInChainMinusOne &&
		Chain.StructBaseChainArray[ParentChain.NumStructBasesInChainMinusOne] == &ParentChain;
}

void UCanvas::K2_DrawLine(FVector2D ScreenPositionA, FVector2D ScreenPositionB, float Thickness, const FLinearColor &RenderColor)
{
	UCanvas_K2_DrawLine(this, ScreenPositionA, ScreenPositionB, Thickness, RenderColor);
}

FVector UCanvas::K2_Project(const FVector &WorldPosition) const
{
	FVector out;
	UCanvas_K2_Project(this, &out, WorldPosition);
	return out;
}

void UCanvas::K2_DrawText(
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
	const FLinearColor &OutlineColor)
{
	UCanvas_K2_DrawText(
		this,
		RenderFont,
		RenderText,
		ScreenPosition,
		RenderColor,
		Kerning,
		ShadowColor,
		ShadowOffset,
		bCentreX,
		bCentreY,
		bOutlined,
		OutlineColor);
}

void UCanvas::K2_DrawTexture(
	UTexture *RenderTexture,
	FVector2D ScreenPosition,
	FVector2D ScreenSize,
	FVector2D CoordinatePosition,
	FVector2D CoordinateSize,
	const FLinearColor &RenderColor,
	EBlendMode BlendMode,
	float Rotation,
	FVector2D PivotPoint)
{
	UCanvas_K2_DrawTexture(
		this,
		RenderTexture,
		ScreenPosition,
		ScreenSize,
		CoordinatePosition,
		CoordinateSize,
		RenderColor,
		BlendMode,
		Rotation,
		PivotPoint);
}

void UCanvas::K2_DrawTriangle(UTexture *RenderTexture, TArray<FCanvasUVTri> *Triangles)
{
	UCanvas_K2_DrawTriangle(this, RenderTexture, Triangles);
}
