//
// Created by cpasjuste on 01/06/18.
//

#include "burner.h"
#include "burnint.h"
#include "driverlist.h"

#include "c2dui.h"
#include "uiEmu.h"
#include "video.h"

using namespace c2d;
using namespace c2dui;

extern int InpMake(Input::Player *players);

extern unsigned char inputServiceSwitch;
extern unsigned char inputP1P2Switch;

static unsigned int myHighCol16(int r, int g, int b, int /* i */) {
    unsigned int t;
    t = (unsigned int) ((r << 8) & 0xf800); // rrrr r000 0000 0000
    t |= (g << 3) & 0x07e0; // 0000 0ggg ggg0 0000
    t |= (b >> 3) & 0x001f; // 0000 0000 000b bbbb
    return t;
}

PFBAGuiEmu::PFBAGuiEmu(UIMain *ui) : UIEmu(ui) {

    printf("PFBAGuiEmu()\n");
}

int PFBAGuiEmu::load(RomList::Rom *rom) {

    int fps;

    nBurnDrvActive = rom->drv;
    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("PFBAGui::runRom: driver not found\n");
        return -1;
    }

    ///////////
    // AUDIO
    //////////
    // get fps
    bForce60Hz = getUi()->getConfig()->get(Option::Id::ROM_FORCE_60HZ, true)->getValueBool();
    bForce50Hz = getUi()->getConfig()->get(Option::Id::ROM_FORCE_50HZ, true)->getValueBool();
    if (bForce60Hz) {
        nBurnFPS = fps = 6000;
    } else if (bForce50Hz) {
        nBurnFPS = fps = 5000;
    } else {
        pDriver[nBurnDrvActive]->Init();
        fps = nBurnFPS;
        pDriver[nBurnDrvActive]->Exit();
    }

    int freq = getUi()->getConfig()->get(Option::Id::ROM_AUDIO_FREQ)->getValueInt();
    addAudio(freq, (float) fps / 100.0f);
    if (getAudio()->isAvailable()) {
        nInterpolation = getUi()->getConfig()->get(Option::Id::ROM_AUDIO_INTERPOLATION)->getValueInt();
        nFMInterpolation = getUi()->getConfig()->get(Option::Id::ROM_AUDIO_FMINTERPOLATION)->getValueInt();
        nBurnSoundRate = getAudio()->getSampleRate();
        nBurnSoundLen = getAudio()->getBufferLen();
        pBurnSoundOut = getAudio()->getBuffer();
    }
    audio_sync = getUi()->getConfig()->get(Option::Id::ROM_AUDIO_SYNC, true)->getValueBool();
    ///////////
    // AUDIO
    //////////

    ///////////////
    // FBA DRIVER
    ///////////////
    EnableHiscores = 1;
    InpInit();
    InpDIP();
    printf("Initialize driver...\n");
    if (DrvInit(rom->drv, false) != 0) {
        printf("\nDriver initialisation failed\n");
        getUi()->getUiProgressBox()->setVisibility(Visibility::Hidden);
        getUi()->getUiMessageBox()->show("ERROR", "DRIVER INIT FAILED", "OK");
        stop();
        return -1;
    }
    nFramesEmulated = 0;
    nFramesRendered = 0;
    nCurrentFrame = 0;
    setFrameDuration(1.0f / ((float) nBurnFPS / 100.0f));
    //printf("frame_duration: %f\n", getFrameDuration());
    printf("FORCE_60HZ: %i, FORCE_50HZ: %i, AUDIO_SYNC: %i, FPS: %f (BURNFPS: %f)\n",
           bForce60Hz, bForce50Hz, audio_sync, (float) fps / 100.0f, (float) nBurnFPS / 100.0f);
    ///////////////
    // FBA DRIVER
    ///////////////

    //////////
    // VIDEO
    //////////
    int w, h;
    BurnDrvGetFullSize(&w, &h);
    nBurnBpp = 2;
    BurnHighCol = myHighCol16;
    BurnRecalcPal();
    PFBAVideo *v = new PFBAVideo(getUi(), (void **) &pBurnDraw, &nBurnPitch, Vector2f(w, h));
    addVideo(v);
    //////////
    // VIDEO
    //////////

    return UIEmu::load(rom);
}

