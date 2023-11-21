// Fill out your copyright notice in the Description page of Project Settings.
#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include"Item.h"
#include "Components/WidgetComponent.h"
#include "Weapons.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Ammo.h"
#include "BulletHitInterface.h"
#include "Components/CapsuleComponent.h"
#include "Enemy.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
// Sets default values
//THIS IS A CONSTRUCTOR


/*

DeltaTime: The time between frames
Frame: A single image updated to the screen 
Frame Rate: Number of frames updated per second (fps)

tick: Synonymous with Frame

*/
AShooterCharacter::AShooterCharacter() :

	//base rate for turning / look up
	BaseLookUpRate(45.f),
	BaseTurnRate(45.f),

	//true when aiming
	bAiming(false),

	//turn rate for aiming/ not aiming
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingLookUpRate(20.f),
	AimingTurnRate(20.f),


	// 
	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingLookUpRate(0.2f),
	MouseAimingTurnRate(0.2f),

	//camera field of view 
	CameraDefaultFOV(0.f), // set in BeginPlay
	CameraZoomedFOV(35.f),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(20.f),

	// Crosshair spread factors
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),

	AutomaticFireRate(0.1f),
	bShouldFire(true),
	bFireButtonPressed(false),

	// Bullet fire timer variables
	bFiringBullet(false),
	ShootTimeDuration(0.05f),


	// THESE ARE ACTUALLY THE FULL AMOUNT OF TOTAL AMMO CHARACTER CAN CARRY
	// Starting ammo amounts
	Starting9mmAmmo(120),
	StartingARAmmo(120),

	Health(100.f),
	MaxHealth(100.f),
	bDead(false),

	StunChance(.25f),
	// combat variables
	CombatState(ECombatState::ECS_Unoccupied)

{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Crate a camera boom (pulls in towards the character if there is a collision) 
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.f;//The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true;//rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	//CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	//create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow camera"));

	//Attach camera to end of boom
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	//camera does not rotate relative to arm
	FollowCamera->bUsePawnControlRotation = false; 


	//Dont rotate when the camera rotates . Let the controller only effect the camera
	bUseControllerRotationPitch = false;
	//changed from false to true cause of target offset
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	//character moves in this direction of input
	//changed from true to false cause of target offset
	GetCharacterMovement()->bOrientRotationToMovement = false;
	//at this rotation rate
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;


}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

  

	if (FollowCamera) {
	CameraDefaultFOV = GetFollowCamera()->FieldOfView;
	CameraCurrentFOV = CameraDefaultFOV;
	}

	// spawn the default weapon and attach it to the mesh
//	SpawnDefaultWeapon();
	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);
	EquippedWeapon->SetSlotIndex(0);
	InitializeAmmoMap();

	// EDIT
	// Ignore the camera for Mesh and Capsule
	GetMesh()->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);

	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore
	);
}

void AShooterCharacter::MoveForward(float Value)
{

	//checking if inherited Variable Controller is valid 
	//and if value is 0 there is no input from user
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		//find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };

		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		//creating a matrix out of yawRotation  then getting forward X axis from the rotation matrix
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		
		// value can be 0, -1 and 1 for direction 
		AddMovementInput(Direction, Value);

	}
}

void AShooterCharacter::MoveRight(float Value)
{

	if ((Controller != nullptr) && (Value != 0.0f))
	{
		//find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };


		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);

	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{

	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());//deg/sec * sec/frame

}

void AShooterCharacter::LookUpRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate	* GetWorld()->GetDeltaSeconds());//deg/sec * sec/frame
}

