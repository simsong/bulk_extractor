#include "lightgrep_controller.h"

LightgrepController::LightgrepController() {}

void LightgrepController::setup() {
	IsSetup = true;
}

void LightgrepController::teardown() {
	IsSetup = false;
}

bool LightgrepController::is_setup() const {
	return IsSetup;
}
