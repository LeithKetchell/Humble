
#define URHO3D_ANGELSCRIPT 1    // include support for AngelScript, not that we're using it yet
#define URHO3D_LOGGING 1        // include support for debug messages, they are very handy
#include <Urho3D/Urho3DAll.h>   // we are too lazy to optimize header inclusion any further

#include "GameSceneController.h"
#include "InGameEditor.h"

using namespace Urho3D;

/// Urho Lesson 9
/// Improved custom character controller
/// Object Selection via Raycast
/// Switch "characters" in realtime (any drawable object, including the "floor")
///
/// Use TAB to hide/show custom gui
/// Use F11/F12 to load/save both scene and ui state
/// Use LEFT MOUSE to Select a Candidate Object
/// Use F1 to Take Control of Current Selected Object
/// Use F2 to Pause Scene Updates (does not affect Editor functionality or GUI)
/// Use SPACE to toggle camera between FreeLook and Chase camera behaviours
/// In Free-Look mode,
/// -- Use WASD to translate your Camera "relative to the Camera Facing Direction"
/// -- Use Mouse to orient your Camera view
/// In Chase mode,
/// -- Use RIGHT MOUSE to rotate your Character "Toward Camera Facing Direction"
/// -- Use MIDDLEMOUSE to freely rotate your Character
/// -- Use WASD to translate your Camera. Default is "relative to the Camera Facing Direction"
/// -- Use LEFT SHIFT to translate your Character "relative to the Character Facing Direction"
/// -- Use Mouse to orient your Camera view


class MyApp : public Application
{
public:

    enum CameraBehaviour{
        FreeLook,
        Chase
    };

    CameraBehaviour cameraBehaviour_ = Chase;

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

        InGameEditor::RegisterObject(context_);

        /// Register to receive major events of interest
        /// Note: we don't care who the "Sender" of these events is,
        /// we're interested in receiving these events from "Any Sender".
        SubscribeToEvent(E_KEYDOWN,           URHO3D_HANDLER(MyApp,HandleKeyDown));             // Keypress
        SubscribeToEvent(E_MOUSEBUTTONDOWN,   URHO3D_HANDLER(MyApp,HandleMouseButtonDown));     // MouseButton Down
        SubscribeToEvent(E_MOUSEBUTTONUP,     URHO3D_HANDLER(MyApp,HandleMouseButtonUp));       // MouseButton Up
        SubscribeToEvent(E_UPDATE,            URHO3D_HANDLER(MyApp,HandleFrameUpdate));         // Frame Update
        SubscribeToEvent(E_POSTRENDERUPDATE,  URHO3D_HANDLER(MyApp,HandlePostRenderUpdate));    // Post-Render Update
        SubscribeToEvent(E_UIMOUSECLICK,      URHO3D_HANDLER(MyApp,HandleControlClicked));      // User clicked a UI element

        /// Perform our custom application setup / initialization
        Setup_UI();
        Setup_Scene();

        URHO3D_LOGINFO("OK! Our application is now ready to rock!");


        // Set mouse behaviour:
        //GetSubsystem<Input>()->SetMouseMode(MM_RELATIVE);
   //     GetSubsystem<Input>()->SetMouseVisible(true);
    }