void AShooterCharacter::Turn(float Value)
{

	float TurnScaleFactor{};
	if (bAiming) {
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else {
		TurnScaleFactor = MouseHipTurnRate;
	}

	AddControllerYawInput(Value * TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if (bAiming) {
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else {
		LookUpScaleFactor = MouseHipLookUpRate; 
	}

	AddControllerPitchInput(Value * LookUpScaleFactor);

}

void AShooterCharacter::FireWeapon()
{
	
	if (EquippedWeapon == nullptr) return;
	//UE_LOG(LogTemp, Warning, TEXT("Fire Weapon"));

	//FORMAT
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if(WeaponHasAmmo()){

	// play fire sound 
	/*
	if (FireSound) {
	UGameplayStatics::PlaySound2D(this, FireSound);
	}
	*/
	PlayFireSound();

	// send bullet
	SendBullet();
		

		// get current size of the viewport 
		/*FVector2D ViewportSize;
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(ViewportSize);
		}*/

		// get screen space location of crosshairs
	/*	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		CrosshairLocation.Y -= 50.f;
		FVector CrosshairWorldPosition;
		FVector CrosshairWorldDirection;*/


		// get world position and direction of crosshairs
	/*	bool bScreentoWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
			CrosshairLocation,
			CrosshairWorldPosition,
			CrosshairWorldDirection);*/

		//if (bScreentoWorld)// if deprojection successfull
		//{
		//	FHitResult ScreenTraceHit;
		//	const FVector Start{ CrosshairWorldPosition };
		//	const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000.f };


		//	// set beam end point to line trace end point 
		//	FVector BeamEndPoint{ End };

		//	//trace outward from crosshairs world location
		//	GetWorld()->LineTraceSingleByChannel(
		//		ScreenTraceHit,
		//		Start,
		//		End,
		//		ECollisionChannel::ECC_Visibility);

		//	if (ScreenTraceHit.bBlockingHit)
		//	{
		//		//beam end point is now trace hit location
		//		BeamEndPoint = ScreenTraceHit.Location;
		//		/*
		//		if (ImpactParticles)
		//		{
		//			UGameplayStatics::SpawnEmitterAtLocation(
		//				GetWorld(),
		//				ImpactParticles,
		//				ScreenTraceHit.Location);
		//		
		//		}
		//		*/
		//	}

		//	//perform a second trace this time from the gun barrel
		//	FHitResult WeaponTraceHit;
		//	const FVector WeaponTraceStart{ SocketTransform.GetLocation() };
		//	const FVector WeaponTraceEnd{ BeamEndPoint };
		//	GetWorld()->LineTraceSingleByChannel(
		//		WeaponTraceHit,
		//		WeaponTraceStart,
		//		WeaponTraceEnd,
		//		ECollisionChannel::ECC_Visibility);
		//	if (WeaponTraceHit.bBlockingHit)
		//	{
		//		BeamEndPoint = WeaponTraceHit.Location;
		//	}

		//	//Spawn impact particles after updating beamendPoint
		//	if (ImpactParticles)
		//	{
		//		UGameplayStatics::SpawnEmitterAtLocation(
		//			GetWorld(),
		//			ImpactParticles,
		//			BeamEndPoint);

		//	}

		//	if (BeamParticles) {
		//		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation
		//		(GetWorld(), BeamParticles, SocketTransform);

		//		if (Beam) {
		//			Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
		//		}
		//	}
		//}



		/*

		FHitResult FireHit;
		const FVector Start{ SocketTransform.GetLocation() };
		const FQuat Rotation{ SocketTransform.GetRotation() };
		const FVector RotationAxis{ Rotation.GetAxisX() };
		const FVector End{ Start + RotationAxis * 50'000.f };
		
		FVector BeamEndPoint{ End };

		GetWorld()->LineTraceSingleByChannel(FireHit ,Start, End , ECollisionChannel::ECC_Visibility);
		
		if (FireHit.bBlockingHit)
		{
			//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f);
			//DrawDebugPoint(GetWorld(), FireHit.Location, 5.f, FColor::Red, false, 2.f);

			BeamEndPoint = FireHit.Location;

			if (ImpactParticles) {
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.Location);
			}
		}

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);

			if (Beam) {
				Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
			}
		}
		*/
	
	// play gun fire montage
	PlayGunfireMontage();

	

	// Decrement Ammo
	EquippedWeapon->DecrementAmmo();
	/*if (EquippedWeapon) {
		//subtract 1 from the weapon ammo
		EquippedWeapon->DecrementAmmo();
	}
	*/
	//Start Fire Timer
	StartFireTimer();

	StartCrosshairBulletFire();
	}
	//StartCrosshairBulletFire();
}
void AShooterCharacter::PlayGunfireMontage()
{
	// Play Hip Fire Montage
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}


void AShooterCharacter::ReloadButtonPressed()
{

ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (EquippedWeapon == nullptr) return;

	// Do we have ammo of the correct type?
	if (CarryingAmmo() )
	//&& !EquippedWeapon->ClipIsFull())
	{/*
		if (bAiming)
		{
		StopAiming();
		}*/
		// create an enum for weapon type
		// switch on equipped weapon -> weapon type

		CombatState = ECombatState::ECS_Reloading;


		//FName MontageSection(TEXT("Reload SMG"));

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);

		//	AnimInstance->Montage_JumpToSection(MontageSection);

			AnimInstance->Montage_JumpToSection(
			EquippedWeapon->GetReloadMontageSection());
		}
	}
}
 
bool AShooterCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr) return false;

