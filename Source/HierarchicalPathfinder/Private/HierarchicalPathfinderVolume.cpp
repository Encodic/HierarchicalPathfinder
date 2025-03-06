// Fill out your copyright notice in the Description page of Project Settings.


#include "HierarchicalPathfinderVolume.h"
#include "Components/BrushComponent.h"

// Sets default values
AHierarchicalPathfinderVolume::AHierarchicalPathfinderVolume()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

bool AHierarchicalPathfinderVolume::ClearNavVolume()
{
	GridNodes.Empty();
	NavClusters.Empty();
	return true;
}

bool AHierarchicalPathfinderVolume::GenerateNavVolume()
{
	if(ClearNavVolume())
	{
		UpdatePFState(EHierarchicalPF_State::NotInitialised);
	}
	else return false;
	
	GetStartTime(PFStartTime);
	
	UpdatePFState(EHierarchicalPF_State::Initialised);

	return true;
}

bool AHierarchicalPathfinderVolume::GenerateNavClusters()
{
	ClusterGridSize.X = FMath::DivideAndRoundUp(GridSize.X , ClusterSize);
	ClusterGridSize.Y = FMath::DivideAndRoundUp(GridSize.Y , ClusterSize);

	for (int x = 0; x < ClusterGridSize.X; ++x)
	{
		for (int y = 0; y < ClusterGridSize.Y; ++y)
		{
			NavClusters.Add(FIntVector2(x ,y));
		}
	}
	
	UpdatePFState(EHierarchicalPF_State::ClustersReady);
	return true;
}

bool AHierarchicalPathfinderVolume::GenerateGridData()
{
	const FBoxSphereBounds VolumeBounds = GetBrushComponent()->CalcBounds(GetBrushComponent()->GetComponentTransform());
	const FVector VolumeExtent = VolumeBounds.GetBox().GetExtent();

	// Calculate the amount of nodes
	GridSize.X = (FMath::Floor(VolumeExtent.X / NodeSize)) * 2;
	GridSize.Y = (FMath::Floor(VolumeExtent.Y / NodeSize)) * 2;

	GridCornerOffset2D.X = -((GridSize.X / 2) * NodeSize);
	GridCornerOffset2D.Y = -((GridSize.Y / 2) * NodeSize);

	UpdatePFState(EHierarchicalPF_State::GridGenerationDataReady);
	return true;
}

bool AHierarchicalPathfinderVolume::PopulateNavGrid()
{
	for (int x = 0; x < GridSize.X; ++x)
	{
		for (int y = 0; y < GridSize.Y; ++y)
		{
			GridNodes.Add(FIntVector2(x, y));
		}
	}

	UpdatePFState(EHierarchicalPF_State::GridReady);
	return (GridNodes.Num() == (GridSize.X * GridSize.Y));
}

void AHierarchicalPathfinderVolume::UpdatePFState(const EHierarchicalPF_State DesiredState)
{
	if (DesiredState != GetCurrentHPVState())
	{
		if(DesiredState == EHierarchicalPF_State::NotInitialised)
		{
			SetHPVState(DesiredState);
			PreviousTime = 0.0f;
		}
		else if (GetCurrentHPVState() == EHierarchicalPF_State::NotInitialised && DesiredState == EHierarchicalPF_State::Initialised)
		{
			SetHPVState(DesiredState);

			PreviousTime = DebugTime(PFStartTime, "Initialization", true);
			GenerateGridData();
		}
		else if(GetCurrentHPVState() == EHierarchicalPF_State::Initialised && DesiredState == EHierarchicalPF_State::GridGenerationDataReady)
		{
			SetHPVState(DesiredState);
			
			PreviousTime = DebugTime(PFStartTime, "GenerateGridData", true);
			PopulateNavGrid();
		}
		else if (GetCurrentHPVState() == EHierarchicalPF_State::GridGenerationDataReady && DesiredState == EHierarchicalPF_State::GridReady)
		{
			SetHPVState(DesiredState);
			
			PreviousTime = DebugTime(PFStartTime, "PopulateNavGrid", true);
			GenerateNavClusters();
		}
		else if (GetCurrentHPVState() == EHierarchicalPF_State::GridReady && DesiredState == EHierarchicalPF_State::ClustersReady)
		{
			SetHPVState(DesiredState);
			
			PreviousTime = DebugTime(PFStartTime, "GenerateNavClusters", true);
			GenerateNodeData();
		}
		else if (GetCurrentHPVState() == EHierarchicalPF_State::ClustersReady && DesiredState == EHierarchicalPF_State::NodeDataReady)
		{
			SetHPVState(DesiredState);
			
			PreviousTime = DebugTime(PFStartTime, "GenerateNodeData", true);
			UpdatePFState(EHierarchicalPF_State::PathfinderVolumeReady);
		}
		else if (GetCurrentHPVState() == EHierarchicalPF_State::NodeDataReady && DesiredState == EHierarchicalPF_State::PathfinderVolumeReady)
		{
			SetHPVState(DesiredState);
			
			PreviousTime = DebugTime(PFStartTime, "GenerateNodeData", true);
			DebugHPVolume();
		}
	}
}