private:

    /// Set up our 2D GUI
    void Setup_UI(){
        /// Obtain access to the root element of the UI system
        uiRoot_ = GetSubsystem<UI>()->GetRoot();

        /// Load XML file containing default UI style sheet
        auto* style = GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

        /// Set the loaded style as default style
        uiRoot_->SetDefaultStyle(style);

        /// Attempt to load UI Layout from xml file
        if(LoadGUIFromXML(myGUILayoutFilePath_))
            URHO3D_LOGINFO("Loaded GUI Layout from XML!");

        /// If that fails, we'll use code to populate our UI
        /// and then we'll dump the UI content to xml file for future reference
        else
        {
            PopulateUI();
            SaveGUIToXML(myGUILayoutFilePath_);
            URHO3D_LOGINFO("Populated new GUI Layout and saved to XML!");
        }


        if(window_->GetChild("DropDownList",true))
                return;

        /// We're going to add a dropdown list to our GUI
        /// We'll need a suitable font for text elements
        auto* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");

        auto* list=new DropDownList(context_);
        list->SetName("DropDownList");
        list->SetResizePopup(true); /// Expand popup to width of container
        list->SetStyleAuto();       /// Use default style


        /// Create a Text element and add it to the dropdownlist
        auto* text=new Text(context_);
        text->SetName("Text Element 1");
        text->SetFont(font);
        text->SetText("testing 123");
        text->SetStyleAuto();
        list->AddItem(text);

        /// Set minimum size of list element to font size plus some border pixels
        list->SetMinHeight(text->GetFontSize()+8);

        /// Create a couple more Text elements for dropdown list
        text=new Text(context_);
        text->SetName("Text Element 2");
        text->SetFont(font);
        text->SetText("testing 456");
        text->SetStyleAuto();
        list->AddItem(text);

        text=new Text(context_);
        text->SetName("Text Element 3");
        text->SetFont(font);
        text->SetText("testing 789");
        text->SetStyleAuto();
        list->AddItem(text);



        // Add DropDownList to Window
        window_->AddChild(list);

        /// Tell the window we can move it (its "draggable")
        window_->SetMovable(true);

    }

    /// Set up our 3D Scene
    void Setup_Scene(){
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
    }

    /// Save gameScene to XML file
    bool SaveSceneToXML(String filepath){
        bool success=false;

        /// Any scene node or component can have custom data associated with it
        /// We'll abuse this to "Save" our camera behaviour type
        gameScene_->SetVar("Camera Behaviour", cameraBehaviour_);

        gameScene_->SetVar("Character Node", characterNode_->GetID());

        /// absolute filepaths are good
        /// We'll ask Urho for the full path to its resource folder (ie Bin)
        /// and tack our relative filepath on the end
        String fullpath=GetSubsystem<FileSystem>()->GetProgramDir ()+filepath ;

        // Dump our Scene to disk, so we can use it to load from in future
        Urho3D::File file(context_, fullpath, FILE_WRITE);
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
        String fullpath=GetSubsystem<FileSystem>()->GetProgramDir ()+filepath ;

        Urho3D::File file(context_, fullpath, FILE_READ);
        if(file.IsOpen())
        {
            success = gameScene_->LoadXML(file);
            if(success){
                /// Retrieve custom data we saved earlier
                cameraBehaviour_ = (CameraBehaviour)gameScene_->GetVar("Camera Behaviour").GetUInt();
                /// Repair weak pointers
                OnSceneReloaded();
            }

            file.Close();
        }
        return success;
    }

    /// Save UI content to XML file
    bool SaveGUIToXML(String filepath){
         bool success=false;
        // Dump our Scene to disk, so we can use it to load from in future
        String fullpath=GetSubsystem<FileSystem>()->GetProgramDir ()+filepath ;
        Urho3D::File file(context_, fullpath, FILE_WRITE);
        if(file.IsOpen()){
            success = GetSubsystem<UI>()->SaveLayout(file,window_);
            file.Close();
        }
        return success;
    }

    /// Load UI content from XML file stored in app root folder (ie Bin folder)
    bool LoadGUIFromXML(String filepath){
        bool success=false;

        // Example: load UI from resource folder
        //SharedPtr<UIElement> newRoot = GetSubsystem<UI>()->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/UILoadExample.xml"));

        /// Format absolute filepath
        String fullpath=GetSubsystem<FileSystem>()->GetProgramDir ()+filepath ;

        /// Create a File object
        File file(context_);

        /// Use our File object to open a file
        file.Open(fullpath);
        if(file.IsOpen()){

            /// Load a UI Layout from file
            SharedPtr<UIElement> newRoot = GetSubsystem<UI>()->LoadLayout(file);
            if(newRoot!=nullptr){

                /// Yay for us!
                success=true;

                /// Remove old UI window content (if any)
                if(window_) window_->Remove();

                /// Attach resulting UI element to the root of the UI system
                uiRoot_->AddChild(newRoot);

                /// Replace weak pointers (including window_)
                OnGUIReloaded();
            }
            file.Close();
        }
        return success;
    }

    /// Restore any UI-dependent weak object pointers... Set event handlers...
    /// After loading GUI from xml file, we need to fix up our weak pointers
    /// and set up our event subscription based on the new values
    void OnGUIReloaded(){
        /// Restore object pointers / Hook up our UI events after UI reload
        window_=            uiRoot_->GetChild("Window",true)->Cast<Window>();
        auto* slider =      uiRoot_->GetChild("Slider",true);
        auto* buttonClose = uiRoot_->GetChild("CloseButton",true);

        /// Subscribe to events with specific senders
        SubscribeToEvent(window_,     E_DRAGMOVE,     URHO3D_HANDLER(MyApp, HandleWindowDragMove));
        SubscribeToEvent(slider,      E_SLIDERCHANGED,URHO3D_HANDLER(MyApp, HandleSliderChanged));
        SubscribeToEvent(buttonClose, E_RELEASED,     URHO3D_HANDLER(MyApp, HandleClosePressed));

        /// Subscribe to anonymous event (any sender)
        SubscribeToEvent(             E_UIMOUSECLICK, URHO3D_HANDLER(MyApp, HandleControlClicked));

        /// Set the Cursor visibility to be the same as that of the GUI Window
        GetSubsystem<Input>()->SetMouseVisible(window_->IsVisible());

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

        Variant v=gameScene_->GetVar("Character Node");
        if(v.GetType()!=VAR_NONE){
            unsigned ID = v.GetUInt();
            Node* node = gameScene_->GetNode( ID );
            characterNode_  = node;

        }
        else
            characterNode_  = gameScene_->GetChild("Character");


        /// Extract initial pitch and yaw values from camera's current orientation
        Vector3 eulers = cameraNode_->GetWorldRotation().EulerAngles();
        yaw_   = eulers.y_;
        pitch_ = eulers.x_;

        /// Invert pitch for chase behaviour - the rotation logic is flipped
        if(cameraBehaviour_==Chase)
            pitch_=-pitch_;

        /// Access our camera component
        auto* camera = cameraNode_->GetOrCreateComponent<Camera>();

        /// Restore our viewport
        auto* graphics=GetSubsystem<Graphics>();
        WeakPtr<Viewport> viewport (new Viewport(context_, gameScene_, camera));
        GetSubsystem<Renderer>()->SetViewport(0, viewport);

        /// Restore DebugRenderer
        debugDraw_ = gameScene_->GetOrCreateComponent<DebugRenderer>();
    }

    /// Calculate signed angle between two vectors - needed for AI steering behaviours!
    float SignedAngle(Vector3 from, Vector3 to, Vector3 upVector)
    {
        float unsignedAngle = from.Angle(to);
        float sign = upVector.DotProduct(from.CrossProduct(to));
        if(sign<0)
            unsignedAngle = -unsignedAngle;
        return unsignedAngle;
    }

    /// Proposed addition to Vector3 class
    /*
    float Vector3::SignedAngle(Vector3 to, Vector3 upVector)
    {
        float unsignedAngle = Angle(to);
        float sign = upVector.DotProduct(CrossProduct(to));
        if(sign<0)
            unsignedAngle = -unsignedAngle;
        return unsignedAngle;
    }
    */


    /// Implements "character chase" camera behaviour
    void MoveCharacter(float timeStep){
        auto* input = GetSubsystem<Input>();

        const float CAMERA_DISTANCE = 10.0f;

        const float MOVE_SPEED = 18.0f;

        /// Mouse sensitivity (degrees per pixel, scaled to match display resolution)
        const float MOUSE_SENSITIVITY = 0.1f * (768.0f / GetSubsystem<Graphics>()->GetHeight());

        Vector3 currentpos = cameraNode_->GetWorldPosition();
        Vector3 newpos = currentpos;
        Vector3 targetPos=characterNode_->GetWorldPosition();

        /// Convert mouse movement into change in camera pitch and yaw
        if(!window_->IsVisible())
        {
            IntVector2 mouseMove = input->GetMouseMove();
            float yawDelta=MOUSE_SENSITIVITY * mouseMove.x_;
            yawDelta = Clamp(yawDelta, -5.0f, +5.0f);
            yaw_   += yawDelta;
            pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
            pitch_ = Clamp(pitch_, -60.0f, 0.0f);

            /// Compute a new position for camera
            Vector3 dir;
            if(!input->GetMouseButtonDown(MOUSEB_MIDDLE))
                dir= Quaternion(pitch_,yaw_+180.0f,0.0f) * Vector3::FORWARD ;
            else
                dir= Quaternion(oldPitch_,oldYaw_+180.0f,0.0f) * Vector3::FORWARD ;

            const BoundingBox& box = characterNode_->GetDerivedComponent<Drawable>()->GetBoundingBox();
            float effectiveDistance = (box.max_ - box.min_).Length() * 10.0f;//+ CAMERA_DISTANCE;
            newpos = targetPos + dir * effectiveDistance;
        }

        /// Transitioning from FreeLook to Chase camera position...
        /// We'll interpolate the camera position from where it started, to where we want it to be.
        if(isCameraLerping)
        {
            /// Compute how far the camera still needs to move
            float distance = (newpos-currentpos).Length();

            /// Note how much time has passed, in seconds, since we started lerping
            lerpAccum_ += timeStep;

            /// We're looking for an interpolation factor, from 0.0 to 1.0
            /// T = accumulatedTime / transitionTotalTime
            float T = lerpAccum_ / 0.8f; /// bad form to use a divide where a multiply could be used

            /// If the remaining distance to travel is very small,
            /// or if our interpolation factor is larger than 1.0
            if(distance < 0.1f || T>=1.0f)
            {
                /// Camera has reached its destination - stop lerping
                T=1.0f;
                isCameraLerping=false;
            }
            /// Apply interpolated camera position
            cameraNode_->SetWorldPosition( cameraLerpStartPos_.Lerp( newpos, T));
            /// Orient camera to look at the player character
            cameraNode_->LookAt(targetPos);
        }
        /// Camera is not "transitioning", its fully in "chase mode" now...
        else
        {
                /// Apply new camera position
                cameraNode_->SetWorldPosition( newpos );
                /// Look at our target (player character)
                cameraNode_->LookAt(targetPos);

            if(input->GetMouseButtonDown(MOUSEB_MIDDLE)){
                // Apply rotation to character
                Quaternion q(0.0f, yaw_, 0.0f);
                characterNode_->SetWorldRotation( q );
            }


            if(input->GetMouseButtonDown(MOUSEB_RIGHT))
            {
                /// Player character assumes same Y orientation as Camera
                /// but does so at a fixed rate of rotation (aka angular velocity)...
                /// Since our start / end orientations can change any time,
                /// we'll need to measure the angle between them
                /// in order to achieve a fixed rate of rotation!

                // Get the current facing direction (character local Z axis, in worldspace)
                Vector3 currentDir =  characterNode_->GetWorldDirection();
                // Get the desired new direction (camera local Z axis, in worldspace)
                Vector3 newDir = Quaternion(0.0f, cameraNode_->GetWorldRotation().YawAngle(), 0.0f) * Vector3::FORWARD;
                newDir = newDir.Normalized();

                // Compute the angle between current and new directions
                // Set the rate of rotation: 5 degrees per second
                float angle = SignedAngle(currentDir, newDir, Vector3::UP);
                angle *= 5 * timeStep;

                // Apply rotation to character
                characterNode_->Rotate( Quaternion(angle, Vector3::UP), TS_WORLD);
            }
        }

        /// Watch the WASD and LEFT-SHIFT Keys

        bool camRelative =!input->GetKeyDown(KEY_SHIFT);

        if (input->GetKeyDown(KEY_W)){
            if(camRelative)
                characterNode_->Translate( Quaternion(0.0f, cameraNode_->GetWorldRotation().YawAngle(), 0.0f) * Vector3::FORWARD * MOVE_SPEED * timeStep, TS_WORLD);
            else
                characterNode_->Translate( Vector3::FORWARD * MOVE_SPEED * timeStep);
        }

        if (input->GetKeyDown(KEY_S)){
            if(camRelative)
                characterNode_->Translate( Quaternion(0.0f, cameraNode_->GetWorldRotation().YawAngle(), 0.0f) * Vector3::BACK * MOVE_SPEED * timeStep, TS_WORLD);
            else
                characterNode_->Translate( Vector3::BACK * MOVE_SPEED * timeStep);
        }

        if (input->GetKeyDown(KEY_A)){
            if(camRelative)
                characterNode_->Translate( Quaternion(0.0f, cameraNode_->GetWorldRotation().YawAngle(), 0.0f) * Vector3::LEFT * MOVE_SPEED * timeStep, TS_WORLD);
            else
                characterNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
        }

        if (input->GetKeyDown(KEY_D)){
            if(camRelative)
                characterNode_->Translate( Quaternion(0.0f, cameraNode_->GetWorldRotation().YawAngle(), 0.0f) * Vector3::RIGHT * MOVE_SPEED * timeStep, TS_WORLD);
            else
                characterNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
        }
    }

    /// Implements "free-look" camera behaviour
    /// Use WASD to move the camera, and mouse to look around.
    void MoveCamera(float timeStep){

        /// Sanity Check!
        if(!cameraNode_)
            return;

        auto* input = GetSubsystem<Input>();

        /// If system cursor is visible, or app window loses input focus, just get outta here
       // if(input->IsMouseVisible() || !input->HasFocus())
       //     return;

        // Do not move if the UI has a focused element (the console)
        auto* ui = GetSubsystem<UI>();
        if (ui->GetFocusElement())
            return;

        /// Movement speed (world units per second)
        const float MOVE_SPEED = 18.0f;

        if(!window_->IsVisible())
        {

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
        }

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


    /// Utility Method: Instantiate a Box in our Scene
    Node* CreateBox(ResourceCache* cache, const String& NodeName, const Vector3& Position, const Quaternion& Rotation, const Vector3& Scale ){
        Node* boxNode = gameScene_->CreateChild(NodeName);
        boxNode->SetPosition(Position);
        boxNode->SetRotation(Rotation);
        boxNode->SetScale(Scale);
        auto* object = boxNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/MyFirstMaterial.xml"));

        if(Scale.x_>3.5f)
            object->SetOccluder(true);

        return boxNode;
    }

    /// Create and initialize a Camera node and component
    Node* CreateCamera(const Vector3& Position){

        Node* node = gameScene_->CreateChild("Camera Node");
        node->SetWorldPosition(Position);           /// Move camera to initial position

        /// We'll create the Camera component itself as a child of our "Camera Node"
        /// The node provides us with a "transform" to position and rotate the camera,
        /// while the camera component contains the camera-specific code and data.
        Camera* camera = node->CreateComponent<Camera>();

        /// We'll set the camera's far clip plane to something reasonable for our scene
        /// This effectively sets the limit for the max. viewing distance that the camera can "see"
        camera->SetFarClip(100.0f);

        /// Create and Setup a Viewport - effectively associating a camera with the scene it will render
        /// We need WeakPtr here to safely hand over ownership of this special object to Urho
        /// Even though we used "new", we don't own this object - this is an exception to the rule!
        SharedPtr<Viewport> viewport (new Viewport(context_, gameScene_, camera));
        /// Hand (ownership of) the viewport to the Rendering system
        GetSubsystem<Renderer>()->SetViewport(0, viewport);

        return node;

    }

    /// Create and initialize a Zone node and component
    Node* CreateZone(const String& Name, float fBoxHalfSize ){
        Node* zoneNode = gameScene_->CreateChild(Name);
        auto* zone = zoneNode->CreateComponent<Zone>();
        /// Set Ambient Light (RGB) to soft white, almost black
        zone->SetAmbientColor(Color(0.25f, 0.25f, 0.25f));
        /// Set up Fog
        zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
        zone->SetFogStart(80.0f);
        zone->SetFogEnd(100.0f);
        /// Provide the extents (or size) of the Zone BoundingBox
        zone->SetBoundingBox(BoundingBox(-fBoxHalfSize, fBoxHalfSize));
        return zoneNode;
    }

    /// Populate a (presumably) empty scene
    void PopulateGameScene(){

        /// The first component our new scene needs is an Octree.
        gameScene_->CreateComponent<Octree>();

        /// Add a debug drawer
        debugDraw_ = gameScene_->CreateComponent<DebugRenderer>();

        editor_ = gameScene_->CreateComponent<InGameEditor>();

        /// Next we'll create a Camera for rendering a 3D scene ...
        cameraNode_ = CreateCamera( Vector3(20,20,-20));

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
        Node* zoneNode = CreateZone("My Zone", 50.0f);

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

        /// Create a Box
        CreateBox(cache, "Box1",Vector3(0.0f, 1.0f, 0.0f), Quaternion(0.0f,45.0f,0.0f), Vector3::ONE);

        /// Create a bunch of Boxes
        for(int i=2;i<64;i++)
        {
            float HalfSize = Random(0.25f, 3.0f);
            CreateBox(cache, "Box"+String(i),Vector3(Random(-50,+50), HalfSize+0.5f, Random(-50,+50)),Quaternion(0.0f,Random(360),0.0f), Vector3(HalfSize*2,HalfSize*2,HalfSize*2));
        }

        /// Create one more box, this one will act as our "character"
        characterNode_ = CreateBox(cache, "Character",Vector3(5.0f, 1.0f, 5.0f),Quaternion(0.0f,45.0f,0.0f), Vector3::ONE);
    }

    /// Populate a custom user interface via hardcode
    void PopulateUI(){

        /// Note about ownership of UI elements...
        /// We create them with "new", but we hand over ownership via AddChild
        /// This method effectively passes ownership, and responsibility for deletion, to Urho
        /// so we don't need to worry about deleting attached UI elements.

        // Create a GUI Window and add it to the UI's root node
        window_ = new Window(context_);
        uiRoot_->AddChild(window_);

        // Set Window size and layout settings
        window_->SetMinWidth(384);
        window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
        window_->SetAlignment(HA_CENTER, VA_CENTER);
        window_->SetName("Window");

        /// Register to receive notification that user is dragging our GUI window
        SubscribeToEvent(window_, E_DRAGMOVE, URHO3D_HANDLER(MyApp, HandleWindowDragMove));


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

        // Create a "Panel"
        auto* panel = new BorderImage(context_);
        panel->SetMinSize(0, 6);
        panel->SetHorizontalAlignment(HA_LEFT);
        panel->SetLayoutMode(LM_HORIZONTAL);
        panel->SetStyle("EditorMenuBar");

        // Create a Button
        auto* button = new Button(context_);
        button->SetName("Button1");
        button->SetMinHeight(24);
        button->SetStyle("Button");

        // Create a Text label for our Button
        auto* blabel=new Text(context_);
        blabel->SetText("test1");
        blabel->SetAlignment(HA_CENTER,VA_CENTER);
        blabel->SetStyleAuto();
        button->AddChild(blabel);   // Add text label to button

        // Create another Button and Label it
        auto* button2 = new Button(context_);
        button2->SetName("Button2");
        button2->SetMinHeight(24);
        button2->SetStyle("Button");

        blabel=new Text(context_);
        blabel->SetText("test2");
        blabel->SetAlignment(HA_CENTER,VA_CENTER);
        blabel->SetStyleAuto();
        button2->AddChild(blabel);

        // Add both Buttons to our Panel
        panel->AddChild(button);
        panel->AddChild(button2);

        // Add our Panel to our Window
        window_->AddChild(panel);

        // Apply styles
        window_    ->SetStyleAuto();
        windowTitle->SetStyleAuto();
        buttonClose->SetStyle("CloseButton");

        /// Subscribe to buttonClose release (following a 'press') events
        /// NOTE!!!
        /// This is a request to receive "FROM A SPECIFIC SENDER" (buttonClose) "A SPECIFIC EVENT" (E_RELEASED)
        SubscribeToEvent(buttonClose, E_RELEASED,     URHO3D_HANDLER(MyApp, HandleClosePressed));

        /// Subscribe also to all UI mouse clicks just to see where we have clicked
        /// NOTE!!!
        /// This is a request to receive "FROM ANY SENDER" a specific event...
        SubscribeToEvent(             E_UIMOUSECLICK, URHO3D_HANDLER(MyApp, HandleControlClicked));

        /// Populate our UI Window:

        // Create a CheckBox
        auto* checkBox = new CheckBox(context_);
        checkBox->SetName("CheckBox");



        // Create a LineEdit
        auto* lineEdit = new LineEdit(context_);
        lineEdit->SetName("LineEdit");
        lineEdit->SetMinHeight(24);

        auto* slider=new Slider(context_);
        slider->SetName("Slider");
        slider->SetMinHeight(24);
        slider->SetRange(100.0f);
        SubscribeToEvent(slider, E_SLIDERCHANGED,     URHO3D_HANDLER(MyApp, HandleSliderChanged));

        // Add controls to Window
        window_->AddChild(checkBox);
        window_->AddChild(lineEdit);
        window_->AddChild(slider);

        /// Individual UI elements can use different stylesheets!
        /// Here we're telling them all to use the default style we set previously.
        checkBox->SetStyleAuto();
        button->SetStyleAuto();
        lineEdit->SetStyleAuto();
        slider->SetStyleAuto();

        /// Hide GUI window (TAB to toggle window visibility)
        window_->SetVisible(false);
        GetSubsystem<Input>()->SetMouseVisible(false);

    }

    /// Cast a Ray into the scene to detect drawable objects
    bool Raycast(float maxDistance, Vector3& hitPos, Vector3& hitNormal, Drawable*& hitDrawable){
        hitDrawable = nullptr;

        auto* ui = GetSubsystem<UI>();
        IntVector2 pos = ui->GetCursorPosition();
        // Check the cursor is visible and there is no UI element in front of the cursor
        if (ui->GetElementAt(pos, true))
            return false;

        auto* graphics = GetSubsystem<Graphics>();
        auto* camera = cameraNode_->GetComponent<Camera>();
        Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());

        // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
        PODVector<RayQueryResult> results;
        RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
        gameScene_->GetComponent<Octree>()->RaycastSingle(query);
        if (results.Size())
        {
            RayQueryResult& result = results[0];
            hitPos = result.position_;
            hitDrawable = result.drawable_;
            hitNormal = result.normal_;
            return true;
        }

    return false;
    }

    /// Frame Update event handler
    void HandleFrameUpdate(StringHash eventType, VariantMap& eventData){

        /// It's possible, but rare, for the frame update to fire too early!
        if(!characterNode_)
            return;

        /// Let's unpack the event data for E_UPDATE (frame update)
        /// The only event data for this event is "frame delta-time", as a float
        using namespace Update;
        float deltaTime = eventData[P_TIMESTEP].GetFloat();

        /// Each frame, we deliberately "forget" the result from previous frame
        candidateDrawable_ = nullptr;

        /// Mouse Pick-Ray: cast a ray into scene
        /// from position of mouse cursor on camera nearplane
        /// and return the first drawable object that ray hits
        Vector3 hitPos, hitNormal;
        Drawable* hitGeom=nullptr;
        bool hit = Raycast(50.0f, hitPos, hitNormal, hitGeom );
        if(hit){
           //URHO3D_LOGINFO(hitGeom->GetTypeName()+" : "+hitGeom->GetNode()->GetName());
           /// Set the "current candidate object" - the object under the cursor right now
           candidateDrawable_ = hitGeom;
           candidateNormal_ = hitNormal;
            /// If left mouse is down, set the "currently selected object" - the object we care to manipulate
            auto* input=GetSubsystem<Input>();
            if(input->GetMouseButtonDown(MOUSEB_LEFT))
                selectedDrawable_ = hitGeom;
        }

        /// Implement desired behaviour: free-look or chase mode
        switch(cameraBehaviour_){

            case FreeLook:
            /// Update our free-look camera behaviour
            MoveCamera(deltaTime);
            break;

            case Chase:
            /// Update our player character / chase camera behaviour
            MoveCharacter(deltaTime);
            break;

            default:
            URHO3D_LOGERROR("Unhandled camera behaviour type");
        }


        /// Let's say we wanted "the box" to rotate at 30 degrees per second, around the World Up Axis
        /// At this rate, it will take 12 seconds to complete one revolution (360 / 30 = 12)
        /// We'll use a Quaternion to represent rotation, and we'll use "angle and axis" to construct it.
        Quaternion myRot(30.0f * deltaTime, Vector3::UP);

        /// Now we apply our rotation to the Node, optionally we provide the desired Transform Space (world, not local).
        if(gameScene_->IsUpdateEnabled())
            gameScene_->GetChild("Box1")->Rotate( myRot, TS_WORLD);


    }

    /// KeyPress event handler
    void HandleKeyDown(StringHash eventType, VariantMap& eventData){
        using namespace KeyDown;
        int key = eventData[P_KEY].GetInt();

        if (key == KEY_ESCAPE)   // Escape key to quit application
            engine_->Exit();

        else if(key==KEY_TAB)    // toggle mouse cursor / gui visibility
        {
            auto* input = GetSubsystem<Input>();
            bool hide = !window_->IsVisible();
            input->SetMouseVisible(hide);
            window_->SetVisible(hide);
        }

        /// SPACE toggles the Camera Behaviour
        else if(key==KEY_SPACE)
        {
            switch(cameraBehaviour_){
                case FreeLook:

                cameraBehaviour_=Chase;
                /// Camera initial yaw angle is taken from the character
                yaw_ = characterNode_->GetRotation().EulerAngles().y_;
                /// We invert the pitch angle as our view logic has flipped
                pitch_ = -pitch_;

                isCameraLerping=true;
                cameraLerpStartPos_=cameraNode_->GetWorldPosition();
                lerpAccum_=0.0f;
                break;

                default:
                cameraBehaviour_=FreeLook;
                /// Extract initial pitch and yaw values from camera's current orientation
                Vector3 eulers = cameraNode_->GetWorldRotation().EulerAngles();
                yaw_   = eulers.y_;
                pitch_ = eulers.x_;
            }
        }



        // TAKE SCREEN SHOT
        else if(key==KEY_BACKSPACE && !window_->IsVisible())
        {
            // Create temporary Urho3D::Image object to receive screenshot
            Image screenshot(context_);
            // Take screen shot
            GetSubsystem<Graphics>()->TakeScreenShot(screenshot);
            // Save image to file
            screenshot.SavePNG("ScreenShot.png");
        }


        else if(key==KEY_F1){
            if(selectedDrawable_)
                characterNode_=selectedDrawable_->GetNode();

        }

        else if(key==KEY_F2)
            gameScene_->SetUpdateEnabled(!gameScene_->IsUpdateEnabled());


        // LOAD SCENE AND UI STATE FROM XML
        else if(key==KEY_F11){

            /// Reload Scene
            LoadSceneFromXML(mySceneFilePath_);

            /// Reload UI
            LoadGUIFromXML(myGUILayoutFilePath_);
        }


        // SAVE SCENE AND UI STATE TO XML FILE
        else if(key==KEY_F12)
        {
            SaveSceneToXML(mySceneFilePath_);
            SaveGUIToXML(myGUILayoutFilePath_);
        }
    }

    float oldYaw_, oldPitch_;
    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData){
        using namespace MouseButtonDown;
        int buttonID = eventData[P_BUTTON].GetInt();
        if(buttonID==MOUSEB_MIDDLE && cameraBehaviour_==Chase)
        {
            /// Preserve yaw and pitch values while manipulating character orientation
            oldYaw_=yaw_;
            oldPitch_=pitch_;
            /// Temporarily assume yaw and pitch from character node
            auto q = characterNode_->GetRotation();
            yaw_   = q.YawAngle();
            pitch_ = q.PitchAngle();
        }

    }

    void HandleMouseButtonUp(StringHash eventType, VariantMap& eventData){
        using namespace MouseButtonUp;
        int buttonID = eventData[P_BUTTON].GetInt();
        if(buttonID==MOUSEB_MIDDLE && cameraBehaviour_==Chase)
        {
            /// Restore yaw and pitch values after manipulating character orientation
            yaw_=oldYaw_;
            pitch_=oldPitch_;
        }

    }

    /// Post-Render event handler (DebugDrawing)
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData){

        if(!characterNode_)
            return;

        /// Obtain access to our "Box1" node
        Node* boxnode = gameScene_->GetChild("Box1");

        /// Declare a variable to hold the world position of some Drawable
        Vector3 drawPos;

        /// Define a (buildtime) Macro to draw the local major axes of some Drawable
        #define DrawMajorAxes(node, length, depthtest) \
        drawPos = node->GetWorldPosition();\
        debugDraw_->AddLine(drawPos, node->LocalToWorld(Vector3::RIGHT   * length), Color(1, 0, 0, 1), depthtest);\
        debugDraw_->AddLine(drawPos, node->LocalToWorld(Vector3::UP      * length), Color(0, 1, 0, 1), depthtest);\
        debugDraw_->AddLine(drawPos, node->LocalToWorld(Vector3::FORWARD * length), Color(0, 0, 1, 1), depthtest)

        /// Draw the major axes of our "Box1", and our current character
        DrawMajorAxes(boxnode,        3.0f, true);
        DrawMajorAxes(characterNode_, 3.0f, true);

        /// Draw ORANGE the World AABB of the object currently underneath the mouse cursor
        /// (unless its our current character...)
        if(candidateDrawable_ && candidateDrawable_->GetNode()!=characterNode_){
            debugDraw_->AddBoundingBox( candidateDrawable_->GetWorldBoundingBox(), Color(1,1,0), true);
            //candidateDrawable_->DrawDebugGeometry(debugDraw_,true);
            DrawMajorAxes(candidateDrawable_->GetNode(),3.0f, true);
        }

        /// Draw DARK GREEN the World AABB of the "currently selected object"
        if(selectedDrawable_){
            //debugDraw_->AddBoundingBox( selectedDrawable_->GetWorldBoundingBox(), Color(0,0.4f,0), true);
            selectedDrawable_->DrawDebugGeometry(debugDraw_,true);
            DrawMajorAxes(selectedDrawable_->GetNode(),3.0f, true);
        }

        // Debug-Draw the Octree
        //gameScene_->GetComponent<Octree>()->DrawDebugGeometry(debugDraw_, true);

        /// Create a Query Ray for MousePicking
        /// This is redundant - we already did this in the Frame Update handler!
        /// We should have cached our query ray...
        auto* input=GetSubsystem<Input>();
        IntVector2 pos=input->GetMousePosition();
        auto* graphics = GetSubsystem<Graphics>();
        auto* camera = cameraNode_->GetComponent<Camera>();
        Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());

        /// Draw a Circle at the MouseCursor's projected world position...
        Vector3 Normal;
        Color color;
        if(candidateDrawable_){
            /// Magenta Circle: Visualize the Surface Normal under the MouseCursor
            Normal = candidateNormal_;
            color=Color::MAGENTA;
        } else {
            /// Cyan Circle: Display a circle that always faces the camera (no candidate drawable under MouseCursor)
            Normal = cameraRay.direction_;
            color = Color::CYAN;
        }
        debugDraw_->AddCircle(cameraRay.origin_+cameraRay.direction_*0.1f, Normal, 0.005f, color, 32,false);



    }

    /// UI Window Close Button Pressed event handler
    void HandleClosePressed(StringHash eventType, VariantMap& eventData){
        if (GetPlatform() != "Web"){
            // Closing the application is certainly an option
            //engine_->Exit();
            /// Hide the GUI window, and the mouse cursor
            window_->SetVisible(false);
            GetSubsystem<Input>()->SetMouseVisible(false);
        }else{
            engine_->Exit();
        }
    }

    /// UI Slider Moved event handler
    void HandleSliderChanged(StringHash eventType, VariantMap& eventData){
        using namespace SliderChanged;

        auto* clicked = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
        float value = eventData[P_VALUE].GetFloat();
        URHO3D_LOGINFO(String(value));
    }

    /// UI Element Clicked event handler
    void HandleControlClicked(StringHash eventType, VariantMap& eventData){

        if(!window_)
            return;

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

        URHO3D_LOGINFO(name);
    }

    /// UI Element DragMove event handler
    void HandleWindowDragMove(StringHash eventType, VariantMap& eventData){
        /// Unpack the 2D position delta
        using namespace DragMove;
        int dx = eventData[P_DX].GetInt();
        int dy = eventData[P_DY].GetInt();
        /// Compute new window position
        IntVector2 pos = window_->GetPosition();
        pos.x_ += dx;
        pos.y_ += dy;
        /// Apply new window position
        window_->SetPosition( pos );
        /// Display new position in window title
        auto* windowTitle = window_->GetChildStaticCast<Text>("WindowTitle", true);
        windowTitle->SetText( String(pos.x_)+", "+String(pos.y_));

   //     URHO3D_LOGINFO(String(dx)+", "+String(dy));

    }

    ///!/////////////////////////////////////////////////////////////////////////////////////////////////////
    ///! Generally, I like to put all the data members together, either at the top or bottom of the class ///
    ///!/////////////////////////////////////////////////////////////////////////////////////////////////////

    /// The default filepath for loading/saving scene content
    String mySceneFilePath_ = "MyGameScene.xml";

    /// The default filepath for loading/saving UI content
    String myGUILayoutFilePath_ = "MyGUI.xml";

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
    WeakPtr<Node> cameraNode_ = nullptr;

    /// Our camera pitch and yaw angles are two of the three "Euler" angles (pitch, yaw, roll)
    /// They describe our camera node's current orientation, in degrees
    float yaw_, pitch_;

    /// More camera-related stuff
    bool isCameraLerping=false;     /// Whether camera is transitioning from freelook to chase mode
    float lerpAccum_;               /// How much time has passed since cam lerping began
    Vector3 cameraLerpStartPos_;    /// Where the cam was in worldspace when lerping began


    /// Root node for our "player character"
    WeakPtr<Node> characterNode_;

    /// Drawable under the mousecursor
    WeakPtr<Drawable> candidateDrawable_;
    /// Drawable currently "selected"
    WeakPtr<Drawable> selectedDrawable_;
    /// SurfaceNormal under the mousecursor
    Vector3 candidateNormal_;

    /// The UI system's root UIElement
    /// We request this object from the UI system.
    /// We don't own it, and we don't include it when we save UI content.
    /// because there seems to be no way to replace root element of UI system after reload
    WeakPtr<UIElement> uiRoot_;

    /// UI Window
    /// It's a kind of UI element, and the only (in our case) child of uiRoot_
    WeakPtr<Window> window_;

    WeakPtr<InGameEditor> editor_;

};

URHO3D_DEFINE_APPLICATION_MAIN(MyApp)
