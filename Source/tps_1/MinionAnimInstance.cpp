// Fill out your copyright notice in the Description page of Project Settings.


#include "MinionAnimInstance.h"
#include "Enemy.h"

void UMinionAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (Enemy == nullptr)
	{
		Enemy = Cast<AEnemy>(TryGetPawnOwner());
	}

	if (Enemy)
	{
		FVector Velocity{ Enemy->GetVelocity() };
		Velocity.Z = 0.f;
		Speed = Velocity.Size();

	}
}