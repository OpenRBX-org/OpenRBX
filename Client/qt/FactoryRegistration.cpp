#include "network/Player.h"
#include "script/Script.h"
#include "script/ScriptContext.h"
#include "util/RunStateOwner.h"
#include "util/Sound.h"
#include "v8datamodel/Accoutrement.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/Team.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/Visit.h"
#include "v8datamodel/custommesh.h"
#include "v8tree/Service.h"

template const RBX::FactoryProduct<RBX::ModelInstance, RBX::PVInstance, RBX::sModel>::Creator
	RBX::FactoryProduct<RBX::ModelInstance, RBX::PVInstance, RBX::sModel>::creator;

template const RBX::FactoryProduct<RBX::Selection, RBX::Instance, RBX::sSelection>::Creator
	RBX::FactoryProduct<RBX::Selection, RBX::Instance, RBX::sSelection>::creator;

template const RBX::FactoryProduct<RBX::RunService, RBX::Instance, RBX::sRunService>::Creator
	RBX::FactoryProduct<RBX::RunService, RBX::Instance, RBX::sRunService>::creator;

template const RBX::FactoryProduct<RBX::ScriptContext, RBX::Instance, RBX::sScriptContext>::Creator
	RBX::FactoryProduct<RBX::ScriptContext, RBX::Instance, RBX::sScriptContext>::creator;

template const RBX::FactoryProduct<RBX::Script, RBX::Instance, RBX::sScript>::Creator
	RBX::FactoryProduct<RBX::Script, RBX::Instance, RBX::sScript>::creator;

template const RBX::FactoryProduct<RBX::LocalScript, RBX::Script, RBX::sLocalScript>::Creator
	RBX::FactoryProduct<RBX::LocalScript, RBX::Script, RBX::sLocalScript>::creator;

template const RBX::FactoryProduct<RBX::DebugSettings, RBX::GlobalSettings::Item, RBX::sDebugSettings>::Creator
	RBX::FactoryProduct<RBX::DebugSettings, RBX::GlobalSettings::Item, RBX::sDebugSettings>::creator;

template const RBX::FactoryProduct<RBX::GameSettings, RBX::GlobalSettings::Item, RBX::sGameSettings>::Creator
	RBX::FactoryProduct<RBX::GameSettings, RBX::GlobalSettings::Item, RBX::sGameSettings>::creator;

template const RBX::FactoryProduct<RBX::Team, RBX::Instance, RBX::sTeam>::Creator
	RBX::FactoryProduct<RBX::Team, RBX::Instance, RBX::sTeam>::creator;

template const RBX::FactoryProduct<RBX::Network::Player, RBX::Instance, RBX::Network::sPlayer>::Creator
	RBX::FactoryProduct<RBX::Network::Player, RBX::Instance, RBX::Network::sPlayer>::creator;

template const RBX::FactoryProduct<RBX::Decal, RBX::FaceInstance, RBX::sDecal>::Creator
	RBX::FactoryProduct<RBX::Decal, RBX::FaceInstance, RBX::sDecal>::creator;

template const RBX::FactoryProduct<RBX::Texture, RBX::Decal, RBX::sTexture>::Creator
	RBX::FactoryProduct<RBX::Texture, RBX::Decal, RBX::sTexture>::creator;

template const RBX::FactoryProduct<RBX::PartInstance, RBX::PVInstance, RBX::sPart>::Creator
	RBX::FactoryProduct<RBX::PartInstance, RBX::PVInstance, RBX::sPart>::creator;

template const RBX::FactoryProduct<RBX::SpecialShape, RBX::Instance, RBX::sSpecialShape>::Creator
	RBX::FactoryProduct<RBX::SpecialShape, RBX::Instance, RBX::sSpecialShape>::creator;

template const RBX::FactoryProduct<RBX::Accoutrement, RBX::Instance, RBX::sAccoutrement>::Creator
	RBX::FactoryProduct<RBX::Accoutrement, RBX::Instance, RBX::sAccoutrement>::creator;

template const RBX::FactoryProduct<RBX::Hat, RBX::Accoutrement, RBX::sHat>::Creator
	RBX::FactoryProduct<RBX::Hat, RBX::Accoutrement, RBX::sHat>::creator;

template const RBX::FactoryProduct<RBX::Soundscape::SoundChannel, RBX::Instance, RBX::Soundscape::sSoundChannel>::
	Creator RBX::FactoryProduct<RBX::Soundscape::SoundChannel, RBX::Instance, RBX::Soundscape::sSoundChannel>::creator;

template const RBX::FactoryProduct<RBX::Soundscape::SoundService, RBX::Instance, RBX::Soundscape::sSoundService>::
	Creator RBX::FactoryProduct<RBX::Soundscape::SoundService, RBX::Instance, RBX::Soundscape::sSoundService>::creator;

template const RBX::FactoryProduct<RBX::Camera, RBX::Instance, RBX::sCamera>::Creator
	RBX::FactoryProduct<RBX::Camera, RBX::Instance, RBX::sCamera>::creator;

template const RBX::FactoryProduct<RBX::ControllerService, RBX::Instance, RBX::sControllerService>::Creator
	RBX::FactoryProduct<RBX::ControllerService, RBX::Instance, RBX::sControllerService>::creator;

template const RBX::FactoryProduct<RBX::Visit, RBX::Instance, RBX::sVisit>::Creator
	RBX::FactoryProduct<RBX::Visit, RBX::Instance, RBX::sVisit>::creator;

template RBX::Soundscape::SoundService* RBX::ServiceProvider::create<RBX::Soundscape::SoundService>() const;
