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


#include "BaseArenaGenerator.h"
#include "Math/RandomStream.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/StaticMeshActor.h"
#include "ArenaGeneratorLog.h"

// Sets default values
ABaseArenaGenerator::ABaseArenaGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; //Does not need to tick

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	
	ArenaSeed = 1010101;
	ArenaStream = FRandomStream(ArenaSeed);
	TotalInstances = 0;

	FocusGridIndex = 0;
	FocusPolygonIndex = 0;
	
}

// Called when the game starts or when spawned
void ABaseArenaGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	
}

void ABaseArenaGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	WipeArena(); //Need to handle components
}

void ABaseArenaGenerator::GenerateArena()
{
	//Clear previous arena
	WipeArena();


	ArenaGenLog_Info("============ Generating Arena ============");

	//Build sections from section list
	BuildSections();

	//Log number of mesh Instances in arena
	ArenaGenLog_Info("============ Finished, # of Instances: %d ============", TotalInstances);

}

void ABaseArenaGenerator::WipeArena()
{
	ArenaGenLog_Info("Wiping Arena...");
	
	//Clear list of used indices
	if (!UsedGroupIndices.IsEmpty()) {
		UsedGroupIndices.Empty();
	}
	
	//Iterate through mesh instances and destroy before dereferencing
	if (!MeshInstances.IsEmpty())
	{	
		for (auto& Inst : MeshInstances)
		{
			for (auto& Component : Inst) {
				if (Component) {
					Component->DestroyComponent();
				}
			}		
		}

		MeshInstances.Empty();
	}

	//Iterate through spawned actors and destroy spawned actors
	if (!SpawnedActors.IsEmpty()) 
	{
		for (AActor* Actor : SpawnedActors)
		{
			if (IsValid(Actor)) {
				Actor->Destroy();
			}
		}

		SpawnedActors.Empty();
	}
	
	TotalInstances = 0;
	
	//Reset parameters for calculations
	CurrentBOR = EArenaBuildOrderRules::PolygonLeadByRadius;
	OriginOffset = FVector(0);
	PreviousMeshSize = FVector(0);
	PreviousTilesPerSide = 0;
	PreviousLastPosition = FVector(0);
	FocusGridIndex = 0;
	FocusPolygonIndex = 0;
	

}

