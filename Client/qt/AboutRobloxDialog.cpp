#include "AboutRobloxDialog.h"

#include "Roblox.h"

#include <G3D/System.h>
#include <GLG3D/GLCaps.h>
#include <string>

static const unsigned aboutTemplateId = 117;
static const unsigned aboutHtmlResourceId = 118;

CAboutRobloxDialog::CAboutRobloxDialog(QWidget* parent)
	: CRobloxDHtmlDialog(aboutTemplateId, aboutHtmlResourceId, parent)
{
}

void CAboutRobloxDialog::OnDocumentComplete()
{
	closeOnClick("ButtonOK", IDOK);

	setElementHtml("Version", QString::fromStdString(theApp.version()));

	std::string systemInfo;
	G3D::System::describeSystem(systemInfo);
	setElementHtml("systemInfo", QString::fromStdString(systemInfo));

	if (glGetString(GL_VENDOR) != nullptr) {
		setElementHtml("glVersion", QString::fromStdString(G3D::GLCaps::glVersion()));
		setElementHtml("driverVersion", QString::fromStdString(G3D::GLCaps::driverVersion()));
		setElementHtml("vendor", QString::fromStdString(G3D::GLCaps::vendor()));
		setElementHtml("renderer", QString::fromStdString(G3D::GLCaps::renderer()));
	}
	else {
		hideElement("glInfo");
	}
}
