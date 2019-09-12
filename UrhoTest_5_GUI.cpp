
#define URHO3D_ANGELSCRIPT 1    // we want angelscript support
#define URHO3D_LOGGING 1        // we want debug log message support
#include <Urho3D/Urho3DAll.h>

using namespace Urho3D;

/// Fixed-rate object rotation via frame dT
/// Free-Look camera behaviour
/// Custom Debug-Drawing (object local primary axes)
/// HOTLOADING: Load/Save snapshot of scene at any time
/// PRELOADING: Code will
/// Take screen shots
/// Custom UI
class MyApp : public Application
{
public:

    /// Typical Urho class constructor
    MyApp(Context* context):Application(context) { }

    /// Configure application prior to App Window creation
    void Setup()
    {
        engineParameters_["FullScreen"]=true;
        //engineParameters_["FullScreen"]=false;
        //engineParameters_["WindowWidth"]=1280;
        //engineParameters_["WindowHeight"]=720;
        //engineParameters_["WindowResizable"]=true;
        //engine_->DumpResources();
    }

    /// Custom initialization prior to first frame update
    void Start()
    {
        // We're not using AngelScript just yet...
        // context_->RegisterSubsystem(new Script(context_));

        /// Register to receive events of interest
        SubscribeToEvent(E_KEYDOWN,           URHO3D_HANDLER(MyApp,HandleKeyDown));             // Keypress
        SubscribeToEvent(E_UPDATE,            URHO3D_HANDLER(MyApp,HandleFrameUpdate));         // Frame Update
        SubscribeToEvent(E_POSTRENDERUPDATE,  URHO3D_HANDLER(MyApp,HandlePostRenderUpdate));    // Post-Render Update


        /// Obtain access to the root element of the UI system
        uiRoot_ = GetSubsystem<UI>()->GetRoot();

        /// Load XML file containing default UI style sheet
        auto* style = GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

        /// Set the loaded style as default style
        uiRoot_->SetDefaultStyle(style);

        /// Create custom GUI
        PopulateUI();

        /// Create a new Scene, and store the pointer in a "SharedPtr" smartpointer container
        /// This arrangement ensures that when our App dies, the Scene will be automatically deleted.
        gameScene_ = new Scene(context_);

        /// We'll give our new scene a name, just because we can
        gameScene_->SetName(mySceneFilePath_);

        /// Attempt to load scene content from xml file
        if(LoadSceneFromXML(mySceneFilePath_))
            URHO3D_LOGINFO("Loaded GameScene from XML!");

        /// If that fails, we'll use code to populate our scene
        /// and then we'll dump the scene content to xml file for future reference
        else{
            PopulateGameScene();
            SaveSceneToXML(mySceneFilePath_);
            URHO3D_LOGINFO("Populated new GameScene and saved to XML!");
        }
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
        /// The only event data for this event is "frame delta-time", as a float
        using namespace Update;
        float deltaTime = eventData[P_TIMESTEP].GetFloat();

        /// Update our camera behaviour
        MoveCamera(deltaTime);

        /// Let's say we wanted "the box" to rotate at 30 degrees per second, around the World Up Axis
        /// At this rate, it will take 12 seconds to complete one revolution (360 / 30 = 12)
        /// We'll use a Quaternion to represent rotation, and we'll use "angle and axis" to construct it.
        Quaternion myRot(30.0f * deltaTime, Vector3::UP);

        /// Now we apply our rotation to the Node, optionally we provide the desired Transform Space (world, not local).
        gameScene_->GetChild("Box1")->Rotate( myRot, TS_WORLD);


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

        // TAKE SCREEN SHOT
        else if(key==KEY_BACKSPACE)
        {
            // Create temporary Urho3D::Image object to receive screenshot
            Image screenshot(context_);
            // Take screen shot
            GetSubsystem<Graphics>()->TakeScreenShot(screenshot);
            // Save image to file
            screenshot.SavePNG("ScreenShot.png");
        }

        // LOAD SCENE FROM XML FILE
        else if(key==KEY_F11)
            LoadSceneFromXML(mySceneFilePath_);

        // SAVE SCENE TO XML FILE
        else if(key==KEY_F12)
            SaveSceneToXML(mySceneFilePath_);
    }