void ABaseArenaGenerator::CalculateSectionParameters(FArenaSection& Section)
{
	if (MeshGroups.IsEmpty()) { 
		ArenaGenLog_Error("Cannot calculate section parameters with empty Mesh Groups!");
		return;
	}

	//For all cases, we need these.
	OriginOffset = FVector(0);
	ArenaSides = FMath::Clamp(Section.Targets.TargetPolygonSides, 3, MaxSides);
	InteriorAngle = ((ArenaSides - 2) * 180) / ArenaSides;
	ExteriorAngle = 360.f / ArenaSides;

	//Determine best starting indices for patterns
	bool bGrided = false;
	bool bPolygoned = false;

	for (int32 i = 0; i < Section.BuildRules.Num(); i++) {
		if (bGrided && bPolygoned) { break; }
		if (Section.BuildRules[i].SectionType == EArenaSectionType::HorizontalGrid && !bGrided) { FocusGridIndex = Section.BuildRules[i].ObjectGroupId; bGrided = true; }
		if (Section.BuildRules[i].SectionType == EArenaSectionType::Polygon && !bPolygoned) { FocusPolygonIndex = Section.BuildRules[i].ObjectGroupId; bPolygoned = true; }
	}
	CurrentBOR = Section.SectionBuildOrderRules;
	FVector GridTileSize;
	FVector PolygonTileSize;

	//CALCULATE SECTION PARAMETERS
	switch (Section.SectionBuildOrderRules) {
	case EArenaBuildOrderRules::GridLeadsByDimensions:
	{
		//ArenaDimensions determines Inscribed Radius
		//Floors
		ArenaDimensions = Section.Targets.TargetGridDimensions;
		InscribedRadius = (MeshGroups[FocusGridIndex].MeshDimensions.X * (Section.Targets.TargetGridDimensions - 1) * 0.5);

		//Walls
		SideLength = 2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f);
		TilesPerArenaSide = FMath::Clamp(FMath::Floor(SideLength / MeshGroups[FocusPolygonIndex].MeshDimensions.X),
			1, //Min
			MaxTilesPerSideRow //Max
		);
		Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

	}break;
	case EArenaBuildOrderRules::GridLeadsByRadius:
	{
		//InscribedRadius determines arenadims w mesh size
		
		//Floors
		ArenaDimensions = (Section.Targets.TargetInscribedRadius / MeshGroups[FocusGridIndex].MeshDimensions.X) > 2 ? FMath::Floor(Section.Targets.TargetInscribedRadius / MeshGroups[FocusGridIndex].MeshDimensions.X) : 2;
		// was Section.BuildRules[FocusGridIndex].MeshGroupId
		InscribedRadius = (MeshGroups[FocusGridIndex].MeshDimensions.X * (ArenaDimensions) * 0.5);
		//was Section.BuildRules[FocusGridIndex].MeshGroupId
		
		//Walls
		SideLength = 2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f);	
		TilesPerArenaSide = FMath::Clamp(FMath::Floor(SideLength / MeshGroups[FocusPolygonIndex].MeshDimensions.X),
			1, //Min
			MaxTilesPerSideRow //Max
		);
	
		Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

	}break;
	case EArenaBuildOrderRules::PolygonLeadByDimensions:
	{
		//find inscribed radius from mesh size, desired tps, arena sides.

		TilesPerArenaSide = Section.Targets.TargetTilesPerSide;
		SideLength = MeshGroups[FocusPolygonIndex].MeshDimensions.X * TilesPerArenaSide;

		InscribedRadius = (SideLength / 2.f) / FMath::Sin(FMath::DegreesToRadians(90.f - InteriorAngle/2)); //Hypotenuse = opposite divided by sine of adjacent angle 
		Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

		ArenaDimensions = FMath::CeilToInt((InscribedRadius * 2.f) / MeshGroups[FocusGridIndex].MeshDimensions.X); //was Section.BuildRules[FocusGridIndex].MeshGroupId
	}break;
	case EArenaBuildOrderRules::PolygonLeadByRadius:
	{
		//Inscribedradius determines final amount of tiles per side 

		TilesPerArenaSide = FMath::Floor((2.f * CalculateOpposite(Section.Targets.TargetInscribedRadius, InteriorAngle / 2.f)) / MeshGroups[FocusPolygonIndex].MeshDimensions.X); //was Section.BuildRules[FocusPolygonIndex].MeshGroupId
		SideLength = MeshGroups[FocusPolygonIndex].MeshDimensions.X * TilesPerArenaSide;

		InscribedRadius = (SideLength / 2.f) / FMath::Sin(FMath::DegreesToRadians(90.f - InteriorAngle/2)); //Hypotenuse = opposite/2 divided by sine of adjacent angle
		Apothem = abs(CalculateAdjacent(InscribedRadius, InteriorAngle / 2));

		ArenaDimensions = FMath::CeilToInt((InscribedRadius * 2.f) / MeshGroups[FocusGridIndex].MeshDimensions.X);
	}break;
	}

	//TODO - Final Checks. Determine if Arena dimensions are sufficient for the amount of arena sides. Use rule to determine if we should reduce arena sides, or increase arena dimensions if so.
	// Polygon is incribed within Grid if 1 >= (meshsize.x / (sin(pi/polygonsides) * grid diagonal))
	//Likely need recursive function to determine how many sides there can be at most.
}

void ABaseArenaGenerator::BuildSections()
{
	if (MeshGroups.IsEmpty() && ActorGroups.IsEmpty()) {
		ArenaGenLog_Error("Cannot build sections with empty Mesh & Actor Groups!");
		return;
	}


	if (SectionList.IsEmpty())
	{
		ArenaGenLog_Warning("SectionList is empty! Arena generation is null.");
		return;
	}
	else 
	{
		ArenaGenLog_Info("Building out %d Sections", SectionList.Num());

		//for every Section...
		for (int32 i = 0; i < SectionList.Num(); i++)
		{
			//Calculate section parameters
			ArenaGenLog_Info("Building out %d patterns", SectionList[i].BuildRules.Num());
			CalculateSectionParameters(SectionList[i]);
			
			for (int32 j = 0; j < SectionList[i].BuildRules.Num(); j++)
			{
				ArenaGenLog_Info("Building SECTION %d : PATTERN %d ", i, j);
				BuildSection(SectionList[i].BuildRules[j]);
			}

		}
	}
}

