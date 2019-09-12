
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
    MyApp(Context* context):Application(context) { }

    void Setup()
    {
        engineParameters_["FullScreen"]=true;
        //engineParameters_["FullScreen"]=false;
        //engineParameters_["WindowWidth"]=1280;
        //engineParameters_["WindowHeight"]=720;
        //engineParameters_["WindowResizable"]=true;
        //engine_->DumpResources();
    }

    /// Let's set up a simple scene, using "hardcode" to do it...
    void Start()
    {
        // We're not using AngelScript just yet...
        // context_->RegisterSubsystem(new Script(context_));

        /// Register to receive keypress events
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MyApp,HandleKeyDown));
        SubscribeToEvent(E_UPDATE,  URHO3D_HANDLER(MyApp,HandleFrameUpdate));

        /// Create a new Scene, and store the pointer in a "SharedPtr" smartpointer container
        /// This arrangement ensures that when our App dies, the Scene will be automatically deleted.
        gameScene_ = new Scene(context_);

        /// We'll give our new scene a name, just because we can
        gameScene_->SetName("My Game Scene");

        /// The first component our new scene needs is an Octree.
        /// This component allows Urho to perform visibility based culling
        /// which helps improve rendering performance in more complex scenes
        /// Since this component does not require a Transform, we'll attach it
        /// directly to the scene root (whose transform is always Identity).
        gameScene_->CreateComponent<Octree>();

        /// Next we'll create a Camera for rendering a 3D scene ...
        /// The camera will require a node to provide rotation and translation.
        /// We'll create a new Node as a child of our scene
        /// and we'll store it in a "WeakPtr" smartpointer object.
        /// The reason we use WeakPtr, and not SharedPtr, is because
        /// the Node was not created with "new" - its lifetime is already
        /// being managed by the scene.
        /// WeakPtr won't "keep the object alive", but should the object die,
        /// the weakptr will hold "null", which alerts us that the pointer has become invalid.
        Node* cameraNode_ = gameScene_->CreateChild("Camera Node");
        cameraNode_->Translate(Vector3(20,20,-20));          /// Move camera back, and way up
        cameraNode_->LookAt(Vector3::ZERO, Vector3::UP);    /// Make camera look down and forward at the World Origin

        /// We'll create the Camera component itself as a child of our "Camera Node"
        /// The node provides us with a "transform" to position and rotate the camera,
        /// while the camera component contains the camera-specific code and data.
        Camera* camera = cameraNode_->CreateComponent<Camera>();

        /// We'll set the camera's far clip plane to something reasonable for our scene
        /// This effectively sets the limit for the max. viewing distance that the camera can "see"
        camera->SetFarClip(100.0f);

        /// Create and Setup a Viewport - effectively associating a camera with the scene it will render
        /// We need WeakPtr here to safely hand over ownership of this special object to Urho
        /// Even though we used "new", we don't own this object - this is an exception to the rule!
        WeakPtr<Viewport> viewport (new Viewport(context_, gameScene_, camera));
        /// Hand (ownership of) the viewport to the Rendering system
        GetSubsystem<Renderer>()->SetViewport(0, viewport);

        /// That's the camera creation taken care of...

        /// Next we'll add a "Zone" component.
        /// This component provides ambient light and fog.
        /// Without a Zone, and with no other sources of light, our scene will remain totally black...
        /// Our scene objects don't need to be children of the zone to fall within its volume...
        Node* zoneNode = gameScene_->CreateChild("My Zone");
        auto* zone = zoneNode->CreateComponent<Zone>();
        /// Set Ambient Light (RGB) to soft white, almost black
        zone->SetAmbientColor(Color(0.25f, 0.25f, 0.25f));
        /// Set up Fog
        zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
        zone->SetFogStart(80.0f);
        zone->SetFogEnd(100.0f);
        /// Provide the extents (or size) of the Zone BoundingBox
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

        /// Create "the floor" using a StaticModel component with its own transform node
        /// We'll use "Scale" to stretch a "unit cube" model to form the floor.

        /// First we create our node, and may as well set up its transform
        Node* floorNode = gameScene_->CreateChild("Floor");
        //floorNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        floorNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));

        /// Then we attach our component to the node
        auto* object = floorNode->CreateComponent<StaticModel>();

        /// We'll manually set up a Model and Material for this simple entity
        /// Urho provides some assets we can play with while getting started...
        auto* cache = GetSubsystem<ResourceCache>();
        object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

        /// Create a 3D box "on the floor"
        Node* boxNode = gameScene_->CreateChild("Box1");
        /// The size of the box model is 2x2x2 units (-1 to +1 on each axis)
        /// and since our floor is at Y=0, we need to translate the box UP one unit.
        boxNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
        object = boxNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        /// We should use a different material so we can clearly see this box object
        /// Locate the "Bin/Data/Materials/Stone.xml" file, and change the "diffuse texture".
        /// Save the material to a new filename... now we can use it.
        object->SetMaterial(cache->GetResource<Material>("Materials/MyFirstMaterial.xml"));

        URHO3D_LOGINFO("OK! Our application is now ready to rock!");

        // Set mouse behaviour:
        //GetSubsystem<Input>()->SetMouseMode(MM_RELATIVE);
   //     GetSubsystem<Input>()->SetMouseVisible(true);
    }

    /// Frame Update event handler is used to perform per-frame logic
    /// All Urho events use "VariantMap" to pass any relevant data along with the event.
    /// The data sent is different for every Urho event.
    /// VariantMap is an alias for "Map<StringHash, Variant>"
    /// Variant is an Urho value container that can hold almost any kind of value or object.
    /// These will likely become more familiar in time, but know they are Urho classes.
    void HandleFrameUpdate(StringHash eventType, VariantMap& eventData)
    {
        /// Let's unpack the event data for E_UPDATE (frame update)
        /// It's merely the frame delta-time, as a float
        using namespace Update;
        float deltaTime = eventData[P_TIMESTEP].GetFloat();

        /// Now we know the frame dT, let's do something with it.

        /// CODING TASK 1:
        /// Make the box rotate at a fixed rate around world Y axis
    }

    /// A keyboard press was detected..
    /// This "event handler" will be triggered to respond to that particular event.
    //
    void HandleKeyDown(StringHash eventType, VariantMap& eventData)
    {
        using namespace KeyDown;
        int key = eventData[P_KEY].GetInt();

        if (key == KEY_ESCAPE)   // Escape key to quit application
            engine_->Exit();

        else if(key==KEY_TAB)    // toggle mouse cursor visibility
        {
            auto* input = GetSubsystem<Input>();
            input->SetMouseVisible(!input->IsMouseVisible());
        }

        /// TAKE SCREEN SHOT
        else if(key==KEY_BACKSPACE)
        {
            // Create temporary Urho3D::Image object to receive screenshot
            Image screenshot(context_);
            // Take screen shot
            GetSubsystem<Graphics>()->TakeScreenShot(screenshot);
            // Save image to file
            screenshot.SavePNG("ScreenShot.png");
        }

        /// LOAD SCENE FROM XML FILE
        else if(key==KEY_F11){
            /// Open a SceneFile for Loading
            Urho3D::File file(context_, "MyGameScene.xml", FILE_READ);
            if(file.IsOpen())
            {
                bool success = gameScene_->LoadXML(file);
                if(success)
                {
                    /// When the new scene is loaded, the old one is destroyed!
                    /// gameScene_ is still valid, but the scene has changed.
                    /// Our cameraNode_ is invalid, and our viewport, is trashed.
                    /// We need to restore them!

                    /// Locate our camera node in the reloaded scene
                    cameraNode_ = gameScene_->GetChild("Camera Node");

                    /// Access our camera component
                    auto* camera = cameraNode_->GetComponent<Camera>();

                    /// Restore our viewport
                    WeakPtr<Viewport> viewport (new Viewport(context_, gameScene_, camera));
                    GetSubsystem<Renderer>()->SetViewport(0, viewport);
                }
                file.Close();
            }
        }

        /// SAVE SCENE TO XML FILE
        else if(key==KEY_F12){
            // Dump our Scene to disk, so we can use it to load from in future
            Urho3D::File file(context_, "MyGameScene.xml", FILE_WRITE);
            gameScene_->SaveXML(file);
            file.Close();
        }
    }

    /// Our scene object - we used "new" - we very much own it, so we use a SharedPtr to hold it
    /// This object will live until our MyApp object is about to be destroyed
    /// at which point SharedPtr will delete it.

    SharedPtr<Scene> gameScene_;

    /// Our camera control node, used to rotate and translate our camera
    /// We don't own nodes or components, scenes do...
    /// Since "its not ours", we store it in a WeakPtr
    /// Why not just store it in a naked Node* ???
    /// Because if the node was for any reason destroyed "somewhere else", we'd never know,
    /// and the next time we tried to access the pointer, we'd crash badly.
    /// At least with a weak pointer, you will end up landing in a "null pointer exception"
    /// which is a massive clue that your weakptr became invalidated...
    /// WeakPtr lets you at least check before you try to use a bad pointer
    /// eg "if (cameraNode_ != nullptr) doStuff();"
    WeakPtr<Node> cameraNode_;

};

URHO3D_DEFINE_APPLICATION_MAIN(MyApp)
