// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "FirstSaveGame.h"
#include "ItemStorage.h"


// Sets default values
AMain::AMain()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//����Camera Boom ��������ײʱ��������ң�
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	//���õ��ɱ� - ���������������һ����
	CameraBoom->TargetArmLength = 600.f;
	//��ת���ڿ����� - ����������Pawn��
	CameraBoom->bUsePawnControlRotation = true;

	//Ϊ��ײ�������趨��С
	GetCapsuleComponent()->SetCapsuleSize(25.f, 74.f);

	//����Follow�����
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	//����Attach��Attach�������õ�RootComponent�������Ҫ���ӵ�CameraBoom
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	//����������ӵ�Boomĩ�ˣ�����Pawn���е�������ʵ��ƥ��������ķ���
	//ֻ����������ӵ�CameraBoom��������CameraBoom�ƶ����������ڿ�����
	FollowCamera->bUsePawnControlRotation = false;

	//Ϊ��������ת������
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	//��������ת��ʱ��Ҫ��ת
	//��Ӱ�������
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	//���������ɫ�ƶ�
	GetCharacterMovement()->bOrientRotationToMovement = true;//��ɫ���Ż�������ķ����ƶ�
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f);//���ڴ���ת����
	GetCharacterMovement()->JumpZVelocity = 450.f;
	GetCharacterMovement()->AirControl = 0.2f;

	//����Player StatsĬ����ֵ
	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 350.f;
	Stamina = 120.f;
	Coins = 0;

	//��ʼ��Running��SpringSpeed����ֵ
	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;

	//bShiftKeyDown
	bShiftKeyDown = false;

	//bLMBDown
	bLMBDown = false;



	bFDown = false;



	//bESCDown
	bESCDown = false;

	//Stamina - ��ʼ��Enum
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	//��ʼ�����������ʺ���С�������ֵ
	StaminaDrainRate = 25.f;
	MinSprintStamina = 75.f;

	//����InterSpeed��InterToEnemyĬ����ֵ
	InterpSpeed = 15.f;
	bInterpToEnemy = false;

	//bHasCombatTarget
	bHasCombatTarget = false;

	bMovingForward = false;
	bMovingRight = false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();
	
	//����MainplayerController
	MainPlayerController = Cast<AMainPlayerController>(GetController());

	FString Map = GetWorld()->GetMapName();
	Map.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	if (Map != "Farm")
	{
		//����LoadGameNoSwitch
		LoadGameNoSwitch();
		if (MainPlayerController)
		{
			MainPlayerController->GameModeOnly();
		}
	}
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	//ʹ�ñ�����ʵ����������
	float DeltaStamina = StaminaDrainRate * DeltaTime;

	//ʵ��StaminaStatus������״̬
	switch (StaminaStatus)
	{
		//ʵ��Normal״̬���
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown)
		{
			if (Stamina - DeltaStamina <= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
				Stamina -= DeltaStamina;
			}
			else
			{
				Stamina -= DeltaStamina;
			}
			if (bMovingForward || bMovingRight)
			{
				SetMovementStatus(EMovementStatus::EMS_Sprinting);//��ƣ��״̬�ָ���
			}
			else
			{
				SetMovementStatus(EMovementStatus::EMS_Normal);//������״̬��
			}
		}
		//ShiftKeyUp - �ж������Ƿ�ָ�
		else
		{
			if (Stamina + DeltaStamina >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;

		//ʵ��BelowMinimum״̬���
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown)
		{
			if (Stamina - DeltaStamina <= 0.f)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
				Stamina = 0;
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else
			{
				Stamina -= DeltaStamina;
				if (bMovingForward || bMovingRight)
				{
					SetMovementStatus(EMovementStatus::EMS_Sprinting);//��ƣ��״̬�ָ���
				}
				else
				{
					SetMovementStatus(EMovementStatus::EMS_Normal);//������״̬��
				}
			}
		}
		//shift key up
		else
		{
			if (Stamina + DeltaStamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;

		//ʵ��Exhausted״̬���
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftKeyDown)
		{
			Stamina = 0.f;
		}
		//shift key up
		else
		{
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
			Stamina += DeltaStamina;
		}
		//��Exhausted״̬����sprinting
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;

		//ʵ��ExhaustedRecovering״̬���
	case EStaminaStatus::ESS_ExhaustedRecovering:
		if (Stamina + DeltaStamina >= MinSprintStamina)
		{
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			Stamina += DeltaStamina;
		}
		else
		{
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	default:
		;
	}

	//����ֵ�ͼ���Ƿ��ڶ�ս
	if (bInterpToEnemy && CombatTarget)
	{
		//����Ŀ��
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		//ʹ�ò�ֵ - ��Ϊ����֡ -ƽ�ȹ����ӽ�
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterpRotation);
	}

	//���¶�սĿ���λ��
	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		//���ö�սĿ��λ�ø��¿�����
		if (MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}

//GetLookAtRotationYaw����
FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	//��麯����Ч��
	check(PlayerInputComponent);

	//����ӳ�����Ծ�� - һ���Ե�  - �ؼ��� -IE_Pressed - IE_Released - ����/�ɿ���
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//����ӳ��ĳ�̰� - һ���Ե�  - �ؼ��� - IE_Pressed - IE_Released - ����/�ɿ���
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	//����ӳ���ESC�� - һ���Ե�  - �ؼ��� - IE_Pressed - IE_Released - ����/�ɿ���
	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMain::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMain::ESCUp);



	PlayerInputComponent->BindAction("F", IE_Pressed, this, &AMain::FDown);
	PlayerInputComponent->BindAction("F", IE_Released, this, &AMain::FUp);



	//����ӳ��ĳ�̰� - һ���Ե�  - �ؼ��� - IE_Pressed - IE_Released - ����/�ɿ���
	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBUp);

	//Ϊ��������
	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	//Ϊ�������
	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::Lookup);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);

}