void ABaseArenaGenerator::BuildSection(FArenaSectionBuildRules& Section)
{
	if(Section.AssetToPlace == ETypeToPlace::StaticMeshes && MeshGroups.IsEmpty())
	{
		ArenaGenLog_Error("Cannot build section pattern because associated mesh group is invalid OR mesh groups are empty.");
		return; 
	}
	else if (Section.AssetToPlace == ETypeToPlace::Actors && (ActorGroups.IsEmpty() || ActorGroups[0].ClassesToSpawn.IsEmpty()))
	{
		ArenaGenLog_Error("Cannot build section pattern because associated actor group is invalid OR actor groups are empty.");
		return;
	}

	if (Section.SectionAmount < 1) { Section.SectionAmount = 1; } //Make sure section amount is not negative or zero

	
	int GroupIdx = 0;
	int ReRouteIdx = 0;
	FVector MeshSize = FVector{ 100 };
	FVector MeshScale = FVector{ 1 };

	switch (Section.AssetToPlace)
	{
		default:
			ArenaGenLog_Warning("Hit default for Asset To Place. Using Static Mesh as default.");
		case ETypeToPlace::StaticMeshes:
		{
			//Clamp MeshGroup index so that we do not search outside of its bounds
			GroupIdx = (Section.ObjectGroupId < MeshGroups.Num() && Section.ObjectGroupId > 0) ? Section.ObjectGroupId : 0;

			//Determine arena parameters based off of current origin offsets etc.
			MeshSize = MeshGroups[GroupIdx].MeshDimensions;
			MeshScale = MeshGroups[GroupIdx].MeshScale;
		}break;
		case ETypeToPlace::Actors:
		{
			GroupIdx = (Section.ObjectGroupId < ActorGroups.Num() && Section.ObjectGroupId > 0) ? Section.ObjectGroupId : 0;

			MeshSize = ActorGroups[GroupIdx].ActorDimensions;
			MeshScale = ActorGroups[GroupIdx].ActorScale;
		}break;
	}
	
	
	
	if (PreviousMeshSize == FVector(0)) { PreviousMeshSize = MeshSize; } 
	float MeshScalar = PreviousMeshSize.X != 0.f ? MeshSize.X / PreviousMeshSize.X: 1.f;
	float HeightAdjustment = MeshSize.Z * Section.InitOffsetByHeightScalar;
	bool bConcavity = Section.bWarpPlacement && Section.WarpConcavityStrength != 0.f;

	//TODO - adjust curr tiles per side to init and per-iteration width offsets
	int CurrTilesPerSide = MeshScalar == 1.f ? TilesPerArenaSide : //Tiles per Side of the pattern
		FMath::Clamp(FMath::Floor((2.f * CalculateOpposite(InscribedRadius, InteriorAngle / 2.f)) / MeshSize.X), 1, MaxTilesPerSideRow);

	if (PreviousTilesPerSide == 0) { PreviousTilesPerSide = TilesPerArenaSide; } // Prev Tiles cannot be zero

	//Update Origin Offset based on Arena placement on actor option and previous parameters
	
	switch (ArenaPlacementOnActor) {
		default:
		ArenaGenLog_Info("Hit default case on ArenaPlacement! Placing in center.");
		case EOriginPlacementType::Center:
		{
			if (Section.SectionType == EArenaSectionType::HorizontalGrid) {
				OriginOffset = FVector(
					(MeshSize.X * (-0.5f * ArenaDimensions) * MeshScale.X),//X
					(MeshSize.Y * (-0.5f * ArenaDimensions) * MeshScale.Y),//Y
					OriginOffset.Z);
			}
			else if (Section.SectionType == EArenaSectionType::Polygon) {
				FVector PolygonOffset = (
					(ForwardVectorFromYaw(InteriorAngle / 2) * InscribedRadius) * FVector(static_cast<float>(CurrTilesPerSide) / (SideLength / MeshSize.X))
					);

				if (CurrentBOR == EArenaBuildOrderRules::GridLeadsByDimensions || CurrentBOR == EArenaBuildOrderRules::GridLeadsByRadius)
				{
					OriginOffset = FVector(-PolygonOffset.X, -PolygonOffset.Y, OriginOffset.Z); //for Grid BOR
				}
				else if (CurrentBOR == EArenaBuildOrderRules::PolygonLeadByDimensions || CurrentBOR == EArenaBuildOrderRules::PolygonLeadByRadius)
				{
					OriginOffset = FVector(-(SideLength / 2), -Apothem, OriginOffset.Z);
				}
			}
		}break;
		case EOriginPlacementType::XY_Positive:
		{
			if (Section.SectionType == EArenaSectionType::HorizontalGrid) {
				OriginOffset = FVector(0, 0, OriginOffset.Z);
					
			}
			else if (Section.SectionType == EArenaSectionType::Polygon) {

				if (CurrentBOR == EArenaBuildOrderRules::GridLeadsByDimensions || CurrentBOR == EArenaBuildOrderRules::GridLeadsByRadius)
				{
					OriginOffset = FVector(((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2))/2
						, ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2))/2
						, OriginOffset.Z);
				}
				else if (CurrentBOR == EArenaBuildOrderRules::PolygonLeadByDimensions || CurrentBOR == EArenaBuildOrderRules::PolygonLeadByRadius)
				{
					OriginOffset = FVector((MeshSize.X * (0.5f * ArenaDimensions) * MeshScale.X) - (SideLength / 2)
						, ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2
						, OriginOffset.Z);
				}
			}
		}
		break;
		case EOriginPlacementType::X_Positive_Y_Negative:
		{
			if (Section.SectionType == EArenaSectionType::HorizontalGrid) {
				OriginOffset = FVector(
					0,//X
					(MeshSize.Y * -ArenaDimensions * MeshScale.Y),//Y
					OriginOffset.Z);
			}
			else if (Section.SectionType == EArenaSectionType::Polygon) {

				if (CurrentBOR == EArenaBuildOrderRules::GridLeadsByDimensions || CurrentBOR == EArenaBuildOrderRules::GridLeadsByRadius)
				{
					OriginOffset = FVector(((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2
						, (-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2)
						, OriginOffset.Z);
				}
				else if (CurrentBOR == EArenaBuildOrderRules::PolygonLeadByDimensions || CurrentBOR == EArenaBuildOrderRules::PolygonLeadByRadius)
				{
					OriginOffset = FVector((MeshSize.X * (0.5f * ArenaDimensions) * MeshScale.X) - (SideLength / 2)
						, (-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2)
						, OriginOffset.Z);
				}
			}
		}break;
		case EOriginPlacementType::XY_Negative:
		{
			if (Section.SectionType == EArenaSectionType::HorizontalGrid) {
				OriginOffset = FVector(
					(MeshSize.X * -ArenaDimensions * MeshScale.X),//X
					(MeshSize.Y * -ArenaDimensions * MeshScale.Y),//Y
					OriginOffset.Z);
			}
			else if (Section.SectionType == EArenaSectionType::Polygon) {
			
				if (CurrentBOR == EArenaBuildOrderRules::GridLeadsByDimensions || CurrentBOR == EArenaBuildOrderRules::GridLeadsByRadius)
				{
					OriginOffset = FVector((-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2)
						, (-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2)
						, OriginOffset.Z); //for Grid BOR
				}
				else if (CurrentBOR == EArenaBuildOrderRules::PolygonLeadByDimensions || CurrentBOR == EArenaBuildOrderRules::PolygonLeadByRadius)
				{
					
					OriginOffset = FVector(-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * (0.5f * ArenaDimensions) * MeshScale.X) - (SideLength / 2))
						, (-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2)
						, OriginOffset.Z);
				}
			}
		}break;
		case EOriginPlacementType::X_Negative_Y_Positive:
		{
			if (Section.SectionType == EArenaSectionType::HorizontalGrid) {
				OriginOffset = FVector(
					(MeshSize.X * -ArenaDimensions * MeshScale.X),//X
					0,//Y
					OriginOffset.Z);
			}
			else if (Section.SectionType == EArenaSectionType::Polygon) {

				if (CurrentBOR == EArenaBuildOrderRules::GridLeadsByDimensions || CurrentBOR == EArenaBuildOrderRules::GridLeadsByRadius)
				{
					OriginOffset = FVector(-Apothem - (SideLength / 2) - (MeshSize.X * 3) / 4
						, ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2
						, OriginOffset.Z);
				}
				else if (CurrentBOR == EArenaBuildOrderRules::PolygonLeadByDimensions || CurrentBOR == EArenaBuildOrderRules::PolygonLeadByRadius)
				{
					OriginOffset = FVector(-(MeshSize.X * ArenaDimensions * MeshScale.X) + ((MeshSize.X * (0.5f * ArenaDimensions) * MeshScale.X) - (SideLength / 2))
						, ((MeshSize.X * ArenaDimensions * MeshScale.X) - (Apothem * 2)) / 2
						, OriginOffset.Z);
				}
			}
		}break;
		
	}
	
	//Update rotation parameters
	float RotationIncr = 360.f / Section.YawPossibilities;
	int YawPosMax = FMath::Clamp(Section.YawPossibilities-1, 2, 720);
	
	//Mesh Instancing, get mesh group
	if (Section.AssetToPlace == ETypeToPlace::StaticMeshes)
	{
		if (!UsedGroupIndices.Contains(GroupIdx))
		{
			UsedGroupIndices.Add(GroupIdx);

			TArray<UInstancedStaticMeshComponent*> ToInstance;

			for (FArenaMesh& ArenaMesh : MeshGroups[GroupIdx].GroupMeshes)
			{
				if (ArenaMesh.Mesh)
				{
					UInstancedStaticMeshComponent* InstancedMesh =
						NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());

					InstancedMesh->SetStaticMesh(ArenaMesh.Mesh);
					InstancedMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
					InstancedMesh->RegisterComponent();
			
					ToInstance.Add(InstancedMesh);
				}
			
			}

			//add tarray of instances to mesh instances
			MeshInstances.Add(ToInstance);
			ReRouteIdx = UsedGroupIndices.Find(GroupIdx);		//Using reroute index allows us to add mesh groups out of order to the mesh instances
			ArenaGenLog_Info("Adding the Mesh Group %d to Mesh Instances at index: %d ", GroupIdx, ReRouteIdx);
		}
		else 
		{
			ReRouteIdx = UsedGroupIndices.Find(GroupIdx);
			ArenaGenLog_Info("Index: %d is already instanced, ignoring request", GroupIdx);
		}
	}
	
	//Build Section
	switch (Section.SectionType) {
		case EArenaSectionType::Polygon:
		{
			FVector LastCachedPosition{ 0 };
			FVector SideAngleFV{ 0 };

			for (int SideIdx = 0; SideIdx < ArenaSides; ++SideIdx) //ArenaSides
			{
				//Cache last used position to update through next loop
				LastCachedPosition = LastCachedPosition + ((SideAngleFV * MeshSize.X));

				//Determine forward vector for placement
				SideAngleFV = ForwardVectorFromYaw(ExteriorAngle * SideIdx);

				//Cache Yaw rotation for the side
				float YawRotation = (360.f / ArenaSides) * SideIdx;

				//Determine right vector for placement offsets
				FVector SideAngleRV = FRotationMatrix(FRotator(0, YawRotation, 0)).GetScaledAxis(EAxis::Y);

				for (int LenIdx = 0; LenIdx < CurrTilesPerSide; ++LenIdx) //CurrTilesPerSide
				{
					LastCachedPosition = (SideAngleFV * MeshSize.X * (LenIdx > 0 ? 1 : 0)) + LastCachedPosition;

					for (int HeightIdx = 0; HeightIdx < Section.SectionAmount; ++HeightIdx)
					{
						//TODO - determine meshIdx
						int MeshIdx = 0;

						//Cache value for rotational mesh offsets
						int RandomVal{ 0 };

						switch (Section.RotationRule) {
						case EPlacementOrientationRule::RotateByYP:
							RandomVal = ArenaStream.RandRange(0, YawPosMax);
							//YawRotation = YawRotation + (RotationIncr * RandomVal);
							break;
						case EPlacementOrientationRule::RotateYawRandomly:
							YawRotation = ArenaStream.FRandRange(0, 360.f);
							break;
						}

						FVector RotationOffsetAdjustment = FVector(0);

						//We rotate the mesh around its assumed center and reposition it by its half size after the calculation.
						if (Section.AssetToPlace == ETypeToPlace::StaticMeshes)
						{
							RotationOffsetAdjustment = OffsetMeshToCenter(MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, Section.DefaultRotation.Yaw + YawRotation + (RotationIncr * RandomVal))
							- (OriginOffsetScalar(MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType) * MeshSize) //Offset back to lead position
							+ SideAngleFV * (FVector(0.5, 0.5, 0) * MeshSize.X);
						}
						

						FTransform TileTransform = FTransform(
						//ROTATION
							FRotator(
							Section.DefaultRotation.Pitch  //Pitch
							, Section.DefaultRotation.Yaw + YawRotation + (RotationIncr * RandomVal) //Yaw
							, Section.DefaultRotation.Roll), //Roll
						
						//LOCATION
							LastCachedPosition // Iterate on position...
							+ OriginOffset // Offset by the origin of our section 
							+ FVector(0, 0, (MeshSize.Z * (HeightIdx * Section.OffsetByHeightIncrement)) + HeightAdjustment) // Height Adjustment
							+ (SideAngleRV * MeshSize.Y * Section.InitOffsetByWidthScalar) // Initial width offset
							+ (SideAngleRV * MeshSize.Y * Section.OffsetByWidthIncrement * HeightIdx) // Offset by width each height increment
							+ RotationOffsetAdjustment // Adjust by offset caused by rotation and mesh origin type
							+ (bConcavity ? PlacementWarpingConcavity(CurrTilesPerSide / 2, CurrTilesPerSide / 2, LenIdx, HeightIdx, Section.WarpConcavityStrength, SideAngleRV) : FVector(0.f)) // Concavity
							+ (Section.bWarpPlacement ? PlacementWarpingDirectional(Section.WarpRange, SideAngleFV, SideAngleRV) : FVector(0)) //Warping along placement
							
						//SCALE
						, FVector( MeshScale.X, MeshScale.Y, MeshScale.Z));


						switch (Section.AssetToPlace) {
							default:
							case ETypeToPlace::StaticMeshes:
							{
								//Instancing mesh
								if (MeshInstances[ReRouteIdx][MeshIdx]) {
									MeshInstances[ReRouteIdx][MeshIdx]->AddInstance(TileTransform);
									TotalInstances++;
								}
								else {
									ArenaGenLog_Error("Could not find Mesh Instance of group: %d at index: %d", GroupIdx, MeshIdx);
								}
							}break;

							case ETypeToPlace::Actors:
							{
								FActorSpawnParameters SpawnParams = FActorSpawnParameters();
								SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
								SpawnParams.Owner = this;

								FTransform ActorTransform;

								AActor* ActorToSpawn = Cast<AActor>(GetWorld()->SpawnActor(ActorGroups[0].ClassesToSpawn[0], &ActorTransform, SpawnParams));
								ActorToSpawn->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

								ActorToSpawn->SetActorRelativeTransform(TileTransform, false);

								SpawnedActors.Add(ActorToSpawn);
								
							}break;
						}

						

					}
				}
			}

			//Cache values before scope ends
			PreviousTilesPerSide = CurrTilesPerSide;
			PreviousLastPosition = LastCachedPosition;

			if (Section.bUpdatesOriginOffsetHeight) {
				OriginOffset = FVector(OriginOffset.X, OriginOffset.Y, OriginOffset.Z + (MeshSize.Z * Section.SectionAmount * Section.OffsetByHeightIncrement)); // Update OriginOffset by height of mesh and scalar of height increment
			}
		}
		break;

		case EArenaSectionType::HorizontalGrid:
		{
			//Cache midpoint index
			int Midpoint = FMath::Floor(ArenaDimensions / 2);

			int SectionDimensions = (MeshSize.X == PreviousMeshSize.X) ? ArenaDimensions :
				FMath::Floor((2.f * SideLength) / MeshSize.X);

			FVector PlacementFV = FVector(1.f, 0.f, 0.f); //ForwardVectorFromYaw(Section.DefaultRotation.Yaw);
			FVector PlacementRV = FVector(0.f, 1.f, 0.f);//FRotationMatrix(FRotator(0, Section.DefaultRotation.Yaw, 0)).GetScaledAxis(EAxis::Y);

			for (int TimesIdx = 0; TimesIdx < Section.SectionAmount; TimesIdx++) {
				for (int Row = 0; Row < SectionDimensions; Row++) {
					for (int Col = 0; Col < SectionDimensions; Col++) {

						//TODO - determine meshIdx
						int MeshIdx = 0;

						//Rotation values
						int RandomVal=0;
						FVector RotationOffsetAdjustment{ 0 };
						float YawRotation{ 0.f };

						switch (Section.RotationRule) {
							
							case EPlacementOrientationRule::RotateByYP:
							{
								RandomVal = ArenaStream.RandRange(0, YawPosMax);
							}break;
							case EPlacementOrientationRule::RotateYawRandomly:
							{
								YawRotation = ArenaStream.FRandRange(0, 360.f);
							}break;

							
						}
						
						RotationOffsetAdjustment = //OffsetMeshAlongDirections(PlacementFV, PlacementRV, MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, RandomVal);
									//RotatedMeshOffset(MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, RandomVal);
							OffsetMeshToCenter(MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType, MeshSize, Section.DefaultRotation.Yaw + YawRotation + (RotationIncr * RandomVal))
							- (OriginOffsetScalar(MeshGroups[GroupIdx].GroupMeshes[MeshIdx].OriginType) * MeshSize) //Offset back to lead position
							+ (FVector(0.5, 0.5, 0) * MeshSize.X);

						//Transform
						FTransform TileTransform = FTransform(
							//ROTATION
							FRotator(
								Section.DefaultRotation.Pitch  //Pitch
								, YawRotation + (RotationIncr * RandomVal) //+ Section.DefaultRotation.Yaw //Yaw
								, Section.DefaultRotation.Roll //Roll
							),
							//LOCATION
							OriginOffset //Cached Origin offset
							+ FVector(0, 0, (MeshSize.Z * TimesIdx) + HeightAdjustment) //Height Offset for section amount and initial height adjustment
							+ (PlacementFV * MeshSize.X * MeshScale.X * Row) // Relative X placement
							+ (PlacementRV * MeshSize.Y * MeshScale.Y * Col) // Relative Y Placement
							+ RotationOffsetAdjustment //Offset from rotation by OriginType
							+ (Section.bWarpPlacement ? PlacementWarpingDirectional(Section.WarpRange, FVector(1, 0, 0), FVector(0, 1, 0)) : FVector(0)) //Warping along placement
							+ (bConcavity ? PlacementWarpingConcavity(CurrTilesPerSide / 2, CurrTilesPerSide / 2, Row, Col, Section.WarpConcavityStrength, FVector(0.f, 0.f, 1.f)) : FVector(0.f))

							//SCALE
							, MeshScale
						);

						switch (Section.AssetToPlace) {
						default:
						case ETypeToPlace::StaticMeshes:
						{
							//Instancing mesh
							if (MeshInstances[ReRouteIdx][MeshIdx]) {
								MeshInstances[ReRouteIdx][MeshIdx]->AddInstance(TileTransform);
								TotalInstances++;
							}
							else {
								ArenaGenLog_Error("Could not find Mesh Instance of group: %d at index: %d", GroupIdx, MeshIdx);
							}
						}break;

						case ETypeToPlace::Actors:
						{
							FActorSpawnParameters SpawnParams = FActorSpawnParameters();
							SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
							SpawnParams.Owner = this;

							AActor* ActorToSpawn = Cast<AActor>(GetWorld()->SpawnActor(ActorGroups[0].ClassesToSpawn[0], &TileTransform, SpawnParams));

							SpawnedActors.Add(ActorToSpawn);

						}break;
						}
					}
				}
			}

			if (Section.bUpdatesOriginOffsetHeight) {
				OriginOffset = FVector(OriginOffset.X, OriginOffset.Y, OriginOffset.Z + (MeshSize.Z * Section.SectionAmount * Section.OffsetByHeightIncrement)); // Update OriginOffset by height of mesh times scalar of height increment
			}
		}
		break;
	}

	//Cache values for next section
	PreviousMeshSize = MeshSize;
}


