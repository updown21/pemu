//
// Created by cpasjuste on 01/06/18.
//

#ifndef PSNES_UIMENU_H
#define PSNES_UIMENU_H

class PSNESUIMenu : public c2dui::UIMenu {

public:

    PSNESUIMenu(c2dui::UIMain *ui);

    bool isOptionHidden(c2dui::Option *option);

};

#endif //PSNES_UIMENU_H