//ʵ�ֲ˵�����ֹͣ��ɫ������������˶�
bool AMain::CanMove(float Value)
{
	if (MainPlayerController)
	{
		return (Value != 0.0f) && 
			(!bAttacking) &&
			(MovementStatus != EMovementStatus::EMS_Dead) &&
			!MainPlayerController->bPauseMenuVisible;
	}
	return false;
}

//��ҪYaw Rotation������
void AMain::Turn(float Value)
{
	if (CanMove(Value))
	{
		AddControllerYawInput(Value);
	}
}

//��ҪPitch Rotation������
void AMain::Lookup(float Value)
{
	if (CanMove(Value))
	{
		AddControllerPitchInput(Value);
	}
}


void AMain::MoveForward(float Value)
{
	bMovingForward = false;
	if (CanMove(Value))
	{
		//�ҳ�ǰ��
		const FRotator Rotation = Controller->GetControlRotation();//����������ת
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		//������ǰʸ�� - RotationMatrix��������ȡ�ض������ʸ��������actor��������������겻ͳһʱ������������ǰʸ��
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		bMovingForward = true;
	}
}

void AMain::MoveRight(float Value)
{
	bMovingRight = false;
	if (CanMove(Value))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);

		bMovingRight = true;
	}
}

void AMain::TurnAtRate(float Rate)
{

	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}


void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


//����LMBDown/Up
void AMain::LMBDown()
{
	bLMBDown = true;
	//����������LMB����
	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	//���ӿ������ж� - ��ʾ�˵����ܴ���������
	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;


	//���ù���
	else if (EquippedWeapon)
	{
		Attack();
	}
}

void AMain::LMBUp()
{
	bLMBDown = false;

}





void AMain::FDown()
{
	bFDown = true;
	if (ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon)
		{
			Weapon->Equip(this);
			//��SetActiveOverlappingOverItem��Ϊnullptr
			SetActiveOverlappingItem(nullptr);
		}
	}
}
void AMain::FUp()
{
	bFDown = false;
}












//ʵ��ESC
void AMain::ESCDown()
{
	bESCDown = true;

	//���ò˵�����ʾ����
	if (MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}

}
void AMain::ESCUp()
{
	bESCDown = false;
}



//����:DecrementHealth
void AMain::DecrementHealth(float Amount)
{
	
}

//����IncrementCoins
void AMain::IncrementCoins(int32 Amount)
{
	Coins += Amount;
}

//��������Ѫ�����ƺ���
void AMain::IncrementHealth(float Amount)
{
	if (Health + Amount >= MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += Amount;
	}
}


//����Die
void AMain::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

//��ɫ�������ڵ���Jump
void AMain::Jump()
{
	//���ӿ������ж� - ��ʾ�˵����ܴ�����Ծ��
	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;

	if (MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();
	}
}


//����DeathEnd -������ɫ����������ƶ�
void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