auto AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType))
	{
	return AmmoMap[AmmoType] > 0;
	}

	return false;

}

void AShooterCharacter::PlayFireSound()
{
	// Play fire sound
	/*if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}*/
	if (FireSound) {
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
}

void AShooterCharacter::SendBullet()
{
	//const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket) {
		//const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}

//		FVector BeamEnd;
		FHitResult BeamHitResult;
		bool bBeamEnd = GetBeamEndLocation(
			SocketTransform.GetLocation(), BeamHitResult);
		if (bBeamEnd) {

			// Does hit Actor implement Bullet Hit Interface 
			if (BeamHitResult.Actor.IsValid()) {
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.Actor.Get());
				if (BulletHitInterface) {
				BulletHitInterface->BulletHit_Implementation(BeamHitResult);
				}



				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.Actor.Get());
				if (HitEnemy)
				{
					int32 Damage{};
					if (BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadBone())
					{
						// Head shot
						Damage = EquippedWeapon->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.Actor.Get(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						UE_LOG(LogTemp, Warning, TEXT("HIT :%s"), *BeamHitResult.BoneName.ToString());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, true);
					}
					else
					{
						// Body shot
						Damage = EquippedWeapon->GetDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.Actor.Get(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, false);
					}


				} 

			}
			if (ImpactParticles) {
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						BeamHitResult.Location
					);
				}
			
			

			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				BeamParticles,
				SocketTransform
			);
			if (Beam) {
				Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
			}
		}

	}
}