void PFBAGuiEmu::stop() {

    DrvExit();
    InpExit();

    UIEmu::stop();
}

void PFBAGuiEmu::updateFb() {

    if (pBurnDraw == nullptr) {
        nFramesEmulated++;
        nCurrentFrame++;
        nFramesRendered++;
        getVideo()->getTexture()->lock(nullptr, (void **) &pBurnDraw, &nBurnPitch);
        BurnDrvFrame();
        getVideo()->getTexture()->unlock();
    }
}

void PFBAGuiEmu::renderFrame(bool draw) {

    if (!isPaused()) {

        nFramesEmulated++;
        nCurrentFrame++;

        pBurnDraw = nullptr;
        if (draw) {
            nFramesRendered++;
            getVideo()->getTexture()->lock(nullptr, (void **) &pBurnDraw, &nBurnPitch);
        }
        BurnDrvFrame();
        if (draw) {
            getVideo()->getTexture()->unlock();
        }

        if (getAudio() && getAudio()->isAvailable()) {
            getAudio()->play(audio_sync);
        }
    }
}

void PFBAGuiEmu::updateFrame() {

    // TODO
    //int frameSkip = getUi()->getConfig()->getValue(Option::Index::ROM_FRAMESKIP, true);

    renderFrame();
    /*
    timer += getUi()->getDeltaTime().asSeconds();
    if (timer >= 1) {
        timer = 0;
        printf("fps: %.2g/%2d, delta: %f\n", getUi()->getFps(), (nBurnFPS / 100),
               getUi()->getDeltaTime().asSeconds());
    }
    */
}

bool PFBAGuiEmu::onInput(c2d::Input::Player *players) {

    if (getUi()->getUiMenu()->isVisible()
        || getUi()->getUiStateMenu()->isVisible()) {
        return UIEmu::onInput(players);
    }

    // TODO: cross2d: add universal input rotation support
#if 0
    int rotation_config =
            getUi()->getConfig()->get(Option::Index::ROM_ROTATION, true)->getValueBool();
    int rotate_input = 0;
#ifdef __PSP2__
    // TODO: find a way to unify platforms,
    // or allow rotation config from main.cpp
    if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        if (rotation_config == 0) {
            //rotate controls by 90 degrees
            rotate_input = 1;
        } else if (rotation_config == 2) {
            //rotate controls by 270 degrees
            rotate_input = 3;
        }
    }
#elif __SWITCH__
    if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        if (rotation_config == 0) {             // OFF
            //rotate controls by 270 degrees
            rotate_input = 3;
        } else if (rotation_config == 1) {      // ON
            //rotate controls by 270 degrees
            rotate_input = 0;
        } else if (rotation_config == 2) {      // FLIP
            //rotate controls by 270 degrees
            rotate_input = 1;
        }
    }
#else
    if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        rotate_input = rotation_config ? 0 : 3;
    }
#endif
#endif

    inputServiceSwitch = 0;
    inputP1P2Switch = 0;

    // look for player 1 combos key
    if ((players[0].keys & Input::Key::Select) && (players[0].keys & Input::Key::Fire3)) {
        inputServiceSwitch = 1;
    } else if ((players[0].keys & Input::Key::Select) && (players[0].keys & Input::Key::Fire4)) {
        inputP1P2Switch = 1;
    }

    return UIEmu::onInput(players);
}

void PFBAGuiEmu::onDraw(c2d::Transform &transform, bool draw) {

    if (!isPaused()) {

        // fps
        int showFps = getUi()->getConfig()->get(Option::Id::ROM_SHOW_FPS, true)->getValueBool();
        getFpsText()->setVisibility(showFps ? c2d::Visibility::Visible : c2d::Visibility::Hidden);
        if (showFps) {
            sprintf(getFpsString(), "FPS: %.2g/%2d", getUi()->getFps(), nBurnFPS / 100);
            getFpsText()->setString(getFpsString());
        }

        auto players = getUi()->getInput()->getPlayers();
        InpMake(players);
        updateFrame();
    }

    UIEmu::onDraw(transform, draw);
}
