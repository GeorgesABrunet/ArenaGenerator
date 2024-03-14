/*
 * MIT License
 *
 * Copyright (c) 2024 Georges Brunet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "CoreMinimal.h"
#include "ArenaGeneratorTypes.generated.h"

#pragma region Enums

//Arena Build Order Rules determine what section of the arena should be generated first
//and how other sections should line up to it. A section leading by dimensions will use
//grid parameters from configurations. Leading by radius will override some configurations
//and align the arena to those dimensions.
UENUM(BlueprintType)
enum class EArenaBuildOrderRules : uint8
{
	FloorLeadsByDimensions,
	FloorLeadsByRadius,
	WallsLeadByDimensions,
	WallsLeadByRadius, 

};

UENUM(BlueprintType)
enum class EMeshOriginPlacement : uint8
{
	XY_Positive,
	XY_Negative,
	X_Positive_Y_Negative,
	X_Negative_Y_Positive,
	Center,
	Custom,
	//...
};

#pragma endregion

#pragma region Structs

// Floor Rule Configuration determine behaviors to consider during transform calculation
USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FFloorTransformRules : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bFloorRotates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMoveFloorWhenRotated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWarpFloorPlacement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWarpFloorScale = false;
	//...
};

// Wall Rule Configuration determine behaviors to consider during transform calculation
USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FWallTransformRules : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWarpWallPlacement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAddWallRotation = false;
	//...
};

// Roof Rule Configuration determine behaviors to consider during transform calculation
USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FRoofTransformRules : public FTableRowBase
{
	GENERATED_BODY()

	//Bring roof forward by width of walls. This may be preferable for some meshes that are angled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBringRoofForward = false;
	
	//If the roof should move forward by its horizontal projection each level
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRoofIncrementsForwardEachLevel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRoofShouldRotate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMoveRoofWhenRotated = false;
	
};

USTRUCT(BlueprintType)
struct ARENAGENERATOR_API FArenaMeshConfig : public FTableRowBase
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMeshOriginPlacement OriginType = EMeshOriginPlacement::XY_Positive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MeshDimensions = FVector{ 500, 500, 500 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MeshScale = FVector{ 1 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UStaticMesh> ArenaMesh;

};

#pragma endregion

