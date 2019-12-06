#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "AssetManager.h"
#include "MiniConfig.h"
#include "DepthSensor.h"
#include "TextureHelper.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct ClothieApp : public App
{
    void setup() override
    {
        const auto& args = getCommandLineArgs();
        log::makeLogger<log::LoggerFileRotating>(fs::path(), "IG.%Y.%m.%d.log");

        ds::DeviceType type = (ds::DeviceType)SENSOR_TYPE;
        ds::Option option;
        option.enableColor = true;
        option.enableBody = true;
        mDevice = ds::Device::create(type, option);
        if (!mDevice->isValid())
        {
            CI_LOG_E("Invalid sensor for " << ds::strFromType(type));
            quit();
            return;
        }

        mDevice->signalColorDirty.connect([&] {
            updateTexture(mColorTexture, mDevice->colorSurface);
            });

        mDevice->signalDepthDirty.connect([&]{
            updateTexture(mDepthTexture, mDevice->depthChannel, getTextureFormatUINT16());
        });

        mCamUi = CameraUi( &mCam, getWindow(), -1 );
        
        createConfigUI({200, 200});
        gl::enableDepth();
    
        getWindow()->getSignalResize().connect([&] {
            mCam.setAspectRatio( getWindowAspectRatio() );
        });

        getSignalCleanup().connect([&] { writeConfig(); });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });
        


        getWindow()->getSignalDraw().connect([&] {
            //gl::setMatrices( mCam );
            gl::clear();

            if (SHOW_COLOR)
            {
                if (mColorTexture)
                {
                    auto glsl = am::glslProg("texture");
                    gl::ScopedGlslProg prog(glsl);
                    glsl->uniform("uTex0", 0);
                    gl::ScopedTextureBind tex0(mColorTexture);
                    gl::drawSolidRect(getWindowBounds());
                }
            }
            else
            {
                if (mDepthTexture)
                {
                    auto glsl = am::glslProg("depthMap.vs", "depthMap.fs");
                    glsl->uniform("uTex0", 0);
                    gl::ScopedGlslProg prog(glsl);
                    gl::ScopedTextureBind tex0(mDepthTexture);
                    gl::drawSolidRect(getWindowBounds());
                }
            }
        });
    }
    
    CameraPersp         mCam;
    CameraUi            mCamUi;
    gl::TextureRef      mColorTexture, mDepthTexture;
    ds::DeviceRef       mDevice;
};

CINDER_APP( ClothieApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
} )
