#include "twoDRobotModel.h"

#include <QtGui/QColor>

#include <interpreterBase/robotModel/robotParts/button.h>
#include <interpreterBase/robotModel/robotParts/speaker.h>
#include <interpreterBase/robotModel/robotParts/motor.h>
#include <interpreterBase/robotModel/robotParts/encoderSensor.h>
#include <interpreterBase/robotModel/robotParts/touchSensor.h>
#include <interpreterBase/robotModel/robotParts/rangeSensor.h>
#include <interpreterBase/robotModel/robotParts/lightSensor.h>
#include <interpreterBase/robotModel/robotParts/colorSensorBlue.h>
#include <interpreterBase/robotModel/robotParts/colorSensorFull.h>
#include <interpreterBase/robotModel/robotParts/colorSensorGreen.h>
#include <interpreterBase/robotModel/robotParts/colorSensorPassive.h>
#include <interpreterBase/robotModel/robotParts/colorSensorRed.h>

#include <interpreterBase/robotModel/robotModelUtils.h>

#include "robotModel/twoD/parts/twoDDisplay.h"
#include "robotModel/twoD/parts/twoDSpeaker.h"
#include "robotModel/twoD/parts/twoDInfraredSensor.h"
#include "robotModel/twoD/parts/twoDLed.h"
#include "robotModel/parts/trikInfraredSensor.h"
#include "robotModel/parts/trikSonarSensor.h"

#include "trikDisplayWidget.h"

using namespace trikKitInterpreter::robotModel;
using namespace trikKitInterpreter::robotModel::twoD;
using namespace interpreterBase::robotModel;

TwoDRobotModel::TwoDRobotModel(RobotModelInterface &realModel)
	: twoDModel::robotModel::TwoDRobotModel(realModel)
{
}

robotParts::Device *TwoDRobotModel::createDevice(PortInfo const &port, DeviceInfo const &deviceInfo)
{
	if (deviceInfo.isA<robotParts::Display>()) {
		return new parts::Display(deviceInfo, port, *engine());
	}

	if (deviceInfo.isA<robotParts::Speaker>()) {
		return new parts::TwoDSpeaker(deviceInfo, port, *engine());
	}

	if (deviceInfo.isA<robotModel::parts::TrikInfraredSensor>()) {
		return new parts::TwoDInfraredSensor(deviceInfo, port, *engine());
	}

	if (deviceInfo.isA<robotModel::parts::TrikLed>()) {
		return new parts::TwoDLed(deviceInfo, port, *engine());
	}

	return twoDModel::robotModel::TwoDRobotModel::createDevice(port, deviceInfo);
}

void TwoDRobotModel::onInterpretationStarted()
{
	robotModel::parts::TrikDisplay * const display =
			RobotModelUtils::findDevice<robotModel::parts::TrikDisplay>(*this, "DisplayPort");
	if (display) {
		display->clearScreen();
		display->setBackground(QColor(Qt::gray));
	} else {
		/// @todo: if we get here it is wrong because display is not configured before the interpretation!
	}
}

QString TwoDRobotModel::robotImage() const
{
	return ":icons/trikTwoDRobot.svg";
}

PortInfo TwoDRobotModel::defaultLeftWheelPort() const
{
	return PortInfo(mLeftWheelPort, output);
}

PortInfo TwoDRobotModel::defaultRightWheelPort() const
{
	return PortInfo(mRightWheelPort, output);
}

twoDModel::engine::TwoDModelDisplayWidget *TwoDRobotModel::displayWidget(QWidget * parent) const
{
	return new TrikDisplayWidget(parent);
}

QString TwoDRobotModel::sensorImagePath(const DeviceInfo &deviceType) const
{
	if (deviceType.isA<interpreterBase::robotModel::robotParts::LightSensor>()) {
		return ":icons/twoDColorEmpty.svg";
	}

	return "";
}

void TwoDRobotModel::setWheelPorts(QString const &leftWheelPort, QString const &rightWheelPort)
{
	mLeftWheelPort = leftWheelPort;
	mRightWheelPort = rightWheelPort;
}