    /// When scene rendering is "complete", we can perform custom debug-drawing
    /// In our case, we'll draw some RGB coloured lines to help visualize orientation...
    /// Red is the X axis, or Right.
    /// Green is the Y axis, or Up.
    /// Blue is the Z axis, or Forwards.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
    {
        /// Obtain access to our "box node"
        Node* boxnode = gameScene_->GetChild("Box1");

        /// Query the position of the origin of our box, in worldspace terms
        /// This will be the starting point for drawing three coloured lines
        /// intended to visually describe the orientation of our test object
        Vector3 boxPos = boxnode->GetWorldPosition();

        /// DebugRenderer generally wants coordinates in worldspace
        /// We'll transform the major axes from local space to world space, and make them a bit longer than 1
        /// Now we can see which way our object is "facing" - which is usually very important for games!
        /// Color is RGBA floats from 0 to 1 - A is Alpha Transparency - Alpha of 1 is Opaque
        /// The bool argument means "Use Depth-Testing" to hide debugdraw content when its "behind" other surfaces.
        debugDraw_->AddLine(boxPos, boxnode->LocalToWorld(Vector3::RIGHT   * 3.0f), Color(1, 0, 0, 1), true); /// X Axis
        debugDraw_->AddLine(boxPos, boxnode->LocalToWorld(Vector3::UP      * 3.0f), Color(0, 1, 0, 1), true); /// Y Axis
        debugDraw_->AddLine(boxPos, boxnode->LocalToWorld(Vector3::FORWARD * 3.0f), Color(0, 0, 1, 1), true); /// Z Axis


    }

private:

    /// Save gameScene to XML file
    bool SaveSceneToXML(String filepath){
        bool success=false;
        // Dump our Scene to disk, so we can use it to load from in future
        Urho3D::File file(context_, filepath, FILE_WRITE);
        if(file.IsOpen()){
            success = gameScene_->SaveXML(file);
            file.Close();
        }
        return success;
    }

    /// Load gameScene from XML file
    bool LoadSceneFromXML(String filepath){
        bool success = false;
        /// Open a SceneFile for Loading
        Urho3D::File file(context_, filepath, FILE_READ);
        if(file.IsOpen())
        {
            success = gameScene_->LoadXML(file);
            if(success)
                OnSceneReloaded();

            file.Close();
        }
        return success;
    }

    /// Restore any scene-dependent object pointers...
    /// When the new scene is loaded, the old scene is destroyed!
    /// gameScene_ now points to a different scene object.
    /// Some of our object pointers have become invalid.
    /// Also, the viewport has been trashed.
    /// In order to remedy the situation, we will query the new scene!
    void OnSceneReloaded(){

        /// Locate our camera node in the reloaded scene
        cameraNode_ = gameScene_->GetChild("Camera Node");
        /// If the scene does not contain a camera node (!!?!) then create it now.
        if(!cameraNode_)
            cameraNode_ = gameScene_->CreateChild("Camera Node");

        /// Extract initial pitch and yaw values from camera's current orientation
        Vector3 eulers = cameraNode_->GetWorldRotation().EulerAngles();
        yaw_   = eulers.y_;
        pitch_ = eulers.x_;

        /// Access our camera component
        auto* camera = cameraNode_->GetOrCreateComponent<Camera>();

        /// Restore our viewport
        WeakPtr<Viewport> viewport (new Viewport(context_, gameScene_, camera));
        GetSubsystem<Renderer>()->SetViewport(0, viewport);

        /// Restore DebugRenderer
        debugDraw_ = gameScene_->GetOrCreateComponent<DebugRenderer>();
    }

