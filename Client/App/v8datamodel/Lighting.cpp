#include "v8datamodel/Lighting.h"

#include "util/RunStateOwner.h"
#include "util/Utilities.h"
#include "v8datamodel/Sky.h"

#include <G3D/format.h>
#include <vector>

namespace RBX {

char sLighting[] = "Lighting";

Reflection::SignalDesc<Lighting, void(bool)> Lighting::event_LightingChanged("LightingChanged", "skyboxChanged");

} // namespace RBX

static RBX::Reflection::PropDescriptor<RBX::Lighting, G3D::Color3> desc_AmbientTop(
	"TopAmbientV9", "Appearance", &RBX::Lighting::getAmbientTop, &RBX::Lighting::setAmbientTop
);

static RBX::Reflection::PropDescriptor<RBX::Lighting, G3D::Color3> desc_AmbientBottom(
	"BottomAmbientV9", "Appearance", &RBX::Lighting::getAmbientBottom, &RBX::Lighting::setAmbientBottom
);

static RBX::Reflection::PropDescriptor<RBX::Lighting, G3D::Color3> desc_LightColor(
	"SpotLightV9", "Appearance", &RBX::Lighting::getLightColor, &RBX::Lighting::setLightColor
);

static RBX::Reflection::PropDescriptor<RBX::Lighting, G3D::Color3> desc_ClearColor(
	"ClearColor", "Appearance", &RBX::Lighting::getClearColor3, &RBX::Lighting::setClearColor3
);

static RBX::Reflection::PropDescriptor<RBX::Lighting, std::string> prop_Time(
	"TimeOfDay", "Data", &RBX::Lighting::getTimeStr, &RBX::Lighting::setTimeStr
);

static RBX::Reflection::PropDescriptor<RBX::Lighting, float> prop_GeographicLatitude(
	"GeographicLatitude", "Data", &RBX::Lighting::getGeographicLatitude, &RBX::Lighting::setGeographicLatitude
);

