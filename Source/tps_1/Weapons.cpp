// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons.h"

AWeapons::AWeapons() :
	ThrowWeaponTime(0.7f),
	bFalling(false),
	Ammo(30),
	MagazineCapacity(230),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9mm),
	ReloadMontageSection(FName(TEXT("Reload SMG")))
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapons::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep the Weapon upright
	if (GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		const FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	// Update slide on pistol
//	UpdateSlideDisplacement();

}

	void AWeapons::ThrowWeapon()
	{
		FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

		const FVector MeshForward{ GetItemMesh()->GetForwardVector() };
		const FVector MeshRight{ GetItemMesh()->GetRightVector() };
		// Direction in which we throw the Weapon
		FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

		float RandomRotation{ 30.f };
		ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
		ImpulseDirection *= 2'000.f;
		GetItemMesh()->AddImpulse(ImpulseDirection);

		bFalling = true;
		GetWorldTimerManager().SetTimer(
			ThrowWeaponTimer,
			this,
			&AWeapons::StopFalling,
			ThrowWeaponTime);

	}


	

	void AWeapons::StopFalling()
	{
		bFalling = false;
		SetItemState(EItemState::EIS_Pickup);
		//StartPulseTimer();
	} 

	void AWeapons::DecrementAmmo()
	{
		if (Ammo - 1 <= 0)
		{
			Ammo = 0;
		}
		else
		{
			--Ammo;
		}
		GEngine->AddOnScreenDebugMessage(1, -4, FColor::Blue,
		FString::Printf(TEXT("Ammo:%int32"), Ammo));
	}


	void AWeapons::ReloadAmmo(int32 Amount)
	{
		checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attempted to reload with more than magazine capacity!"));
		Ammo += Amount;
	}


	bool AWeapons::ClipIsFull()
	{
		return Ammo >= MagazineCapacity;
	}