#pragma region Utility

float ABaseArenaGenerator::CalculateOpposite(float length, float angle)
{
	return (length * FMath::Cos(FMath::DegreesToRadians(angle)));
}

float ABaseArenaGenerator::CalculateAdjacent(float length, float angle)
{
	return (length * FMath::Sin(FMath::DegreesToRadians(angle)));
}

FVector ABaseArenaGenerator::ForwardVectorFromYaw(float yaw)
{
	float YawRad = FMath::DegreesToRadians(yaw);
	
	return FVector(FMath::Cos(YawRad),
		FMath::Sin(YawRad),
	0.f);
}

FVector ABaseArenaGenerator::RotatedMeshOffset(EOriginPlacementType OriginType, FVector& MeshSize, int RotationIndex)
{
	float X1 = 0;
	float Y1 = 0;

	switch (OriginType) {
	case(EOriginPlacementType::XY_Positive):
		if (RotationIndex == 1) {
			X1 = MeshSize.X;
		}
		else if (RotationIndex == 2) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			Y1 = MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::XY_Negative):
		if (RotationIndex == 0) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 1) {
			Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			X1 = MeshSize.X;
		}
		break;

	case(EOriginPlacementType::X_Positive_Y_Negative):
		if (RotationIndex == 0) {
			Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			X1 = MeshSize.X; 
		}
		else if (RotationIndex == 3) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::X_Negative_Y_Positive):
		if (RotationIndex == 0) {
			X1 = MeshSize.X;
		}
		else if (RotationIndex == 1) {
			X1 = MeshSize.X; Y1 = MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			Y1 = MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::Center):
		return FVector(0);
		break;
	}

	return FVector(X1, Y1, 0);
}