/*
bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocaton, FVector& OutBeamLocation)
{  

	
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit) {
	//Tentative beam location - still need to trace from gun

		OutBeamLocation = CrosshairHitResult.Location;
	}

	else // no crosshair trace hit
	{
	
		// OutBeamLocation is the End location for the line trace

	}


	//perform a second trace this time from the gun barrel
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart{ MuzzleSocketLocaton };

	const FVector StartToEnd{ OutBeamLocation - MuzzleSocketLocaton };

	const FVector WeaponTraceEnd{ MuzzleSocketLocaton + StartToEnd * 1.25f };
	GetWorld()->LineTraceSingleByChannel(
		WeaponTraceHit,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);
	if (WeaponTraceHit.bBlockingHit)
	{
		OutBeamLocation = WeaponTraceHit.Location;
		return true;
	}


	/*

	// get current size of the viewport 
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	CrosshairLocation.Y -= 50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// get world position and direction of crosshairs
	bool bScreentoWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);



	if (bScreentoWorld)// if deprojection successfull
	{
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000.f };


		// set beam end point to line trace end point 

		OutBeamLocation = End;

		//trace outward from crosshairs world location
		GetWorld()->LineTraceSingleByChannel(
			ScreenTraceHit,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);

		if (ScreenTraceHit.bBlockingHit)
		{
			//beam end point is now trace hit location
		//	BeamEndPoint = ScreenTraceHit.Location;

			OutBeamLocation = ScreenTraceHit.Location;
			/*
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					ScreenTraceHit.Location);

			}
			
		}


		//perform a second trace this time from the gun barrel
		FHitResult WeaponTraceHit;
		const FVector WeaponTraceStart{ MuzzleSocketLocaton };
		const FVector WeaponTraceEnd{ OutBeamLocation };
		GetWorld()->LineTraceSingleByChannel(
			WeaponTraceHit,
			WeaponTraceStart,
			WeaponTraceEnd,
			ECollisionChannel::ECC_Visibility);
		if (WeaponTraceHit.bBlockingHit)
		{
	    OutBeamLocation = WeaponTraceHit.Location;
		}

		return true;


	}

	return false;

	
}
*/
bool AShooterCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation,
	FHitResult& OutHitResult)
{
	FVector OutBeamLocation;
	// Check for crosshair trace hit
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		// Tentative beam location - still need to trace from gun
		OutBeamLocation = CrosshairHitResult.Location;
	}
	else // no crosshair trace hit
	{
		// OutBeamLocation is the End location for the line trace
	}

	// Perform a second trace, this time from the gun barrel
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector WeaponTraceEnd{ OutBeamLocation };
	GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);
	if (!OutHitResult.bBlockingHit) // object between barrel and BeamEndPoint?
	{
		OutHitResult.Location = OutBeamLocation;
		return false;
	}

	return true;
}

void AShooterCharacter::AimingButtonPressed()
{


	bAiming = true;
//	GetFollowCamera()->SetFieldOfView(CameraZoomedFOV);
//	CameraCurrentFOV = CameraDefaultFOV;
}

void AShooterCharacter::AimingButtonReleased()
{

	bAiming = false;
//	GetFollowCamera()->SetFieldOfView(CameraDefaultFOV);
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{

	if (bAiming)
	{
		//interpolate to zoomed FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);

	}
	else {
		//interpolate to deafult FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);

	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::SetLookRates()
{


	if (bAiming) {
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else {
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}


// crosshair spread



void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.f, 600.f };
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;

	// Calculate crosshair velocity factor
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange,
		VelocityMultiplierRange,
		Velocity.Size());

	// Calculate crosshair in air factor
	if (GetCharacterMovement()->IsFalling()) // is in air?
	{
		// Spread the crosshairs slowly while in air
		CrosshairInAirFactor = FMath::FInterpTo(
			CrosshairInAirFactor,
			2.25f,
			DeltaTime,
			2.25f);
	}
	else // Character is on the ground
	{
		// Shrink the crosshairs rapidly while on the ground
		CrosshairInAirFactor = FMath::FInterpTo(
			CrosshairInAirFactor,
			0.f,
			DeltaTime,
			30.f);
	}

	// Calculate crosshair aim factor
	if (bAiming) // Are we aiming?
	{
		// Shrink crosshairs a small amount very quickly
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.6f,
			DeltaTime,
			30.f);
	}
	else // Not aiming
	{
		// Spread crosshairs back to normal very quickly
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.f,
			DeltaTime,
			30.f);
	}

	// True 0.05 second after firing
	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.3f,
			DeltaTime,
			60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.f,
			DeltaTime,
			60.f);
	}

	CrosshairSpreadMultiplier =
		0.5f +
		CrosshairVelocityFactor +
		CrosshairInAirFactor -
		CrosshairAimFactor +
		CrosshairShootingFactor;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(
		CrosshairShootTimer,
		this,
		&AShooterCharacter::FinishCrosshairBulletFire,
		ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

/////////////////////////////////////////////////////




void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	/*
	if (WeaponHasAmmo()) {
	
		StartFireTimer();
	}
	*/
	FireWeapon();
	
}