static RBX::Reflection::BoundFuncDesc<RBX::Lighting, float(), 0> prop_GetMoonPhase(
	&RBX::Lighting::getMoonPhase, "GetMoonPhase", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<RBX::Lighting, G3D::Vector3(), 0> prop_GetMoonPosition(
	&RBX::Lighting::getMoonPosition, "GetMoonDirection", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<RBX::Lighting, G3D::Vector3(), 0> prop_GetSunPosition(
	&RBX::Lighting::getSunPosition, "GetSunDirection", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<RBX::Lighting, double(), 0> func_GetMinutesAfterMidnight(
	&RBX::Lighting::getMinutesAfterMidnight, "GetMinutesAfterMidnight", RBX::Reflection::FunctionDescriptor::AnyCaller
);

static RBX::Reflection::BoundFuncDesc<RBX::Lighting, void(double), 1> func_SetMinutesAfterMidnight(
	&RBX::Lighting::setMinutesAfterMidnight,
	"SetMinutesAfterMidnight",
	"minutes",
	RBX::Reflection::FunctionDescriptor::AnyCaller
);

namespace RBX {

void Lighting::fireLightingChanged(bool skyChanged)
{
	event_LightingChanged.fire(this, skyChanged);

	if (RunService* runService = ServiceProvider::find<RunService>(this)) {
		runService->invalidateRunViews();
	}
}

G3D::Vector3 Lighting::getMoonPosition()
{
	return skyParameters.physicallyCorrect ? skyParameters.trueMoonPosition : skyParameters.moonPosition;
}

G3D::Vector3 Lighting::getSunPosition()
{
	return skyParameters.physicallyCorrect ? skyParameters.trueSunPosition : skyParameters.sunPosition;
}

bool Lighting::askAddChild(const Instance* instance) const
{
	return dynamic_cast<const Sky*>(instance) != nullptr;
}

void Lighting::replaceSky(Sky* newSky)
{
	while (Sky* oldSky = findFirstChildOfType<Sky>()) {
		oldSky->setParent(nullptr);
	}

	newSky->setParent(this);
}

double Lighting::getMinutesAfterMidnight()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(timeOfDay).count() / 60000.0;
}

std::string Lighting::getTimeStr() const
{
	const long long total = std::chrono::abs(timeOfDay).count();

	return G3D::format(
		"%s%02d:%02d:%02d",
		timeOfDay.count() < 0 ? "-" : "",
		static_cast<int>(total / 3600),
		static_cast<int>(total / 60 % 60),
		static_cast<int>(total % 60)
	);
}

Lighting::Lighting()
	: ambientTop(209 / 255.0f, 208 / 255.0f, 217 / 255.0f), ambientBottom(122 / 255.0f, 134 / 255.0f, 120 / 255.0f),
	  hasSky(true), clearColor(G3D::Color3::white()), timeOfDay(std::chrono::hours(14))
{
	skyParameters.lightColor = G3D::Color3(152 / 255.0f, 137 / 255.0f, 102 / 255.0f);
	skyParameters.setTime(static_cast<int>(timeOfDay.count()));
	setName("Lighting");
}

void Lighting::setClearColor(G3D::Color4 value)
{
	if (value != clearColor) {
		clearColor = value;
		raisePropertyChanged(desc_ClearColor);
		fireLightingChanged(false);
	}
}

void Lighting::setLightColor(G3D::Color3 value)
{
	if (value != skyParameters.lightColor) {
		skyParameters.lightColor = value;
		raisePropertyChanged(desc_LightColor);
		fireLightingChanged(false);
	}
}

void Lighting::setAmbientBottom(G3D::Color3 value)
{
	if (value != ambientBottom) {
		ambientBottom = value;
		raisePropertyChanged(desc_AmbientBottom);
		fireLightingChanged(false);
	}
}

void Lighting::onChildRemoving(Instance* child)
{
	if (sky.get() == child) {
		sky.reset();
		fireLightingChanged(true);
	}
}

void Lighting::onChildAdded(Instance* child)
{
	if (Sky* s = dynamic_cast<Sky*>(child)) {
		sky = shared_from(s);
		fireLightingChanged(true);
	}
}

void Lighting::onChildChanged(Instance* instance, const PropertyChanged& event)
{
	Instance::onChildChanged(instance, event);
	if (sky.get() == instance) {
		fireLightingChanged(true);
	}
}

void Lighting::setTime(std::chrono::seconds value)
{
	int seconds = static_cast<int>(value.count());
	seconds = seconds % 86400;

	if (timeOfDay.count() != seconds) {
		timeOfDay = std::chrono::seconds(seconds);

		skyParameters.setTime(seconds);
		raisePropertyChanged(prop_Time);
		fireLightingChanged(false);
	}
}

void Lighting::setAmbientTop(G3D::Color3 value)
{
	if (value != ambientTop) {
		ambientTop = value;
		raisePropertyChanged(desc_AmbientTop);
		fireLightingChanged(false);
	}
}

void Lighting::setGeographicLatitude(float value)
{
	if (value != skyParameters.geoLatitude) {
		skyParameters.geoLatitude = value;
		raisePropertyChanged(prop_GeographicLatitude);
		fireLightingChanged(false);
	}
}

void Lighting::setMinutesAfterMidnight(double value)
{
	setTime(std::chrono::seconds(static_cast<long>(value * 60.0)));
}

void Lighting::setTimeStr(const std::string& value)
{
	const bool negative = value.at(0) == '-';
	const std::vector<std::string> tokens = tokenize(value, "-:,.");

	int hours = 0;
	unsigned short minutes = 0;
	unsigned short seconds = 0;
	long long fraction = 0;

	for (std::vector<std::string>::size_type i = 0; i < tokens.size() && i < 4; ++i) {
		switch (i) {
		case 0:
			hours = lexicalCast<int>(tokens[i]);
			break;
		case 1:
			minutes = lexicalCast<unsigned short>(tokens[i]);
			break;
		case 2:
			seconds = lexicalCast<unsigned short>(tokens[i]);
			break;
		case 3: {
			const std::string::size_type precision = 6;

			fraction = lexicalCast<long long>(tokens[i].substr(0, precision));

			for (std::string::size_type digits = tokens[i].size(); digits < precision; ++digits) {
				fraction *= 10;
			}
		} break;
		}
	}

	long long microseconds = (static_cast<long long>(hours) * 3600 + minutes * 60 + seconds) * 1000000 + fraction;

	if (negative) {
		microseconds = -microseconds;
	}

	setTime(std::chrono::seconds(microseconds / 1000000));
}

} // namespace RBX

template class RBX::FactoryProduct<RBX::Lighting, RBX::Instance, RBX::sLighting>;
template class RBX::Reflection::
	Described<RBX::Lighting, RBX::sLighting, RBX::FactoryProduct<RBX::Lighting, RBX::Instance, RBX::sLighting>>;
template class RBX::DescribedCreatable<RBX::Lighting, RBX::Instance, RBX::sLighting>;
template const RBX::FactoryProduct<RBX::Sky, RBX::Instance, RBX::sSky>::Creator
	RBX::FactoryProduct<RBX::Sky, RBX::Instance, RBX::sSky>::creator;