FVector ABaseArenaGenerator::OffsetMeshAlongDirections(const FVector& FV, const FVector& RV, EOriginPlacementType OriginType, const FVector& MeshSize, int RotationIndex)
{
	FVector Span;

	switch (OriginType) {
	case(EOriginPlacementType::XY_Positive):
		if (RotationIndex == 1) {
			Span = FV * MeshSize.X;
		}
		else if (RotationIndex == 2) {
			Span = FV * MeshSize.X + RV * MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			Span = RV * MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::XY_Negative):
		if (RotationIndex == 0) {
			Span = FV * MeshSize.X + RV * MeshSize.Y;
		}
		else if (RotationIndex == 1) {
			Span = RV * MeshSize.Y;
		}
		else if (RotationIndex == 3) {
			Span = FV * MeshSize.X;
		}
		break;

	case(EOriginPlacementType::X_Positive_Y_Negative):
		if (RotationIndex == 0) {
			Span = RV * MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			Span = FV * MeshSize.X;
		}
		else if (RotationIndex == 3) {
			Span = FV * MeshSize.X + RV * MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::X_Negative_Y_Positive):
		if (RotationIndex == 0) {
			Span = FV * MeshSize.X;
		}
		else if (RotationIndex == 1) {
			Span = FV * MeshSize.X + RV * MeshSize.Y;
		}
		else if (RotationIndex == 2) {
			Span = RV * MeshSize.Y;
		}
		break;

	case(EOriginPlacementType::Center):
		return FVector(0);
		break;
	}
	return Span;
}