void AShooterCharacter::FireButtonReleased()
{

	bFireButtonPressed = false;

}

void AShooterCharacter::StartFireTimer()
{

//	if (bShouldFire) {

	//	FireWeapon();

	//	bShouldFire = false;
	if (EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_FireTimerInProgress;
		GetWorldTimerManager().SetTimer(

			AutoFireTimer,
			this,
			&AShooterCharacter::AutoFireReset,
			AutomaticFireRate
		);
//}
	
}

void AShooterCharacter::AutoFireReset()
{	
	/*
	if (WeaponHasAmmo()) {
		bShouldFire = true;
		if (bFireButtonPressed)
		{
			StartFireTimer();
		}

	}*/

	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;

	if (EquippedWeapon == nullptr) return;
	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed )
		//	&& EquippedWeapon->GetAutomatic()
		{
			FireWeapon();
		}
	}
	else
	{
		// Reload Weapon
		ReloadWeapon();
	}
	
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	// get current size of the viewport 
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	CrosshairLocation.Y -= 50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// get world position and direction of crosshairs
	bool bScreentoWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);



	if (bScreentoWorld)// if deprojection successfull
	{
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50'000.f };
		OutHitLocation = End;
		//trace outward from crosshairs world location
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{	
			OutHitResult.Location;
			//beam end point is now trace hit location
			return true;

		}
		
	}

	return false;
}

void AShooterCharacter::TraceForItems()
{

	if (bShouldTraceForItems) {
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit) {

		//	AItem* HitItem = Cast<AItem>(ItemTraceResult.Actor);
			TraceHitItem = Cast<AItem>(ItemTraceResult.Actor);

			if (TraceHitItem && TraceHitItem->GetPickupWidget()) {
				//show item pickup widget
				TraceHitItem->GetPickupWidget()->SetVisibility(true);

			}  

			//We hit an AItem last frame
			if (TraceHitItemLastFrame) {
				if (TraceHitItem != TraceHitItemLastFrame) {

					//We are hittin a different AItem this frame than last frame
					//Or AItem is null
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
				}
			}
			//store a reference to HitItem for last frame
			TraceHitItemLastFrame = TraceHitItem;
		}

	}

	else if (TraceHitItemLastFrame) {
	//No longer overlapping any items,
	//Items last frame should not show widget
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
	}
}



AWeapons* AShooterCharacter::SpawnDefaultWeapon()
{	//Check the TSubclassOf variable
	if (DefaultWeaponClass) {

		//Spawn the Weapon 
		//AWeapon* DefaultWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		return GetWorld()->SpawnActor<AWeapons>(DefaultWeaponClass);


		////Get the Hand Socket 
		//const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(
		//	FName("RightHandSocket")
		//);

		//if (HandSocket) {
		//	//Attach the Weapon to the hand socket RightHandSocket
		//	HandSocket->AttachActor(DefaultWeapon, GetMesh());
		//}

		//// set equipped weapon to the newly spawned weapon
		//EquippedWeapon = DefaultWeapon;
	}
	return nullptr;

}