    /// Implements "free-look" camera behaviour
    /// Use WASD to move the camera, and mouse to look around.
    void MoveCamera(float timeStep)
    {

        /// Sanity Check!
        if(!cameraNode_)
            return;

        auto* input = GetSubsystem<Input>();

        /// If system cursor is visible, or app window loses input focus, just get outta here
        if(input->IsMouseVisible() || !input->HasFocus())
            return;

        // Do not move if the UI has a focused element (the console)
        auto* ui = GetSubsystem<UI>();
        if (ui->GetFocusElement())
            return;

        /// Movement speed (world units per second)
        const float MOVE_SPEED = 18.0f;

        /// Mouse sensitivity (degrees per pixel, scaled to match display resolution)
        const float MOUSE_SENSITIVITY = 0.1f * (768.0f / GetSubsystem<Graphics>()->GetHeight());

        /// Convert mouse movement into change in camera pitch and yaw
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_   += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;

        /// We clamp the pitch to +/- 90 degrees so the camera can't flip upside down
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        /// Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

        /// Watch the WASD Keys - notice we're not using the KeyDown event to do so.
        /// Translation will be performed in "Local Space" - so relative to current camera orientation.
        if (input->GetKeyDown(KEY_W))
            cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_S))
            cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_A))
            cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_D))
            cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    }

    /// Populate a (presumably) empty scene
    void PopulateGameScene(){

        /// The first component our new scene needs is an Octree.
        /// This component allows Urho to perform visibility based culling
        /// which helps improve rendering performance in more complex scenes
        /// Since this component does not require a Transform, we'll attach it
        /// directly to the scene root (whose transform is always Identity).
        gameScene_->CreateComponent<Octree>();

        /// Add a debug drawer
        debugDraw_ = gameScene_->CreateComponent<DebugRenderer>();


        /// Next we'll create a Camera for rendering a 3D scene ...
        /// The camera will require a node to provide rotation and translation.
        /// We'll create a new Node as a child of our scene
        /// and we'll store it in a "WeakPtr" smartpointer object.
        /// The reason we use WeakPtr, and not SharedPtr, is because
        /// the Node was not created with "new" - its lifetime is already
        /// being managed by the scene.
        /// WeakPtr won't "keep the object alive", but should the object die,
        /// the weakptr will hold "null", which alerts us that the pointer has become invalid.
        cameraNode_ = gameScene_->CreateChild("Camera Node");
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

        /// Extract the initial pitch and yaw values from the current camera orientation
        /// We won't implement camera roll behavior
        Vector3 eulers = cameraNode_->GetWorldRotation().EulerAngles();
        yaw_   = eulers.y_;
        pitch_ = eulers.x_;

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

        Node* boxNode = gameScene_->CreateChild("Box1");
        boxNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
        object = boxNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/MyFirstMaterial.xml"));
    }

    /// Populate a custom user interface via hardcode
    void PopulateUI(){

        // Create the Window and add it to the UI's root node
        window_ = new Window(context_);
        uiRoot_->AddChild(window_);

        // Set Window size and layout settings
        window_->SetMinWidth(384);
        window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
        window_->SetAlignment(HA_CENTER, VA_CENTER);
        window_->SetName("Window");

        // Create Window 'titlebar' container
        auto* titleBar = new UIElement(context_);
        titleBar->SetMinSize(0, 24);
        titleBar->SetVerticalAlignment(VA_TOP);
        titleBar->SetLayoutMode(LM_HORIZONTAL);

        // Create the Window title Text
        auto* windowTitle = new Text(context_);
        windowTitle->SetName("WindowTitle");
        windowTitle->SetText("Hello GUI! Press TAB to enable/disable cursor!");

        // Create the Window's close button
        auto* buttonClose = new Button(context_);
        buttonClose->SetName("CloseButton");

        // Add the controls to the title bar
        titleBar->AddChild(windowTitle);
        titleBar->AddChild(buttonClose);

        // Add the title bar to the Window
        window_->AddChild(titleBar);

        // Apply styles
        window_    ->SetStyleAuto();
        windowTitle->SetStyleAuto();
        buttonClose->SetStyle("CloseButton");

        /// Subscribe to buttonClose release (following a 'press') events
        /// NOTE!!!
        /// This is a request to receive "FROM A SPECIFIC SENDER" (buttonClose) "A SPECIFIC EVENT" (E_RELEASED)
        ///
        SubscribeToEvent(buttonClose, E_RELEASED,     URHO3D_HANDLER(MyApp, HandleClosePressed));

        /// Subscribe also to all UI mouse clicks just to see where we have clicked
        /// NOTE!!!
        /// This is a request to receive "FROM ANY SENDER" a specific event...
        SubscribeToEvent(             E_UIMOUSECLICK, URHO3D_HANDLER(MyApp, HandleControlClicked));

        /// Populate our UI Window:

        // Create a CheckBox
        auto* checkBox = new CheckBox(context_);
        checkBox->SetName("CheckBox");

        // Create a Button
        auto* button = new Button(context_);
        button->SetName("Button");
        button->SetMinHeight(24);

        // Create a LineEdit
        auto* lineEdit = new LineEdit(context_);
        lineEdit->SetName("LineEdit");
        lineEdit->SetMinHeight(24);

        // Add controls to Window
        window_->AddChild(checkBox);
        window_->AddChild(button);
        window_->AddChild(lineEdit);

        // Apply previously set default style
        checkBox->SetStyleAuto();
        button->SetStyleAuto();
        lineEdit->SetStyleAuto();
    }

    /// User has clicked a "ui window close button"
    void HandleClosePressed(StringHash eventType, VariantMap& eventData)
    {
        if (GetPlatform() != "Web")
            engine_->Exit();
    }

    /// User has clicked a "ui element" (of any kind)
    void HandleControlClicked(StringHash eventType, VariantMap& eventData)
    {

        /// Get control that was clicked
        using namespace UIMouseClick;
        auto* clicked = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());

        /// Access the Text element acting as the Window's title
        /// This code performs a static-cast to the given type.
        /// The boolean "true" indicates that the search should be recursive.
        auto* windowTitle = window_->GetChildStaticCast<Text>("WindowTitle", true);

        /// Set window title to reflect the name of the clicked ui element
        String name = "...?";
        if (clicked) name = clicked->GetName();
        windowTitle->SetText("Hello " + name + "!");
    }

    ///!/////////////////////////////////////////////////////////////////////////////////////////////////////
    ///! Generally, I like to put all the data members together, either at the top or bottom of the class ///
    ///!/////////////////////////////////////////////////////////////////////////////////////////////////////

    /// The default filepath for loading/saving scene content
    String mySceneFilePath_ = "MyGameScene.xml";

    /// Our scene object - we used "new" - we very much own it, so we use a SharedPtr to hold it
    /// This object will live until our MyApp object is about to be destroyed
    /// at which point SharedPtr will delete it.
    /// Note: loading scene from file will cause the scene to be deleted and replaced.
    SharedPtr<Scene> gameScene_;

    /// We introduce the notion of "custom" debug drawing
    /// We'll need a DebugRenderer component in our Scene somewhere...
    /// All component lifetime is tied to that of the owner Scene
    /// which means again, we're responsible for restoring this guy after a scene reload.
    WeakPtr<DebugRenderer> debugDraw_;

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
    /// If the scene is reloaded, its up to us to restore this pointer from the new scene!
    WeakPtr<Node> cameraNode_;

    /// Our camera pitch and yaw angles are two of the three "Euler" angles
    /// They describe our camera node's current orientation, in degrees
    float yaw_, pitch_;

    /// The Window.
    SharedPtr<Window> window_;

    /// The UI's root UIElement.
    WeakPtr<UIElement> uiRoot_;

};

URHO3D_DEFINE_APPLICATION_MAIN(MyApp)