FVector ABaseArenaGenerator::OriginOffsetScalar(EOriginPlacementType OriginType)
{
	switch (OriginType) {
	case(EOriginPlacementType::XY_Positive):
		return FVector(0.5, 0.5, 0);
		break;
	case(EOriginPlacementType::XY_Negative):
		return FVector(-0.5, -0.5, 0);
		break;
	case(EOriginPlacementType::X_Positive_Y_Negative):
		return FVector(0.5, -0.5, 0);
		break;
	case(EOriginPlacementType::X_Negative_Y_Positive):
		return FVector(-0.5, 0.5, 0); 
		break;
	case(EOriginPlacementType::Center):
		return FVector(0);
		break;
	}

	return FVector(0);
}

FVector ABaseArenaGenerator::PlacementWarpingConcavity(int ColMidpoint, int RowMidpoint, int Col, int Row, float ConcavityStrength, FVector WarpDirection)
{
	float ConcaveWarp = ConcavityStrength *
		(FMath::Clamp((FMath::Lerp(0.f, 1.f, FMath::Clamp((static_cast<float>(abs(Col - ColMidpoint)) / RowMidpoint), 0.f, 1.f)) *
		FMath::Lerp(0.f, 1.f, FMath::Clamp((static_cast<float>(abs(Row - RowMidpoint)) / RowMidpoint), 0.f, 1.f))), 0.f, 1.f));

	return  (WarpDirection * ConcaveWarp); 

}

