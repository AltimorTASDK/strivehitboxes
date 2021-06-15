#include "ue4.h"
#include "sigscan.h"

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

UWorld **GWorld = (UWorld**)get_rip_relative(
	sigscan::get().scan("\x0F\x85\xFC\x00\x00\x00\x48\x8B\x05", "xxxxxxxxx") + 9);

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