void AShooterCharacter::EquipWeapon(AWeapons* WeaponToEquip)
{
	if (WeaponToEquip)
	{
		/*
		// set area sphere to ignore all collision channels
		WeaponToEquip->GetAreaSphere()->SetCollisionResponseToAllChannels(
		ECollisionResponse::ECR_Ignore);

		WeaponToEquip->GetCollisionBox()->SetCollisionResponseToAllChannels(
		ECollisionResponse::ECR_Ignore);

		*/

		  
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(
			FName("RightHandSocket")
		);
		if (HandSocket) {
			//Attach the Weapon to the hand socket RightHandSocket
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		if (EquippedWeapon == nullptr)
		{
			// -1 == no EquippedWeapon yet. No need to reverse the icon animation
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else
		//if (!bSwapping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}

		// set equipped weapon to the newly spawned weapon
		EquippedWeapon = WeaponToEquip;


		//SOund to play when equip
		if (WeaponToEquip->GetEquipSound())
		UGameplayStatics::PlaySound2D(this, WeaponToEquip->GetEquipSound());



		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}


}

void AShooterCharacter::DropWeapon()
{
	if (EquippedWeapon)// check currently equip weapon
	{   
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
								// this detaches obj from mesh		
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}
 
void AShooterCharacter::SelectButtonPressed()
{
//FORMAT
if (CombatState != ECombatState::ECS_Unoccupied ) return;

	/*if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this, true);
		TraceHitItem = nullptr;
	}*/

	//DropWeapon();
	if (TraceHitItem) {
		auto TraceHitWeapon = Cast<AWeapons>(TraceHitItem);
		
		if (TraceHitWeapon) {
			/*if (TraceHitWeapon->GetPickupSound()) {
				UGameplayStatics::PlaySound2D(this, TraceHitItem->GetPickupSound());
			}*/
			if (Inventory.Num() < INVENTORY_CAPACITY) {
			TraceHitWeapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(TraceHitWeapon);
			TraceHitWeapon->SetItemState(EItemState::EIS_PickedUp);
			//FORMAT
		//	TraceHitItem = nullptr;
			}
			else{
			SwapWeapon(TraceHitWeapon);
		//	TraceHitItem = nullptr;
			}
		
			
		}
		auto Ammo = Cast<AAmmo>(TraceHitItem);

		if (Ammo) {
			if (Ammo->GetPickupSound()) {
			UGameplayStatics::PlaySound2D(this, TraceHitItem->GetPickupSound());
			}
			PickupAmmo(Ammo);
			TraceHitItem = nullptr;
		//	TraceHitItemLastFrame = nullptr;
		}


	//if (TraceHitItem->GetPickupSound()){
	//UGameplayStatics::PlaySound2D(this, TraceHitItem->GetPickupSound());
	//}
	//format
	//TraceHitItem = nullptr;
	}

}
void AShooterCharacter::PickupAmmo(AAmmo* Ammo)
{
	// check to see if AmmoMap contains Ammo's AmmoType
	if (AmmoMap.Find(Ammo->GetAmmoType()))
	{
		// Get amount of ammo in our AmmoMap for Ammo's type
		int32 AmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		AmmoCount += Ammo->GetItemCount();
		// Set the amount of ammo in the Map for this type
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}

	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		// Check to see if the gun is empty
		if (EquippedWeapon->GetAmmo() == 0)
		{
			ReloadWeapon();
		}
	}

	Ammo->Destroy();
}

void AShooterCharacter::SelectButtonReleased()
{


}

void AShooterCharacter::SwapWeapon(AWeapons* WeaponToSwap)
{

	if (Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}

	DropWeapon();
	EquipWeapon(WeaponToSwap);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;

}



void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr) return false;

	return EquippedWeapon->GetAmmo() > 0;
}
//
//void AShooterCharacter::PlayFireSound()
//{
//	// Play fire sound
//	if (EquippedWeapon->GetFireSound())
//	{
//		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
//	}
//}
//

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//handle interpolation for zoom while aiming
	CameraInterpZoom(DeltaTime);

	//change look sensitivity based on aiming
	SetLookRates();

	// Calculate crosshair spread multiplier
	CalculateCrosshairSpread(DeltaTime);

	//Check OverlappedItemCount, then trace for items
	TraceForItems();

	
}

void AShooterCharacter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex)
{
	/*const bool bCanExchangeItems =
		(CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		(CombatState == ECombatState::ECS_Unoccupied || CombatState == ECombatState::ECS_Equipping);*/

//	UE_LOG(LogTemp, Warning, TEXT("STate :%s"),CombatState);

	/*if (bCanExchangeItems)
	{*/
		/*if (bAiming)
		{
			StopAiming();
		}*/
		if( (CurrentItemIndex == NewItemIndex) || (NewItemIndex >= Inventory.Num())  ) return;
	//	if((CombatState != ECombatState::ECS_Unoccupied || CombatState != ECombatState::ECS_Equipping) ) return;

		auto OldEquippedWeapon = EquippedWeapon;
		auto NewWeapon = Cast<AWeapons>(Inventory[NewItemIndex]);
		EquipWeapon(NewWeapon);

		OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);

	//	CombatState = ECombatState::ECS_Equipping;
		//CombatState = ECombatState::ECS_Unoccupied;

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.0f);
			AnimInstance->Montage_JumpToSection(FName("EquipMontage"));
		}
		//NewWeapon->PlayEquipSound(true);
	}
