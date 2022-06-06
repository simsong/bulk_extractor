#ifndef LIGHTGREP_CONTROLLER_H
#define LIGHTGREP_CONTROLLER_H

class LightgrepController {
public:
    LightgrepController();

    void setup();
    void teardown();

    bool is_setup() const;

private:
	bool IsSetup;
};

#endif