//����Setter - MovementStatus
void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;
	//�жϽ�����
	if (MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

//Shift���º�������
void AMain::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

//Shift�ɿ���������
void AMain::ShiftKeyUp()
{
	bShiftKeyDown = false;
}

//ʵ��ShowPickupLocations����
void AMain::ShowPickupLocations()
{
	//��������ռ���Ӳ��ÿһ����λ��
	for (auto Location : PickupLocations)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 8, FLinearColor::Green, 10.f, .5f);
	}
}


//����һ������װ��������Setter
void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
	EquippedWeapon = WeaponToSet;
}


//����Attack����
void AMain::Attack()
{
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead)
	{
		bAttacking = true;
		SetInterpToEnemy(true);
		//���ʵ�ʱ����в�ֵ
		SetInterpToEnemy(true);

		bAttacking = true;
		//��ȡAnimʵ��
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0, 1);
			switch (Section)
			{
			case 0:
				//Ӧ��Montage Play������������̫�� -�����ٶ�Ϊ2.2����
				AnimInstance->Montage_Play(CombatMontage, 2.2f);
				//�ڲ�����̫��ʱ����ζ����ʲ��ض�����ĳһƬ��
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;
			case 1:
				//Ӧ��Montage Play������������̫�� -�����ٶ�Ϊ2.2����
				AnimInstance->Montage_Play(CombatMontage, 1.8f);
				//�ڲ�����̫��ʱ����ζ����ʲ��ض�����ĳһƬ��
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;
			default:
				;
			}
		}
	}
}

//����AttackEnd����
void AMain::AttackEnd()
{
	bAttacking = false;
	SetInterpToEnemy(false);
	//��סLMB���Գ�������
	if (bLMBDown)
	{
		Attack();
	}
}

//PlaySwingSound
void AMain::PlaySwingSound()
{
	if (EquippedWeapon->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}



//SetInterpToEnemy����
void AMain::SetInterpToEnemy(bool Interp)
{
	bInterpToEnemy = Interp;
}


//����TakeDamge�˺�����
float AMain::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	//�жϽ�ɫ�Ƿ�Ӧ��������Ѫ������
	if (Health - DamageAmount <= 0.f)
	{
		Health -= DamageAmount;
		Die();
		//ת��Ϊ����
		if (DamageCauser)
		{
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if (Enemy)
			{
				//��ɫ����enemy�Ĳ���Ϊfalse
				Enemy->bHasValidTarget = false;
			}
		}
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

//���� UpdateCombatTarget
void  AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);

	//��ȡEnemy
	if (OverlappingActors.Num() == 0)
	{
		if (MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}
	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);
	if (ClosestEnemy)
	{
		FVector Location = GetActorLocation();
		float MinDistance = (ClosestEnemy->GetActorLocation() - Location).Size();

		for (auto Actor : OverlappingActors)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy)
			{
				float DistanceToActor = (Enemy->GetActorLocation() - Location).Size();
				if (DistanceToActor < MinDistance)
				{
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}
			}
		}
		if (MainPlayerController)
		{
			MainPlayerController->DisplayEnemyHealthBar();
		}
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget = true;
	}
}

//�������ɹؿ�����
void  AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();

	if (World)
	{
		FString CurrentLevel = World->GetMapName();

		FName CurrentLevelName(*CurrentLevel);
		if (CurrentLevelName != LevelName)
		{
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}



//�����洢��������Ϸ����
void AMain::SaveGame()
{
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));
	//���ñ���
	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;

	//����ؿ���
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	SaveGameInstance->CharacterStats.LevelName = MapName;

	//�ж��Ƿ�װ������
	if (EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);

}

void AMain::LoadGame(bool SetPosition)
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	//ΪWeaponStroage��U�ࣩ - ����һ��ʵ�� - ȥ����һ��Actor
	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;

			if (Weapons->WeaponMap.Contains(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}
		}
	}

	if (SetPosition)
	{
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}

	//�� -������Ϸ�еĽ�ɫ״̬����
	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	//�жϹؿ���
	if (LoadGameInstance->CharacterStats.LevelName != TEXT(""))
	{
		FName LevelName = (*LoadGameInstance->CharacterStats.LevelName);

		SwitchLevel(LevelName);
	}
}


//����LoadGameNoSwitch - ���ڹؿ�����л�
void AMain::LoadGameNoSwitch()
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));
	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	//ΪWeaponStroage��U�ࣩ - ����һ��ʵ�� - ȥ����һ��Actor
	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;

			if (Weapons->WeaponMap.Contains(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}
		}
	}
	//�� -������Ϸ�еĽ�ɫ״̬����
	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

}