#pragma region Node Data

bool AHierarchicalPathfinderVolume::GenerateNodeData()
{
	for (auto& Node : GridNodes)
	{
		Node.Value.NodeCoords = Node.Key;
		Node.Value.ClusterID = GetNodeClusterID(Node.Key);
		Node.Value.NodeNeighbours = GetNodeNeighbours(Node.Key);
		Node.Value.NodeOffset = GetNodeOffset(Node.Key);
		Node.Value.NodeExtent = (NodeSize / 2);

		// Add node to cluster
		NavClusters.Find(Node.Value.ClusterID)->ClusterNodeChildren.Add(Node.Key);
	}

	UpdatePFState(EHierarchicalPF_State::NodeDataReady);
	return true;
}

TMap<FName, FIntVector2> AHierarchicalPathfinderVolume::GetNodeNeighbours(const FIntVector2 Node) const
{
	TMap<FName, FIntVector2> Neighbours;
	TMap<FName, FIntVector2> NeighbourCoordsToCheck;
	
	NeighbourCoordsToCheck.Add("North",Node + FIntVector2(0, 1));			// North
	NeighbourCoordsToCheck.Add("East", Node + FIntVector2(1, 0));			// East
	NeighbourCoordsToCheck.Add("South", Node + FIntVector2(0, -1));		// South
	NeighbourCoordsToCheck.Add("West", Node + FIntVector2(-1, 0));		// West
	NeighbourCoordsToCheck.Add("NorthEast", Node + FIntVector2(1, 1));	// North East
	NeighbourCoordsToCheck.Add("NorthWest", Node + FIntVector2(-1, 1));	// North West
	NeighbourCoordsToCheck.Add("SouthEast", Node + FIntVector2(1, -1));	// South East
	NeighbourCoordsToCheck.Add("SouthWest", Node + FIntVector2(-1, -1));	// South West

	// Iterates through possible neighbours to check if neighbour node is a valid node.
	for (auto& NeighbourNode : NeighbourCoordsToCheck)
	{
		// If valid add it
		if(GridNodes.Contains(NeighbourNode.Value))
		{
			Neighbours.Add(NeighbourNode);
		}
	}
	
	return Neighbours;
}

FVector2D AHierarchicalPathfinderVolume::GetNodeOffset(const FIntVector2 Node) const
{
	FVector2D Offset;
	Offset.X = ((NodeSize / 2) + GridCornerOffset2D.X + (Node.X * NodeSize));
	Offset.Y = ((NodeSize / 2) + GridCornerOffset2D.Y + (Node.Y * NodeSize));
	
	return Offset;
}

FIntVector2 AHierarchicalPathfinderVolume::GetNodeClusterID(const FIntVector2 Node) const
{
	const int X = FMath::DivideAndRoundDown(Node.X, ClusterSize);
	const int Y = FMath::DivideAndRoundDown(Node.Y, ClusterSize);

	return FIntVector2(X,Y);
}

