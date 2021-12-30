#include "OTRWindow.h"
#include "spdlog/spdlog.h"
#include "KeyboardController.h"
#include "OTRContext.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include <map>
#include <string>

extern "C" {
    struct OSMesgQueue;

    uint8_t __osMaxControllers = MAXCONTROLLERS;

    int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status) {
        std::shared_ptr<OtrLib::OTRConfigFile> pConf = OtrLib::OTRContext::GetInstance()->GetConfig();
        OtrLib::OTRConfigFile& Conf = *pConf.get();

        for (size_t i = 0; i < __osMaxControllers; i++) {
            std::string ControllerType = Conf["CONTROLLERS"]["CONTROLLER " + std::to_string(i)];
            mINI::INIStringUtil::toLower(ControllerType);

            if (ControllerType == "keyboard") {
                OtrLib::OTRController* pad = new OtrLib::KeyboardController(i);
                OtrLib::OTRWindow::Controllers[i] = std::shared_ptr<OtrLib::OTRController>(pad);
            } else if (ControllerType == "Unplugged") {
                // Do nothing for unplugged controllers
            } else {
                spdlog::error("Invalid Controller Type: {}", ControllerType);
            }
        }

        for (size_t i = 0; i < __osMaxControllers; i++) {
            if (OtrLib::OTRWindow::Controllers[i] != nullptr) {
                *controllerBits |= 1 << i;
            }
        }

        return 0;
    }

    int32_t osContStartReadData(OSMesgQueue* mesg) {
        return 0;
    }

    void osContGetReadData(OSContPad* pad) {
        pad->button = 0;
        pad->stick_x = 0;
        pad->stick_y = 0;
        pad->err_no = 0;

        for (size_t i = 0; i < __osMaxControllers; i++) {
            if (OtrLib::OTRWindow::Controllers[i] != nullptr) {
                OtrLib::OTRWindow::Controllers[i]->Read(&pad[i]);
            }
        }
    }
}

extern "C" struct GfxRenderingAPI gfx_opengl_api;
extern "C" struct GfxWindowManagerAPI gfx_sdl;
extern "C" void StupidShimThingy(GfxWindowManagerAPI** WmApi, GfxRenderingAPI** RenderingApi);

namespace OtrLib {
    std::shared_ptr<OtrLib::OTRController> OTRWindow::Controllers[MAXCONTROLLERS] = { nullptr };

    OTRWindow::OTRWindow(std::shared_ptr<OTRContext> Context) : Context(Context) {
        StupidShimThingy(&WmApi, &RenderingApi);
        //WmApi = &gfx_sdl;
        //RenderingApi = &gfx_opengl_api;
    }

    void OTRWindow::Init() {
        gfx_init(WmApi, RenderingApi, Context->GetName().c_str(), false);
        WmApi->set_keyboard_callbacks(OTRWindow::KeyDown, OTRWindow::KeyUp, OTRWindow::AllKeysUp);
    }

    void OTRWindow::RunCommands(Gfx* Commands) {
        gfx_start_frame();
        gfx_run(Commands);
        gfx_end_frame();
    }

    void OTRWindow::SetFrameDivisor(int divisor)
    {
        gfx_set_framedivisor(divisor);
    }

    void OTRWindow::MainLoop(void (*MainFunction)(void)) {
        WmApi->main_loop(MainFunction);
    }

    bool OTRWindow::KeyDown(int32_t dwScancode) {
        bool bIsProcessed = false;
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(OtrLib::OTRWindow::Controllers[i].get());
            if (pad != nullptr) {
                if (pad->PressButton(dwScancode)) {
                    bIsProcessed = true;
                }
            }
        }

        return bIsProcessed;
    }

    bool OTRWindow::KeyUp(int32_t dwScancode) {
        bool bIsProcessed = false;
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(OtrLib::OTRWindow::Controllers[i].get());
            if (pad != nullptr) {
                if (pad->ReleaseButton(dwScancode)) {
                    bIsProcessed = true;
                }
            }
        }

        return bIsProcessed;
    }

    void OTRWindow::AllKeysUp(void) {
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(OtrLib::OTRWindow::Controllers[i].get());
            if (pad != nullptr) {
                pad->ReleaseAllButtons();
            }
        }
    }
}