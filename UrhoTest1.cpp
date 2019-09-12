
/// It's possible to include just what you really need in order to speed up build times
/// but its definitely easiest to just include everything and let the compiler figure it out
#define URHO3D_ANGELSCRIPT 1    // we want angelscript support
#define URHO3D_LOGGING 1        // we want debug log message support
#include <Urho3D/Urho3DAll.h>
using namespace Urho3D;

/// Let's define (and implement) our custom application class
/// Note it derives from Urho3D::Application class - there's some virtual methods we can override...
///
class MyApp : public Application
{

public:

    /// Most Urho3D classes have a constructor that takes an "urho engine context" object (by pointer).
    /// All we need to do, is pass that pointer to whatever class is our immediate ancestor!
    /// Having done so, the (inherited) class member "context_" should be available at all times.
    MyApp(Context* context):Application(context) { }

    /// Called before engine initialization... engineParameters_ member variable can be modified here
    void Setup()
    {
        engineParameters_["FullScreen"]=true;
        //engineParameters_["FullScreen"]=false;
        //engineParameters_["WindowWidth"]=1280;
        //engineParameters_["WindowHeight"]=720;
        //engineParameters_["WindowResizable"]=true;
        //engine_->DumpResources();
    }

    /// Called when Urho has completed initialization / created app window,
    /// but before the first frame update occurs
    void Start()
    {
        /// Register the AngelScript subsystem (a non-core subsystem) with Urho3D
        context_->RegisterSubsystem(new Script(context_));

        /// Register application handlers for "Core Events":
        /// All objects deriving from Urho3D::Object can register to receive specific events.
        /// Some events are broadcast engine-wide, while others have a specific "sender" (we'll cover that later).
        /// Here's some of the most common events:
        // E_BEGINFRAME                 (Start of frame event)
        // E_UPDATE                     (Logic update event, usually used to do "normal" stuff each frame)
        // E_POSTUPDATE                 (Logic post-update event)
        // E_RENDERUPDATE               (Render update event)
        // E_POSTRENDERUPDATE           (Post-render update event)
        // E_ENDFRAME                   (End frame event)
        // See https://github.com/urho3d/Urho3D/wiki/Events for a complete list of possible events
        //
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MyApp,HandleKeyDown));

        URHO3D_LOGINFO("OK! Our application is now ready to rock!");

        // Set mouse behaviour:
        //GetSubsystem<Input>()->SetMouseMode(MM_RELATIVE);
   //     GetSubsystem<Input>()->SetMouseVisible(true);
    }

    /// A keyboard press was detected..
    /// This "event handler" will be triggered to respond to that particular event.
    //
    void HandleKeyDown(StringHash eventType, VariantMap& eventData)
    {
        using namespace KeyDown;
        // Check KeyPress. Note the engine_ member variable for convenience access to the Engine object
        int key = eventData[P_KEY].GetInt();

        if (key == KEY_ESCAPE)   // Escape key to quit application
            engine_->Exit();

        else if(key==KEY_TAB)    // toggle mouse cursor visibility
        {
            auto* input = GetSubsystem<Input>();
            input->SetMouseVisible(!input->IsMouseVisible());
        }

        else if(key==KEY_BACKSPACE)// SDLK_PRINTSCREEN) // Take a screen shot
        {
            // Create temporary Urho3D::Image object to receive screenshot
            Image screenshot(context_);
            // Take screen shot
            GetSubsystem<Graphics>()->TakeScreenShot(screenshot);
            // Save image to file
            screenshot.SavePNG("ScreenShot.png");
        }
    }
};

/// This macro hides the implementation of "int main()" application entrypoint function
/// and also hides code that creates and destroys one instance of "MyApp"
/// and also hides the code for "main loop" that drives Urho3D
URHO3D_DEFINE_APPLICATION_MAIN(MyApp)
