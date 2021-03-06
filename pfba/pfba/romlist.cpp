//
// Created by cpasjuste on 29/05/18.
//

#include "burner.h"

#include "c2dui.h"
#include "romlist.h"

using namespace c2d;
using namespace c2dui;

static std::string str_toupper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); }
    );
    return s;
}

PFBARomList::PFBARomList(UIMain *ui, const std::string &emuVersion) : RomList(ui, emuVersion) {

    printf("PFBARomList::PFBARomList()\n");
}

void PFBARomList::build() {

    printf("PFBARomList::build()\n");

    char path[MAX_PATH];
    const char *pathUppercase; // sometimes on FAT32 short files appear as all uppercase
    bool use_icons = ui->getConfig()->get(Option::Id::GUI_SHOW_ICONS)->getIndex() == 1;

    for (unsigned int i = 0; i < nBurnDrvCount; i++) {

        nBurnDrvActive = i;

        auto *rom = new Rom();
        rom->drv = i;
        rom->drv_name = BurnDrvGetTextA(DRV_NAME);
        rom->path = BurnDrvGetTextA(DRV_NAME);
        rom->parent = BurnDrvGetTextA(DRV_PARENT);
        rom->name = BurnDrvGetTextA(DRV_FULLNAME);
        rom->year = BurnDrvGetTextA(DRV_DATE);
        rom->manufacturer = BurnDrvGetTextA(DRV_MANUFACTURER);
        rom->system = BurnDrvGetTextA(DRV_SYSTEM);
        rom->genre = BurnDrvGetGenreFlags();
        rom->flags = (unsigned int) BurnDrvGetFlags();
        rom->hardware = (unsigned int) BurnDrvGetHardwareCode();
        rom->state = RomState::MISSING;

        // load icon if needed, only for parent roms
        if (use_icons && !rom->parent) {
            snprintf(icon_path, 1023, "%sicons/%s.png", ui->getConfig()->getHomePath()->c_str(), rom->drv_name);
            if (ui->getIo()->exist(icon_path)) {
                rom->icon = new C2DTexture(icon_path);
                rom->icon->setDeleteMode(DeleteMode::Manual);
                if (!rom->icon->available) {
                    delete (rom->icon);
                    rom->icon = nullptr;
                }
            }
        }

        // add rom to "ALL" game list
        hardwareList->at(0).supported_count++;
        if (rom->parent) {
            hardwareList->at(0).clone_count++;
        }

        // add rom to specific hardware
        Hardware *hardware = getHardware(rom->hardware);
        if (hardware) {
            hardware->supported_count++;
            if (rom->parent) {
                hardware->clone_count++;
            }
        }

        // get real rom name
        char *z_name = nullptr;
        BurnDrvGetZipName(&z_name, 0);
        if (z_name) {
            snprintf(path, MAX_PATH - 1, "%s.zip", z_name);
        } else {
            snprintf(path, MAX_PATH - 1, "%s.zip", rom->drv_name);
        }
        pathUppercase = str_toupper(path).c_str();

        for (unsigned int j = 0; j < files.size(); j++) {

            if (files.at(j).empty()) {
                continue;
            }

            std::vector<std::string> fileList;
            for(auto &f : files.at(j)) {
                fileList.emplace_back(f.name);
            }

            auto file = std::find(fileList.begin(), fileList.end(), path);
            if (file == fileList.end()) {
                file = std::find(fileList.begin(), fileList.end(), pathUppercase);
            }

            if (file != fileList.end()) {

                int prefix = (((rom->hardware | HARDWARE_PREFIX_CARTRIDGE) ^ HARDWARE_PREFIX_CARTRIDGE) & 0xff000000);

                switch (prefix) {
                    case HARDWARE_PREFIX_COLECO:
                        if (!Utility::endsWith(paths->at(j), "coleco/")) {
                            continue;
                        }
                        break;
                    case HARDWARE_PREFIX_SEGA_GAME_GEAR:
                        if (!Utility::endsWith(paths->at(j), "gamegear/")) {
                            continue;
                        }
                        break;
                    case HARDWARE_PREFIX_SEGA_MEGADRIVE:
                        if (!Utility::endsWith(paths->at(j), "megadriv/")) {
                            continue;
                        }
                        break;
                    case HARDWARE_PREFIX_MSX:
                        if (!Utility::endsWith(paths->at(j), "msx/")) {
                            continue;
                        }
                        break;
                    case HARDWARE_PREFIX_SEGA_SG1000:
                        if (!Utility::endsWith(paths->at(j), "sg1000/")) {
                            continue;
                        }
                        break;
                    case HARDWARE_PREFIX_SEGA_MASTER_SYSTEM:
                        if (!Utility::endsWith(paths->at(j), "sms/")) {
                            continue;
                        }
                        break;
                    case HARDWARE_PREFIX_PCENGINE:
                        switch (rom->hardware) {
                            case HARDWARE_PCENGINE_PCENGINE:
                                if (!Utility::endsWith(paths->at(j), "pce/")) {
                                    continue;
                                }
                                break;
                            case HARDWARE_PCENGINE_TG16:
                                if (!Utility::endsWith(paths->at(j), "tg16/")) {
                                    continue;
                                }
                                break;
                            case HARDWARE_PCENGINE_SGX:
                                if (!Utility::endsWith(paths->at(j), "sgx/")) {
                                    continue;
                                }
                                break;
                            default:
                                continue;
                        }
                        break;
                    default:
                        if (Utility::endsWith(paths->at(j), "coleco/")
                            || Utility::endsWith(paths->at(j), "gamegear/")
                            || Utility::endsWith(paths->at(j), "megadriv/")
                            || Utility::endsWith(paths->at(j), "msx/")
                            || Utility::endsWith(paths->at(j), "sg1000/")
                            || Utility::endsWith(paths->at(j), "sms/")
                            || Utility::endsWith(paths->at(j), "pce/")
                            || Utility::endsWith(paths->at(j), "sgx/")
                            || Utility::endsWith(paths->at(j), "tg16/")) {
                            continue;
                        }
                        break;
                }

                rom->path = paths->at(j) + file->c_str();
                rom->state = BurnDrvIsWorking() ? RomState::WORKING : RomState::NOT_WORKING;
                hardwareList->at(0).available_count++;

                if (rom->parent) {
                    hardwareList->at(0).available_clone_count++;
                }

                if (hardware) {
                    hardware->available_count++;
                    if (rom->parent) {
                        hardware->available_clone_count++;
                    }
                }
                break;
            }
        }

        // set "Io::File"" color for ui
        rom->color = COL_RED;
        if (rom->state == RomList::RomState::NOT_WORKING) {
            rom->color = COL_ORANGE;
        } else if (rom->state == RomList::RomState::WORKING) {
            rom->color = rom->parent == nullptr ? COL_GREEN : COL_YELLOW;
        }

        if (rom->state == RomState::MISSING) {
            hardwareList->at(0).missing_count++;
            if (hardware) {
                hardware->missing_count++;
            }
            if (rom->parent) {
                hardwareList->at(0).missing_clone_count++;
                if (hardware) {
                    hardware->missing_clone_count++;
                }
            }
        }

        list.push_back(rom);

        // UI
        if (i % 250 == 0) {
            sprintf(text_str, "Scanning... %i%% - ROMS : %i / %i",
                    (i * 100) / nBurnDrvCount, hardwareList->at(0).supported_count, nBurnDrvCount);
            text->setString(text_str);
            ui->flip();
        }
        // UI
    }

    if (use_icons) {
        // set childs with no icon to parent icon
        for (auto rom : list) {
            if (!rom->icon && rom->parent) {
                auto it = std::find_if(list.begin(), list.end(), [&](const Rom *r) {
                    return r->drv_name == rom->parent;
                });
                if (it != list.end()) {
                    rom->icon = (*it)->icon;
                }
            }
        }
    }

    // cleanup
    RomList::build();
}