//}

void AShooterCharacter::OneKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 0) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::TwoKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 1) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::ThreeKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 2) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}



float AShooterCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();

		auto EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController)
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(
				FName(TEXT("CharacterDead")),
				true
			);
		}
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AShooterCharacter::Die()
{
	bDead = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
		AnimInstance->Montage_JumpToSection(FName("DeathA"), DeathMontage);
	}
}

void AShooterCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		DisableInput(PC);
	}
	UGameplayStatics::OpenLevel(GetWorld(), FName("MainMenuMap"));
}

void AShooterCharacter::Stun()
{
	if (Health <= 0.f) return;

	CombatState = ECombatState::ECS_Stunned;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

void AShooterCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;
//
//	if (bAimingButtonPressed)
//	{
//		Aim();
//	}
}

void AShooterCharacter::Aim()
{
	bAiming = true;
	/*GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;*/
}

void AShooterCharacter::StopAiming()
{
	bAiming = false;
	//if (!bCrouching)
	//{
	//	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	//}
}


void AShooterCharacter::DanceEnd()
{
	
	CombatState = ECombatState::ECS_Unoccupied;

	
}


void AShooterCharacter::Dance()
{
	if (Health <= 0.f) return;

	CombatState = ECombatState::ECS_Dancing;


	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DanceMontage)
	{
		AnimInstance->Montage_Play(DanceMontage);
		//	AnimInstance->Montage_JumpToSection(FName("DanceMontage"),DanceMontage);
	}


}


// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);

	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);


	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);

	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpRate);



/*
 PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput );
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
*/
     

	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

//	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireWeapon);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);

	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);

	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	//PlayerInputComponent->BindAction("NewButton", IE_Released, this, &AShooterCharacter::ButtonPressed);


	PlayerInputComponent->BindAction("Select", IE_Pressed, this,&AShooterCharacter::SelectButtonPressed);
	
	PlayerInputComponent->BindAction("Select", IE_Released, this,&AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("1Key", IE_Pressed, this,&AShooterCharacter::OneKeyPressed);
	
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this,&AShooterCharacter::TwoKeyPressed);

	PlayerInputComponent->BindAction("3Key", IE_Pressed, this,&AShooterCharacter::ThreeKeyPressed);

	PlayerInputComponent->BindAction("Dance", IE_Pressed, this, &AShooterCharacter::Dance);

}

//float AShooterCharacter::GetCrosshairSpreadMultiplier() const
//{
//	//return CrosshairSpreadMultiplier;
//}

void AShooterCharacter::IncreamentOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;

	}

}


void AShooterCharacter::FinishReloading()
{
  
	//update the combat state
	CombatState = ECombatState::ECS_Unoccupied;


	// update the ammo map

	//if (bAimingButtonPressed)
	//{
	//Aim();
	//}

	if (EquippedWeapon == nullptr) return;
	const auto AmmoType{ EquippedWeapon->GetAmmoType() };

	// Update the AmmoMap
	if (AmmoMap.Contains(AmmoType))
	{
		// Amount of ammo the Character is carrying of the EquippedWeapon type
		int32 CarriedAmmo = AmmoMap[AmmoType];

		// Space left in the magazine of EquippedWeapon
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo)
		{
			// Reload the magazine with all the ammo we are carrying
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else
		{
			// fill the magazine
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}

}


float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
return CrosshairSpreadMultiplier;
}

