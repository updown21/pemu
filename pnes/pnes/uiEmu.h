//
// Created by cpasjuste on 01/06/18.
//

#ifndef PNES_UIEMU_H
#define PNES_UIEMU_H

#include <string>

class PNESGuiEmu : public c2dui::UIEmu {

public:

    explicit PNESGuiEmu(c2dui::UIMain *ui);

    bool onInput(c2d::Input::Player *players) override;

    void onDraw(c2d::Transform &transform, bool draw = true) override;

    int load(c2dui::RomList::Rom *rom) override;

    void stop() override;

    void nestopia_config_init();

    int nestopia_core_init(const char *rom_path);

private:

};

#endif //PNES_UIEMU_H