FVector AHierarchicalPathfinderVolume::GetNodeWorldLocation(const FIntVector2 Node)
{
	const FVector2D Offset = GridNodes.Find(Node)->NodeOffset;
	return NodeOffsetToWorld(FVector(Offset.X, Offset.Y, 0));
}

FVector AHierarchicalPathfinderVolume::NodeOffsetToWorld(const FVector NodeOffset) const
{
	FVector WorldPos = RotateVectorAroundPivot((NodeOffset + GetActorLocation()), GetActorRotation(), GetActorLocation());

	return FVector(WorldPos);
}

#pragma endregion

#pragma region Debugs

float AHierarchicalPathfinderVolume::DebugTime(const float StartTime, const FName ProcessName, const bool IncludeExecution) const
{
	if (bDebug)
	{
		const float Time = FPlatformTime::Seconds() - StartTime;
		FString DebugMessage;
		if(!IncludeExecution) DebugMessage = FString::Printf(TEXT("HierarchicalPathfinderVolume : %s completed at [ %f ] seconds."), *ProcessName.ToString(), Time);
		else DebugMessage = FString::Printf(TEXT("HierarchicalPathfinderVolume : %s completed at [ %f ] seconds. ExecutionTime = [ %f ] seconds."), *ProcessName.ToString(), Time, (Time - PreviousTime));
			
		UE_LOG(LogTemp, Log, TEXT("%s"), *DebugMessage);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, DebugMessage, true, FVector2D::UnitVector);

		return Time;
	}
	return 0.0f;
}

void AHierarchicalPathfinderVolume::DebugNumNodes() const
{
	if (bDebug)
	{
		const FString DebugMessage = FString::Printf(TEXT("HierarchicalPathfinderVolume : [ %i ] nodes Generated."), GridNodes.Num());
		UE_LOG(LogTemp, Log, TEXT("%s"), *DebugMessage);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, DebugMessage, true, FVector2D::UnitVector);
	}
}

void AHierarchicalPathfinderVolume::DebugHPVolume()
{
	if (bDebug)
	{
		if (bDrawNodes)
		{
			for (const auto& Node : GridNodes)
			{
				DrawNode(Node.Key);
			}
		}

		if (bDrawClusters)
		{
			for (const auto& Cluster : NavClusters)
			{
				DrawCluster(Cluster.Key);
			}
		}

		DebugNumNodes();
	}
}

void AHierarchicalPathfinderVolume::DrawNode(const FIntVector2 Node)
{
	float NodeExtent = (GridNodes.Find(Node)->NodeExtent) - DrawNodeLineThickness;
	DrawDebugBox(GetWorld(), GetNodeWorldLocation(Node), FVector(NodeExtent), FColor::MakeRandomColor(), false, DrawNodeTime, 0, DrawNodeLineThickness);
}

void AHierarchicalPathfinderVolume::DrawCluster(const FIntVector2 ClusterID)
{
	FNavCluster* Cluster = NavClusters.Find(ClusterID);
	
	FColor ClusterColor = FColor::MakeRandomColor();

	// TODO: Draw Cluster Border

	if (bDrawClusterNodes)
	{
		for (auto& Node : Cluster->ClusterNodeChildren)
		{
			float NodeExtent = (GridNodes.Find(Node)->NodeExtent) - DrawNodeLineThickness;
			DrawDebugBox(GetWorld(), GetNodeWorldLocation(Node), FVector(NodeExtent), ClusterColor, false, DrawClusterTime, 0, DrawNodeLineThickness);
		}
	}
}

#pragma endregion

FVector AHierarchicalPathfinderVolume::RotateVectorAroundPivot(FVector InVector, FRotator Rotation, FVector Pivot)
{
	FVector NewVector = (Rotation.RotateVector((InVector - Pivot))) + Pivot;
	return NewVector;
}

// Called when the game starts or when spawned
void AHierarchicalPathfinderVolume::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHierarchicalPathfinderVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