FVector ABaseArenaGenerator::PlacementWarpingDirectional(FVector OffsetRanges, const FVector& DirFV, const FVector& DirRV)
{

	return (ArenaStream.FRandRange(OffsetRanges.X * -1, OffsetRanges.X) * DirFV)
		+ (ArenaStream.FRandRange(OffsetRanges.Y * -1, OffsetRanges.Y) * DirRV)
		+ FVector(0,0, ArenaStream.FRandRange(OffsetRanges.Z * -1, OffsetRanges.Z));// FVector();
}

FVector ABaseArenaGenerator::OffsetMeshToCenter(EOriginPlacementType OriginType, const FVector& MeshSize, float angle)
{
	if (OriginType == EOriginPlacementType::Center) { return FVector(0); } //early return if origin type is zero

	float AngleRad = FMath::DegreesToRadians(angle);
	float cosTheta = FMath::Cos(AngleRad);
	float sinTheta = FMath::Sin(AngleRad);
	FVector InitialCenter = OriginOffsetScalar(OriginType) * MeshSize; //determines the offset direction for calculation based on origin type

	FVector RotatedCenter = FVector(((InitialCenter.X * cosTheta) - (InitialCenter.Y * sinTheta)), ((InitialCenter.X * sinTheta) + (InitialCenter.Y * cosTheta)), 0);

	return InitialCenter - RotatedCenter;
}


void ABaseArenaGenerator::ConvertToStaticMeshActors()
{

	if (!MeshInstances.IsEmpty())
	{
		for (auto& Inst : MeshInstances)
		{
			for (auto& Component : Inst) {
				if (!Component || Component->GetStaticMesh() == nullptr) { continue; }

				int32 NumInsts = Component->GetInstanceCount();
				ArenaGenLog_Info("Converting %d instances into static mesh actors", NumInsts);

				for (int32 i = 0; i < NumInsts; ++i)
				{
					FTransform InstTransform;
					if (Component->GetInstanceTransform(i, InstTransform, true))
					{
						AStaticMeshActor* NewMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), InstTransform);

						if (NewMeshActor)
						{
							NewMeshActor->GetStaticMeshComponent()->SetStaticMesh(Component->GetStaticMesh());
						}
					}
				}
			}
		}
	}
	else {
		ArenaGenLog_Warning("No mesh instances to convert. Generate arena first to convert it!");
	}


}



#pragma endregion

