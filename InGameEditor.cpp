
#define URHO3D_ANGELSCRIPT 1    // include support for AngelScript, not that we're using it yet
#define URHO3D_LOGGING 1        // include support for debug messages, they are very handy
#define URHO3D_NAVIGATION 1
#include <Urho3D/Urho3DAll.h>   // we are too lazy to optimize header inclusion any further


#include "InGameEditor.h"


    void InGameEditor::RegisterObject(Context* context){
        context->RegisterFactory<InGameEditor>();
        URHO3D_ATTRIBUTE("Is Visibible", bool, isVisible, false, AM_DEFAULT );
        URHO3D_ATTRIBUTE("Hierarchy Visibible", bool, HierarchyIsVisible, false, AM_DEFAULT );
        URHO3D_ATTRIBUTE("Inspector Visibible", bool, InspectorIsVisible, false, AM_DEFAULT );
        URHO3D_ATTRIBUTE("Hierarchy Pos", IntVector2, HierarchyPos_,IntVector2::ZERO, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Inspector Pos", IntVector2, InspectorPos_,IntVector2::ZERO, AM_DEFAULT);

        URHO3D_ATTRIBUTE("hideVars", bool, hideVars, true, AM_DEFAULT);
        URHO3D_ATTRIBUTE("hideNodeAttribs", bool, hideNodeAttribs, true, AM_DEFAULT);
        URHO3D_ATTRIBUTE("hideComponentAttribs", bool, hideComponentAttribs, true, AM_DEFAULT);
    }

    InGameEditor::InGameEditor(Context* context):LogicComponent(context){}




    /// Set up UI and event handling
    void InGameEditor::DelayedStart(){



        /// Obtain access to the UI subsystem
        uiRoot_=GetSubsystem<UI>()->GetRoot();

        Scene* scene = GetScene();

       SubscribeToEvent(E_POSTRENDERUPDATE,  URHO3D_HANDLER(InGameEditor,HandlePostRenderUpdate));    // Post-Render Update
       debugDraw_ = scene->GetOrCreateComponent<DebugRenderer>();

               /// Locate our camera node in the reloaded scene
                auto* camnode = scene->GetChild("Camera Node");

                /// Access our camera component
                auto* camera = camnode->GetOrCreateComponent<Camera>();

                /// Restore our viewport
                auto* graphics=GetSubsystem<Graphics>();
                WeakPtr<Viewport> viewport (new Viewport(context_, scene, camera));
                GetSubsystem<Renderer>()->SetViewport(0, viewport);

                /// Restore DebugRenderer
                scene->GetOrCreateComponent<DebugRenderer>();

                /// Retrieve custom data we saved earlier
                cameraBehaviour_ = (CameraBehaviour)scene->GetVar("Camera Behaviour").GetUInt();


                        /// Create a control node for our character



                Variant v=scene->GetVar("Character Node");
                if(v.GetType()!=VAR_NONE){
                    unsigned ID = v.GetUInt();
                    Node* node = scene->GetNode( ID );
                    characterNode_  = node;
                }
                else
                {
                    characterNode_ = scene->GetChild("Character",true);
                    if(!characterNode_)
                        characterNode_ = scene->CreateChild("Character");
                }

        /// When a scene containing InGameEditor is reloaded,
        /// the existing UI system is not destroyed
        /// however any ui event subscriptions have become invalid!

        /// Locate or Create our Main Menu
        MainMenu_=uiRoot_->GetChild("Main Menu",false);
        if(MainMenu_==nullptr){
            MainMenu_ = new UIElement(context_);
            MainMenu_->SetName("Main Menu");
            MainMenu_->SetAlignment(HA_LEFT, VA_TOP);
            MainMenu_->SetLayout(LM_HORIZONTAL, 28);
            MainMenu_->SetMinWidth(128);
            uiRoot_->AddChild(MainMenu_);

            CreateMainMenuItem("Project", {"New","Load","Save"},"bleh");
            CreateMainMenuItem("Scene",   {"New Scene","Load Scene","Save Scene"},"blehh");
            CreateMainMenuItem("Tools",   {"Hierarchy","Inspector", "Transform","NavMesh"},"blehh");
            CreateMainMenuItem("Prefab",  {"Load Prefab","Save Prefab"},"meh");
        }

        /// Subscribe to receive UI events for our main menu dropdown lists
        /// Note we query the UI for each DropDownList element
        auto* list = MainMenu_->GetChild("Project",true)->GetChild("DropDownList",true);
        SubscribeToEvent(list, "ItemSelected", URHO3D_HANDLER(InGameEditor, HandleUIDropDownItemSelected));

        list = MainMenu_->GetChild("Scene",true)->GetChild("DropDownList",true);
        SubscribeToEvent(list, "ItemSelected", URHO3D_HANDLER(InGameEditor, HandleUIDropDownItemSelected));

        list = MainMenu_->GetChild("Tools",true)->GetChild("DropDownList",true);
        SubscribeToEvent(list, "ItemSelected", URHO3D_HANDLER(InGameEditor, HandleUIDropDownItemSelected));

        list = MainMenu_->GetChild("Prefab",true)->GetChild("DropDownList",true);
        SubscribeToEvent(list, "ItemSelected", URHO3D_HANDLER(InGameEditor, HandleUIDropDownItemSelected));

        /// Set visibility of main menu
        MainMenu_->SetVisible(isVisible);

        /////////////////////////////////////////////////////////////////////


        /// Create the Scene Hierarchy Editor window
        CreateHierarchyWindow();

        /// Populate the ListView with existing scene nodes
        RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetNode()->GetScene());

        /// Subscribe to receive notification of "TreeView Element Clicked"
        list = HierarchyWindow_->GetChild("ListContent",true);
        SubscribeToEvent(list, E_ITEMCLICKED, URHO3D_HANDLER(InGameEditor, HandleListViewItemClicked));

        auto* btn = HierarchyWindow_->GetChild("CloseButton",true);
        SubscribeToEvent(btn, E_RELEASED,     URHO3D_HANDLER(InGameEditor, HandleClosePressed));

        /// Register to receive notification that user is dragging our GUI window
        SubscribeToEvent(HierarchyWindow_, E_DRAGMOVE, URHO3D_HANDLER(InGameEditor, HandleWindowDragMove));

        HierarchyPos_ = HierarchyWindow_->GetPosition();

        HierarchyWindow_->SetVisible(HierarchyIsVisible);

        ////////////////////////////

        /// Locate or Create the Inspector Window
        InspectorWindow_=(Window*)uiRoot_->GetChild("Inspector",true);
        if(!InspectorWindow_){
            InspectorWindow_=CreateWindow("Inspector", "Inspector");
            /// Initialize window position attribute
            InspectorPos_=InspectorWindow_->GetPosition();
        }else{
            /// We probably don't need this - its a serialized attribute?
            InspectorWindow_->SetPosition(InspectorPos_);
            auto* btn = InspectorWindow_->GetChild("CloseButton",true);
            SubscribeToEvent(btn, E_RELEASED,     URHO3D_HANDLER(InGameEditor, HandleClosePressed));


        }

        /// Register to receive notification that user is dragging our GUI window
        SubscribeToEvent(InspectorWindow_, E_DRAGMOVE, URHO3D_HANDLER(InGameEditor, HandleWindowDragMove));

        /// Populating the Inspector will ensure all relevant UI events are hooked up
        RebuildInspector();
        InspectorWindow_->SetVisible(InspectorIsVisible);


        /////////////////////////////

        /// Obtain access to camera node in the current scene
        EditorCameraNode_=GetNode()->GetScene()->GetChild("Camera Node");

        /// Extract camera's current pitch and yaw angles
        Quaternion rot = EditorCameraNode_->GetWorldRotation();
        pitch_ = rot.PitchAngle();
        yaw_   = rot.YawAngle();

        //////////////////////////////

        /// Subscribe to receive notification of user inputs
        SubscribeToEvent(E_KEYDOWN,           URHO3D_HANDLER(InGameEditor, HandleKeyDown));
        SubscribeToEvent(E_MOUSEBUTTONDOWN,   URHO3D_HANDLER(InGameEditor, HandleMouseButtonDown));     // MouseButton Down
        SubscribeToEvent(E_MOUSEBUTTONUP,     URHO3D_HANDLER(InGameEditor, HandleMouseButtonUp));       // MouseButton Up



        ///////////////////////////////////////////


        const HashMap< String, Vector< StringHash > > & categories = context_->GetObjectCategories();

        const HashMap<StringHash, SharedPtr<ObjectFactory>> factories = context_->GetObjectFactories();
        for(auto it=factories.Begin();it!=factories.End();it++){

            String name = it->second_->GetTypeName();
            const TypeInfo* type = it->second_->GetTypeInfo();
            if(type->IsTypeOf<Component>()){

                bool found=false;
                for(auto itt=categories.Begin();itt!=categories.End();itt++){
                    if(itt->second_.Contains(name)){
 //                       URHO3D_LOGINFO("Found Component: "+name+" in category: "+itt->first_);
                        found=true;
                        componentMap_[itt->first_].Push(name);
                        break;
                    }
                }

                if(!found){
//                    URHO3D_LOGINFO("Found Component: "+name+" (no category)");
                    componentMap_["None"].Push(name);
                }

                int x=0;
            }else{
                bool found=false;
                for(auto itt=categories.Begin();itt!=categories.End();itt++){
                    if(itt->second_.Contains(name)){
                        URHO3D_LOGINFO("Found NONComponent: "+name+" in category: "+itt->first_);
                        found=true;
                    //    componentMap_[itt->first_].Push(name);
                        break;
                    }
                }

                if(!found){
                    URHO3D_LOGINFO("Found NONComponent: "+name+" (no category)");
                  //  componentMap_["None"].Push(name);
                }

            }
        }

        for(auto it=componentMap_.Begin();it!=componentMap_.End();it++){

            const String& cat = it->first_;
            for(auto itt=it->second_.Begin(); itt!=it->second_.End();itt++)
                URHO3D_LOGINFO("Component "+ *itt + " in category: "+cat);


        }

    }


    /// When the menu is hidden, we can move the camera
    void InGameEditor::Update(float dT){
        if(!MainMenu_->IsVisible())
            MoveCamera(dT);

        /// Each frame, we deliberately "forget" the result from previous frame
        candidateDrawable_ = nullptr;

        /// TODO: Move this code into a mousebutton event handler

        /// Mouse Pick-Ray: cast a ray into scene
        /// from position of mouse cursor on camera nearplane
        /// and return the first drawable object that ray hits
        Vector3 hitPos, hitNormal;
        Drawable* hitGeom=nullptr;

        auto* graphics = GetSubsystem<Graphics>();
        auto* camera = EditorCameraNode_->GetComponent<Camera>();
        auto* ui = GetSubsystem<UI>();

        IntVector2 pos = ui->GetCursorPosition();
        // Check the cursor is visible and there is no UI element in front of the cursor
        if (ui->GetElementAt(pos, true))
            return;

        Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());


        bool hit = Raycast(cameraRay, 50.0f, hitPos, hitNormal, hitGeom );
        if(hit){
           //URHO3D_LOGINFO(hitGeom->GetTypeName()+" : "+hitGeom->GetNode()->GetName());
           /// Set the "current candidate object" - the object under the cursor right now
           candidateDrawable_ = hitGeom;
           candidateNormal_ = hitNormal;

            /// If left mouse is down, set the "currently selected object" - the object we care to manipulate
            auto* input=GetSubsystem<Input>();
            if(input->GetMouseButtonDown(MOUSEB_LEFT)){
                selectedDrawable_ = hitGeom;
                selectedComponent_=selectedDrawable_;
                selectedNode_=selectedComponent_->GetNode();
                RebuildInspector();
                RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetScene());
            }
        }

        if(selectedDrawable_){

                const BoundingBox& bb = selectedDrawable_->GetWorldBoundingBox();
                Sphere sphere(bb);

                float r = sphere.radius_;

                float d = cameraRay.HitDistance(sphere);

                Vector3 hitpos = cameraRay.origin_ + cameraRay.direction_ * d;

                float dd = hitpos.DistanceToPoint(cameraRay.origin_);

                if(dd == sphere.radius_){
                    debugDraw_->AddCircle(hitpos, (hitpos - bb.Center() ).Normalized(),0.1f,Color::RED, 24);
                    debugDraw_->AddLine(hitpos, bb.Center(), Color::MAGENTA, false);
                }
        }

    }


    /// Utility: Create a UI Vertical Divider Element
    void InGameEditor::AddVerticalDivider(UIElement* container, const String& text){
        Button* btn=AddButton(container,text,Color::RED);
        btn->SetName(text);
        btn->SetMinHeight(24);
        SubscribeToEvent(btn, E_CLICK, URHO3D_HANDLER(InGameEditor, HandleCollapsingSection));
        BorderImage* bi=new BorderImage(context_);
        bi->SetStyle("EditorDivider");
        bi->SetMinHeight(24);
        container->AddChild(bi);
    }

    /// Utility: Create a UI LineEdit Element
    LineEdit* InGameEditor::AddLineEdit(UIElement* container, const String& Name, const String& text){
        auto* lineEdit = new LineEdit(context_);
        lineEdit->SetName(Name);
        lineEdit->SetStyleAuto();
 //       bool res=lineEdit->GetTextElement()->SetFont(GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Ubuntu-Bold.ttf"));
        lineEdit->SetMinHeight(24);
        lineEdit->SetText(text);
        lineEdit->SetCursorPosition(0); /// If we don't do this, our default text won't be visible :(
        container->AddChild(lineEdit);
        SubscribeToEvent( E_TEXTFINISHED, URHO3D_HANDLER(InGameEditor, HandleTextEditFinished));
        return lineEdit;
    }

    /// Utility: Create a UI LineEdit Element
    LineEdit* InGameEditor::InsertLineEdit(DropDownList* container, const String& Name, const String& text){
        auto* lineEdit = new LineEdit(context_);
        lineEdit->SetName(Name);
        lineEdit->SetStyle("LineEdit");
        lineEdit->SetMinHeight(24);
        lineEdit->SetText(text);
        lineEdit->SetCursorPosition(0); /// If we don't do this, our default text won't be visible :(
        lineEdit->SetInternal(true);
        container->AddItem(lineEdit);
        SubscribeToEvent(lineEdit, E_TEXTFINISHED, URHO3D_HANDLER(InGameEditor, HandleTextEditFinished));
        return lineEdit;
    }

    /// Utility: Create a UI Text Element
    /// container = Parent Element
    /// text      = plaintext input string
    /// color     = colour for the text element
    Text* InGameEditor::AddText(  UIElement* container, const String& text, const Color& color, TextEffect effect, String style) {
        Text* blabel=new Text(context_);
        if(style=="")
            blabel->SetStyleAuto();
        else
            blabel->SetStyle(style);
        blabel->SetText(text);
        blabel->SetVerticalAlignment(VA_CENTER);
        container->AddChild(blabel);
        /// Set color AFTER adding Text element to parent
        blabel->SetColor(color);
        return blabel;
    }

    Text* InGameEditor::AddTextItem(  DropDownList* container, const String& text, const Color& color, TextEffect effect, String style) {
        Text* blabel=new Text(context_);
        if(style=="")
            blabel->SetStyleAuto();
        else
            blabel->SetStyle(style);
        blabel->SetText(text);
        blabel->SetVerticalAlignment(VA_CENTER);
        container->AddItem(blabel);
        /// Set color AFTER adding Text element to parent
        blabel->SetColor(color);
        return blabel;
    }

    /// Utility: Create a UI Button Element
    Button* InGameEditor::AddButton(UIElement* container, const String& text, const Color& TextColor){
        Button* btn=new Button(context_);
        btn->SetStyle("Button");
        btn->SetAlignment(HA_CENTER, VA_CENTER);

        /// Add Button to parent BEFORE adding child Text element
        container->AddChild(btn);
        /// Add child Text element
        Text* t = AddText(btn, text, TextColor);
        btn->SetMinWidth(t->GetWidth());
        btn->SetMinHeight(t->GetHeight());
        return btn;
    }

    /// Utility: Create a container UI Element
    /// representing a Horizontal Row of Elements
    UIElement* InGameEditor::AddRow(UIElement* container){
        UIElement* row = new UIElement(context_);
        row->SetLayout(LM_HORIZONTAL);
        row->SetMinHeight(20);
        row->SetStyleAuto();
        container->AddChild(row);
        return row;
    }

    DropDownList* InGameEditor::AddDropDownList(UIElement* container){
        /// Create a dropdown list for the new main menu item
        DropDownList* list = new DropDownList(context_);
        container->AddChild(list);
        list->SetStyleAuto();
        list->SetMinHeight(32);

        return list;
    }

    /// Utility: Create a UI ListView element, in "hierarchy mode"... AKA TREE VIEW
    ListView* InGameEditor::CreateTreeView_FixedSize(UIElement* container, const String& Name, int width, int height ){
        /// Create UI ListView Element in "Hierarchy Mode" (AKA TreeView)
        ListView* meh=new ListView(context_);
        meh->SetName(Name);
        meh->SetHierarchyMode(true);
        meh->SetFixedHeight(width);
        meh->SetFixedWidth(height);
        /// Set default style for content elements
        meh->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
        /// Set style for ListView element
        meh->SetStyle("HierarchyListView");
        /// Set hover behaviour
        meh->SetHighlightMode(HM_FOCUS);

        container->AddChild(meh);

        return meh;

    }

    /// Utility: Create a UI MainMenu Dropdown List
    DropDownList* InGameEditor::CreateMainMenuItem(String label, Vector<String> items, String handler) {


        /// Create a cell container for the new main menu item
        auto* subcontainer = new UIElement(context_);
        subcontainer->SetName(label);
        subcontainer->SetAlignment(HA_CENTER, VA_TOP);
        subcontainer->SetLayout(LM_VERTICAL, 2);
        subcontainer->SetMinHeight(32);
        subcontainer->SetMinWidth(128);
        MainMenu_->AddChild(subcontainer);

        /// Create a text label for the new main menu item
        Text* t4 = AddText(subcontainer,label,Color::WHITE,TE_NONE,"FileSelectorListText");
        t4->SetStyle("FileSelectorListText");  /// Provides selection highlighting


        /// Create a dropdown list for the new main menu item
        DropDownList* list = new DropDownList(context_);
        list->SetName("DropDownList");
        subcontainer->AddChild(list);
        list->SetStyleAuto();
        list->SetMinHeight(32);




        /// Populate the dropdown list
        for (int i = 0; i < items.Size(); ++i)
        {
            Text* t = new Text(context_);
            list->AddItem(t);
            t->SetText( items[i] );
            t->SetStyleAuto();
            t->SetMinWidth(  t->GetRowWidth(0) + 10 );
            t->SetHoverColor(Color(0.2f,0.5f,0.2f, 0.9f));

        }

//        text->SetMaxWidth( text->GetRowWidth(0) );

        /// Subscribe to receive notification of "user selected an item in a dropdown list"
        return list;
    }

    /// Utility: Create a Moveable UI Window
    /// populated with a TitleBar, Close Button, and Content Panel.
    Window* InGameEditor::CreateWindow(const String& Name, const String& Title){

        Window* window = new Window(context_);
        uiRoot_->AddChild(window);

        // Set Window size and layout settings
        window->SetMinWidth(384);
        window->SetMinHeight(24);
        window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
        window->SetAlignment(HA_CENTER, VA_CENTER);
        window->SetName(Name);
        window->SetStyleAuto();

        /// Register to receive notification that user is dragging our GUI window
        SubscribeToEvent(window, E_DRAGMOVE, URHO3D_HANDLER(InGameEditor, HandleWindowDragMove));


        // Create Window 'titlebar' container
        auto* titleBar = new UIElement(context_);
        titleBar->SetMinSize(0, 24);
        titleBar->SetVerticalAlignment(VA_TOP);
        titleBar->SetLayoutMode(LM_HORIZONTAL);
        titleBar->SetFixedHeight(24);
        window->AddChild(titleBar);

        BorderImage* bi=new BorderImage(context_);
        bi->SetStyle("EditorDivider");
        window->AddChild(bi);

        // Create the Window title Text
        auto* windowTitle = new Text(context_);
        windowTitle->SetName("WindowTitle");
        windowTitle->SetText(Title);
        windowTitle->SetStyleAuto();

        // Create the Window's close button
        auto* buttonClose = new Button(context_);
        buttonClose->SetName("CloseButton");
        buttonClose->SetStyle("CloseButton");
        SubscribeToEvent(buttonClose, E_RELEASED,     URHO3D_HANDLER(InGameEditor, HandleClosePressed));


        // Add the controls to the title bar
        titleBar->AddChild(windowTitle);
        titleBar->AddChild(buttonClose);

        // Add the title bar to the Window

        // Create a "Panel"
        auto* panel = new BorderImage(context_);
        panel->SetMinSize(0, 6);
        panel->SetHorizontalAlignment(HA_LEFT);
        panel->SetLayoutMode(LM_VERTICAL);
        panel->SetStyle("EditorMenuBar");
        panel->SetName("Panel");

        // Add our Panel to our Window
        window->AddChild(panel);

        window->SetVisible(isVisible);

        return window;
    }


    void InGameEditor::CreateContextMenu(Node* parent){

        if(nodeContextWindow_)
            nodeContextWindow_->Remove();
        nodeContextWindow_ = CreateWindow("NodeContext","Node Options:");
        UIElement* panel = nodeContextWindow_->GetChild("Panel",true);

        panel->SetVar("ContextParentNode",parent);

        String nodename = parent->GetName();
        if(nodename=="")
            nodename="Nameless, ID="+String(parent->GetID());
        Button* btn = AddButton(panel,"Delete Node ("+nodename+")" );
        btn->SetName("DeleteNode");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

        btn = AddButton(panel,"Create Node (local)");
        btn->SetName("LocalNode");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

        btn = AddButton(panel,"Create Node (networked)");
        btn->SetName("NetworkNode");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

        btn = AddButton(panel,"Create Component (local)");
        btn->SetName("LocalComponent");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

        btn = AddButton(panel,"Create Component (replicated)");
        btn->SetName("NetworkComponent");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

    }

    void InGameEditor::CreateContextMenu(Component* comp){
        if(nodeContextWindow_)
            nodeContextWindow_->Remove();

        nodeContextWindow_ = CreateWindow("NodeContext","Component Options:");
        UIElement* panel = nodeContextWindow_->GetChild("Panel",true);

        panel->SetVar("ContextParentNode",comp->GetNode());
        panel->SetVar("ContextComponent",comp);

        Button* btn = AddButton(panel,"Delete Component ("+comp->GetTypeName()+")");
        btn->SetName("DeleteComponent");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

        btn = AddButton(panel,"Create Component (local)");
        btn->SetName("LocalComponent");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

        btn = AddButton(panel,"Create Component (replicated)");
        btn->SetName("NetworkComponent");
        SubscribeToEvent(btn,E_CLICK,URHO3D_HANDLER(InGameEditor, HandleContextButtonClick));

    }


    void InGameEditor::AddAttribute_Vector2(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
        const Vector2* pos = (Vector2*)info->ptr_;
        LineEdit* ed = AddLineEdit(row,"X", String(pos->x_));
                    ed->SetVar("Type",VAR_VECTOR2);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);

        ed=AddLineEdit(row,"Y", String(pos->y_));
                    ed->SetVar("Type",VAR_VECTOR2);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        }else{

         Variant vpos;
         info->accessor_->Get(source, vpos);
         Vector2 pos = vpos.GetVector2();
         LineEdit* ed = AddLineEdit(row,"X", String(pos.x_));
                    ed->SetVar("Type",VAR_VECTOR2);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Y", String(pos.y_));
                    ed->SetVar("Type",VAR_VECTOR2);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        }
    }

    void InGameEditor::AddAttribute_Vector3(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
        const Vector3* pos = (Vector3*)info->ptr_;
        LineEdit* ed = AddLineEdit(row,"X", String(pos->x_));
                    ed->SetVar("Type",VAR_VECTOR3);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);

        ed=AddLineEdit(row,"Y", String(pos->y_));
                    ed->SetVar("Type",VAR_VECTOR3);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);

        ed = AddLineEdit(row,"Z", String(pos->z_));
                    ed->SetVar("Type",VAR_VECTOR3);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        }else{

         Variant vpos;
         info->accessor_->Get(source, vpos);
         Vector3 pos = vpos.GetVector3();
         LineEdit* ed = AddLineEdit(row,"X", String(pos.x_));
                    ed->SetVar("Type",VAR_VECTOR3);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Y", String(pos.y_));
                    ed->SetVar("Type",VAR_VECTOR3);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Z", String(pos.z_));
                    ed->SetVar("Type",VAR_VECTOR3);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        }
    }

    void InGameEditor::AddAttribute_Vector4(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
        const Vector4* pos = (Vector4*)info->ptr_;
        LineEdit* ed = AddLineEdit(row,"X", String(pos->x_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);

        ed=AddLineEdit(row,"Y", String(pos->y_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);

        ed = AddLineEdit(row,"Z", String(pos->z_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        ed = AddLineEdit(row,"W", String(pos->w_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        }else{

         Variant vpos;
         info->accessor_->Get(source, vpos);
         Vector4 pos = vpos.GetVector4();
         LineEdit* ed = AddLineEdit(row,"X", String(pos.x_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Y", String(pos.y_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Z", String(pos.z_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
         ed = AddLineEdit(row,"W", String(pos.w_));
                    ed->SetVar("Type",VAR_VECTOR4);
                    ed->SetVar("Attribute",info->name_);
                    ed->SetVar("Source",source);
        }
    }

    void InGameEditor::AddAttribute_Quaternion(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
        const Quaternion* pos = (Quaternion*)info->ptr_;
        LineEdit* ed = AddLineEdit(row,"X", String(pos->x_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
        ed->SetVar("Source",source);
        ed = AddLineEdit(row,"Y", String(pos->y_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
        ed->SetVar("Source",source);
        ed = AddLineEdit(row,"Z", String(pos->z_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
        ed->SetVar("Source",source);
        ed = AddLineEdit(row,"W", String(pos->w_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
        ed->SetVar("Source",source);
        }else{

         Variant vpos;
         info->accessor_->Get(source, vpos);
         Quaternion pos = vpos.GetQuaternion();
         LineEdit* ed = AddLineEdit(row,"X", String(pos.x_));
         ed->SetVar("Attribute",info->name_);
         ed->SetVar("Type",VAR_QUATERNION);
         ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Y", String(pos.y_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
         ed->SetVar("Source",source);
         ed = AddLineEdit(row,"Z", String(pos.z_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
         ed->SetVar("Source",source);
         ed = AddLineEdit(row,"W", String(pos.w_));
        ed->SetVar("Attribute",info->name_);
        ed->SetVar("Type",VAR_QUATERNION);
         ed->SetVar("Source",source);

        }
    }

    void InGameEditor::AddAttribute_Float(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
            const float* val = (float*)info->ptr_;
            LineEdit* ed = AddLineEdit(row,info->name_, String(*val));
            ed->SetVar("Attribute",info->name_);
            ed->SetVar("Type",VAR_FLOAT);
            ed->SetVar("Source",source);
        }else{

             Variant val;
             info->accessor_->Get(source, val);
             LineEdit* ed = AddLineEdit(row,info->name_, val.ToString());
             ed->SetVar("Attribute",info->name_);
             ed->SetVar("Type",VAR_FLOAT);
             ed->SetVar("Source",source);
        }
    }

    void InGameEditor::AddAttribute_Bool(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
            const bool* val = (bool*)info->ptr_;
            LineEdit* ed = AddLineEdit(row,info->name_, String(*val));
            ed->SetVar("Attribute",info->name_);
            ed->SetVar("Type",VAR_BOOL);
            ed->SetVar("Source",source);
        }else{

             Variant val;
             info->accessor_->Get(source, val);
             LineEdit* ed = AddLineEdit(row, info->name_, String(val.GetBool()) );
             ed->SetVar("Attribute",info->name_);
             ed->SetVar("Type",VAR_BOOL);
             ed->SetVar("Source",source);
        }
    }

    void InGameEditor::AddAttribute_Int(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
            const int* val = (int*)info->ptr_;
            LineEdit* ed = AddLineEdit(row,info->name_, String(*val));
            ed->SetVar("Attribute",info->name_);
            ed->SetVar("Type",VAR_INT);
            ed->SetVar("Source",source);
        }else{

             Variant val;
             info->accessor_->Get(source, val);
             LineEdit* ed = AddLineEdit(row, info->name_, String(val.GetInt()) );
             ed->SetVar("Attribute",info->name_);
             ed->SetVar("Type",VAR_INT);
             ed->SetVar("Source",source);
        }
    }

    void InGameEditor::AddAttribute_String(Serializable* source, UIElement* container, const AttributeInfo* info){
        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(info->ptr_){
            const String* val = (String*)info->ptr_;
            LineEdit* edit = AddLineEdit(row,info->name_, *val);
            edit->SetVar("Attribute",info->name_);
            edit->SetVar("Type",info->type_);
            edit->SetVar("Source",source);
        }else{

             Variant val;
             info->accessor_->Get(source, val);
             LineEdit* edit = AddLineEdit(row,info->name_, val.GetString());
             edit->SetVar("Attribute",info->name_);
             edit->SetVar("Type",info->type_);
             edit->SetVar("Source",source);

        }
    }

    void InGameEditor::AddAttribute_StringVector(Serializable* source, UIElement* container, const AttributeInfo* info){

        const StringVector* strings;

        if(info->ptr_){
            const StringVector* val = (StringVector*)info->ptr_;

        }else{

             Variant val;
             info->accessor_->Get(source, val);

             strings = val.GetStringVectorPtr();


        }

        UIElement* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        if(strings->Empty())
        {
            AddText(row, "(Empty StringVector)");
            return;
        }

        DropDownList* list = new DropDownList(context_);
        list->SetName(info->name_);
        list->SetStyleAuto();
        list->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
        list->SetResizePopup(true); /// Expand popup to width of container


            for(int i=0;i<strings->Size();i++)
            {
                //LineEdit* edit = InsertLineEdit(list, val->At(i), val->At(i));

                Text* text=new Text(context_);
                text->SetStyleAuto();
                text->SetText(strings->At(i));

                list->AddItem(text);

                /*edit->SetVar("Attribute",info->name_);
                edit->SetVar("Type",info->type_);
                edit->SetVar("Source",source);*/
            }


        row->AddChild(list);


    }

    void InGameEditor::AddAttribute_ResourceRef(Serializable* source, UIElement* container, const AttributeInfo* info){

        Variant val;
        info->accessor_->Get(source, val );

        const ResourceRef& ref = val.GetResourceRef();

        auto* row = AddRow(container);
        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);

        AddText(row, ref.name_);

    }

    void InGameEditor::AddAttribute_ResourceRefList(Serializable* source, UIElement* container, const AttributeInfo* info){

        Variant val;
        info->accessor_->Get(source, val );

        const ResourceRefList& refs = val.GetResourceRefList();
        //auto* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");

        auto* row = AddRow(container);

        Button* btn = AddButton(row, info->name_+": ", Color::GREEN);
        //DropDownList* list = AddDropDownList(row);


        ScrollView* sv = new ScrollView(context_);
        sv->SetStyleAuto();
        sv->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
        sv->SetFixedSize(100,200);

        row->AddChild(sv);

        ListView* img = new ListView(context_);
        img->SetStyleAuto();
        img->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
        img->SetLayoutMode(LM_VERTICAL);
        img->SetSize(100,400);

        sv->SetContentElement(img);





        for(int i=0;i<refs.names_.Size();i++){
                auto* text=new Text(context_);
        text->SetName(refs.names_[i]);
        //text->SetFont(font);
        text->SetText(refs.names_[i]);
        text->SetStyleAuto();
        img->AddItem(text);

        }

        //    InsertLineEdit(list,refs.names_[i],refs.names_[i]);
    }

    FileSelector* InGameEditor::CreateFileSelector(const String& title){

            auto* fileSelector_ = new FileSelector(context_);
            fileSelector_->SetTitle(title);
            fileSelector_->SetButtonTexts("OK","CANCEL");
//            dummy->GetWindow()->SetModal(false);
//            dummy->SetBlockEvents(false);
            fileSelector_->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
//            SubscribeToEvent( E_FILESELECTED, URHO3D_HANDLER(InGameEditor, HandleFileSelected));
            return fileSelector_;

    }

    /// GUI WINDOW: Create and pre-populate the Scene Hierarchy Editor Window
    void InGameEditor::CreateHierarchyWindow(){

        /// Check if window already exists eg due to reload of scene containing editor component
        HierarchyWindow_ = (Window*)uiRoot_->GetChild("Hierarchy",true);
        if(HierarchyWindow_)
        {
            HierarchyWindow_->SetPosition(HierarchyPos_);
            return;
        }

        /// Create a basic GUI Window (with title bar, close button, and content panel)
        HierarchyWindow_=CreateWindow("Hierarchy","Scene Hierarchy");

        /// Create UI ListView Element in "Hierarchy Mode" (AKA TreeView)
        ListView* meh=CreateTreeView_FixedSize(HierarchyWindow_, "ListContent", 300, 300);

        /// Add UI TreeView element to Scene Hierarchy Window
        HierarchyWindow_->AddChild(meh);
    }

    void InGameEditor::RebuildHierarchy(ListView* meh, Node* node, UIElement* parent){
        meh->RemoveAllItems();
        RebuildHierarchyRecursive(meh,node,parent);
    }

    void InGameEditor::RebuildHierarchyRecursive(ListView* meh, Node* node, UIElement* parent){


        /// Add current node to list
        Text* t3=new Text(context_);
        meh->InsertItem(-1,t3,parent);

        t3->SetStyle("FileSelectorListText");  /// Provides selection highlighting
        t3->SetText(node->GetName()+" - "+String(node->GetID()));
        t3->SetInternal(true);
        t3->SetVar("NodeID",node->GetID());
        t3->SetColor(Color(0.0f,1.0f,1.0f));



        /// Add all Components of the input node
        Vector<SharedPtr<Component>> ccc = node->GetComponents();
        for(int j=0; j<node->GetNumComponents();j++){
            Component* comp = ccc[j];

            Text* t4=new Text(context_);
            meh->InsertItem(-1,t4,t3);
            t4->SetStyle("FileSelectorListText");  /// Provides selection highlighting
            t4->SetText(comp->GetTypeName()+" - "+String(comp->GetID()));
            t4->SetInternal(true);
            t4->SetVar("ComponentID",comp->GetID());
            t4->SetColor(Color(0,1,0));

            if(selectedComponent_ && selectedComponent_->GetID()==comp->GetID())
            {

                meh->SetSelection(j);

            }

        }

        /// Add all child nodes of the input node
        for(int i=0;i<node->GetNumChildren();i++)
        {
            /// Recurse child node
            RebuildHierarchyRecursive(meh, node->GetChild(i), t3);

        }

    }


    void InGameEditor::RebuildInspector_NodeVars(UIElement* panel){
            const VariantMap vars = selectedNode_->GetVars();
            for(auto it=vars.Begin();it!=vars.End();it++)
            {
                auto* row = AddRow(panel);

                String varname = GetScene()->GetVarName(it->first_);

                Quaternion q;
                LineEdit* ed;
                AddText(row, varname + " (" + it->second_.GetTypeName() + ")");
                switch(it->second_.GetType())
                {
                    case VAR_BOOL:
                        ed=AddLineEdit(row, varname, String(it->second_.GetBool()));
                        ed->SetVar("Type",VAR_BOOL);
                        ed->SetVar("VarName",varname);
                        break;
                    case VAR_INT:
                        ed=AddLineEdit(row, varname, String(it->second_.GetInt()));
                        ed->SetVar("Type",VAR_INT);
                        ed->SetVar("VarName",varname);
                        break;
                    case VAR_FLOAT:
                        ed=AddLineEdit(row, varname, String(it->second_.GetFloat()));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);
                        break;
                    case VAR_VECTOR3:
                        ed=AddLineEdit(row, varname+"_x", String(it->second_.GetVector3().x_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        ed=AddLineEdit(row, varname+"_y", String(it->second_.GetVector3().y_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        ed=AddLineEdit(row, varname+"_z", String(it->second_.GetVector3().z_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        break;
                    case VAR_QUATERNION:
                        q = it->second_.GetQuaternion();
                        ed=AddLineEdit(row, varname+"_x", String(q.x_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        ed=AddLineEdit(row, varname+"_y", String(q.y_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        ed=AddLineEdit(row, varname+"_z", String(q.z_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        ed=AddLineEdit(row, varname+"_w", String(q.w_));
                        ed->SetVar("Type",VAR_FLOAT);
                        ed->SetVar("VarName",varname);

                        break;

                    default:
                        URHO3D_LOGERROR("Unhandled Var Type !!! "+it->second_.GetTypeName());
                }
            }

    }

    void InGameEditor::RebuildInspector_NodeAttribs(UIElement* panel){

            // NODE ATTRIBUTES
            auto attribs = selectedNode_->GetAttributes();
            for(int i=0;i<attribs->Size();i++){
                const AttributeInfo& attrib =  attribs->At(i);

                if(attrib.name_=="Variables")
                    continue;
                if(attrib.name_.StartsWith("Network "))
                    continue;

                switch(attrib.type_){
                case VAR_BOOL:
                    AddAttribute_Bool(selectedNode_, panel, &attrib);
                    break;
                case VAR_VECTOR2:
                    AddAttribute_Vector2(selectedNode_, panel, &attrib);
                    break;
                case VAR_VECTOR3:
                    AddAttribute_Vector3(selectedNode_, panel, &attrib);
                    break;
                case VAR_VECTOR4:
                    AddAttribute_Vector4(selectedNode_, panel, &attrib);
                    break;
                case VAR_FLOAT:
                    AddAttribute_Float(selectedNode_, panel, &attrib);
                    break;
                case VAR_QUATERNION:
                    AddAttribute_Quaternion(selectedNode_,panel, &attrib);
                    break;
                case VAR_STRING:
                    AddAttribute_String(selectedNode_, panel, &attrib);
                    break;
                case VAR_STRINGVECTOR:
                    AddAttribute_StringVector(selectedNode_, panel, &attrib);
                    break;
                case VAR_INT:
                    AddAttribute_Int(selectedNode_, panel, &attrib);
                    break;
                default:
                    /// Unhandled Attribute Type!
                    auto* row=AddRow(panel);
                    AddText(row,attrib.name_+ "(" + Variant::GetTypeName( attrib.type_)+")");
                }

            }
    }

    void InGameEditor::RebuildInspector_ComponentAttribs(UIElement* panel){
        auto attribs = selectedComponent_->GetAttributes();
        if(attribs){
            for(int i=0;i<attribs->Size();i++){
                const AttributeInfo& attrib =  attribs->At(i);

                switch(attrib.type_){
                    case VAR_BOOL:
                        AddAttribute_Bool(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_VECTOR2:
                        AddAttribute_Vector2(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_VECTOR3:
                        AddAttribute_Vector3(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_VECTOR4:
                        AddAttribute_Vector4(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_FLOAT:
                        AddAttribute_Float(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_QUATERNION:
                        AddAttribute_Quaternion(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_STRING:
                        AddAttribute_String(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_STRINGVECTOR:
                        AddAttribute_StringVector(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_INT:
                        AddAttribute_Int(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_RESOURCEREF:
                        AddAttribute_ResourceRef(selectedComponent_, panel, &attrib);
                        break;
                    case VAR_RESOURCEREFLIST:
                        AddAttribute_ResourceRefList(selectedComponent_, panel, &attrib);
                        break;
                    default:
                        /// Unhandled Attribute Type!
                        auto* row=AddRow(panel);
                        AddText(row,attrib.name_+ "(" + Variant::GetTypeName( attrib.type_)+")");

                }
            }
        }
    }

    /// GUI WINDOW: Rebuild content of the Inspector Window
    void InGameEditor::RebuildInspector(){

        if(!selectedNode_)
            selectedNode_=GetScene();



        /// Trash existing content in the UI "Panel" element
        /// (we leave the window titlebar etc intact)
        UIElement* panel = InspectorWindow_->GetChild("Panel",true);

        panel->RemoveAllChildren();

        InspectorWindow_->SetSize(384,24);

        /// Add a new Row to the panel
        UIElement* row = AddRow(panel);

        /// Add a Button to the Row
        Button* btn = AddButton(row, "Node ID: "+String(selectedNode_->GetID()), Color::RED);
        btn->SetColor(Color::GREEN);

        /// Add a Text to the Row
        Text* t = AddText(row, selectedNode_->GetName(),Color::GREEN, TE_SHADOW);

        ///////////
        /// Add a second Row to the panel
        row = AddRow(panel);

        /// Add a button for toggling the spatial system display mode (local or world)
        btn = AddButton(row, "Display as: ", Color::GREEN);
        btn->SetName("SpatialSystemToggleButton");
        btn->SetColor(Color::RED);

        /// Add some text to describe the state of the toggle in human terms
        if(useLocalSpace)
            AddText(row, "Local (Relative)", Color::RED);
        else
            AddText(row, "World (Absolute)", Color::RED);

        /// Subscribe to receive notification of Button Click events
        SubscribeToEvent(btn, E_CLICK, URHO3D_HANDLER(InGameEditor, HandleUIButtonClick));



        ////////////////////////////////////////////////////
        // NODE VARIABLES
        AddVerticalDivider(panel, "Variables");
        if(hideVars==false)
            RebuildInspector_NodeVars(panel);

        ////////////////////////////////////////////////////
        // NODE ATTRIBUTES
        AddVerticalDivider(panel,"Node Attributes");
        if(hideNodeAttribs==false)
            RebuildInspector_NodeAttribs(panel);

        ////////////////////////////////////////////////////
        // COMPONENT ATTRIBUTES
        AddVerticalDivider(panel,"Component Attributes");
        if(!selectedComponent_)
        {
            /// Display full list of Components owned by selected Node
            Vector<SharedPtr<Component>> comps = selectedNode_->GetComponents();
            for(int i=0;i<selectedNode_->GetNumComponents();i++){
                row = AddRow(panel);
                AddText(row,comps[i]->GetTypeName(), Color::MAGENTA);
            }

        } else {

            /// Display name of selected Component
            row = AddRow(panel);
            AddText(row,selectedComponent_->GetTypeName(), Color::MAGENTA);

            /// Optionally, display Component Attributes
            if(hideComponentAttribs==false)
                RebuildInspector_ComponentAttribs(panel);
        }



    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    /// Load gameScene from XML file
    bool InGameEditor::LoadSceneFromXML(String filepath){
        bool success = false;
        /// Open a SceneFile for Loading
        Urho3D::File file(context_, filepath, FILE_READ);
        if(file.IsOpen())
        {
            success = GetScene()->LoadXML(file);


            file.Close();
        }
        return success;

    }

    /// Save gameScene to XML file
    bool InGameEditor::SaveSceneToXML(String filepath){
        bool success=false;
    // Dump our Scene to disk, so we can use it to load from in future
        Urho3D::File file(context_, filepath, FILE_WRITE);
        if(file.IsOpen()){

            Scene* scene = GetScene();
        scene->RegisterVar("Camera Behaviour");
        scene->RegisterVar("Character Node");

        scene->SetVar("Camera Behaviour", cameraBehaviour_);
        scene->SetVar("Character Node", characterNode_->GetID());

            int pos = filepath.FindLast('/');
            String filename=filepath.Substring(pos);

            scene->SetName(filename);
            success = scene->SaveXML(file);
            file.Close();
        }
        return success;
    }

    bool InGameEditor::SaveNodeToXML(String filepath){
        Urho3D::File file(context_, filepath, FILE_WRITE);
        if(file.IsOpen()){
            bool success = selectedNode_->SaveXML(file);
            file.Close();
            return success;
        }
        return false;

    }

    bool InGameEditor::LoadNodeFromXML(String filepath){

        XMLFile* file = GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>(filepath);




            Node* node = GetScene()->InstantiateXML(file->GetRoot(),Vector3::ZERO, Quaternion::IDENTITY);

            if(node)
            {
                node->SetParent(selectedNode_);
                return true;
            }
        return false;

    }

    /// Cast a Ray into the scene to detect drawable objects (via Octree Query)
    bool InGameEditor::Raycast(const Ray& ray, float maxDistance, Vector3& hitPos, Vector3& hitNormal, Drawable*& hitDrawable){
        hitDrawable = nullptr;

        // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
        PODVector<RayQueryResult> results;
        RayOctreeQuery query(results, ray, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
        GetScene()->GetComponent<Octree>()->RaycastSingle(query);
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

    /// EventSink: UI DragMove
    void InGameEditor::HandleWindowDragMove(StringHash eventType, VariantMap& eventData){
        /// Unpack the 2D position delta
        using namespace DragMove;
        int dx = eventData[P_DX].GetInt();
        int dy = eventData[P_DY].GetInt();

        Window* window = (Window*)eventData[P_ELEMENT].GetPtr();

        /// Compute new window position
        IntVector2 pos = window->GetPosition();
        pos.x_ += dx;
        pos.y_ += dy;
        /// Apply new window position
        window->SetPosition( pos );
        // Display new position in window title
        //auto* windowTitle = window->GetChildStaticCast<Text>("WindowTitle", true);
        //windowTitle->SetText( String(pos.x_)+", "+String(pos.y_));

   //     URHO3D_LOGINFO(String(dx)+", "+String(dy));
        const String& name = window->GetName();
        if(name=="Inspector")
            InspectorPos_ = pos;
        else if(name=="Hierarchy")
            HierarchyPos_=pos;

    }

    /// EventSink: user has selected a dropdown list item
    void InGameEditor::HandleUIDropDownItemSelected(StringHash eventType, VariantMap& eventData){
        using namespace ItemSelected;
        DropDownList* list = (DropDownList*)eventData[P_ELEMENT].GetPtr();
        Text* text = (Text*)list->GetSelectedItem();
        const String& bleh=text->GetText();

        if(bleh=="Inspector")
        {
            InspectorIsVisible=!InspectorIsVisible;
            InspectorWindow_->SetVisible(InspectorIsVisible);
        } else if(bleh=="Hierarchy") {
            HierarchyIsVisible=!HierarchyIsVisible;
            HierarchyWindow_->SetVisible(HierarchyIsVisible);
        } else if(bleh=="Load Scene") {
            if(fileSelector_)
                delete fileSelector_;
            fileSelector_=CreateFileSelector("LOAD SCENE:");
            SubscribeToEvent(E_FILESELECTED, URHO3D_HANDLER(InGameEditor, HandleSceneLoadFileSelected));
        } else if(bleh=="Save Scene") {
            if(fileSelector_)
                delete fileSelector_;
            fileSelector_=CreateFileSelector("SAVE SCENE:");
            SubscribeToEvent(E_FILESELECTED, URHO3D_HANDLER(InGameEditor, HandleSceneSaveFileSelected));
        } else if(bleh=="New Scene")
        {

                MessageBox* box = new MessageBox(context_,"Are you sure?", "NEW SCENE!",GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/MessageBox.xml"));
                box->GetWindow()->GetChild("CancelButton",true)->SetVisible(true);
                SubscribeToEvent(box, E_MESSAGEACK,URHO3D_HANDLER(InGameEditor, HandleNewSceneMessageBoxAck));

        } else if(bleh=="Load Prefab") {
            if(fileSelector_)
                delete fileSelector_;
            fileSelector_=CreateFileSelector("LOAD PREFAB:");
            SubscribeToEvent(E_FILESELECTED, URHO3D_HANDLER(InGameEditor, HandleNodeLoadFileSelected));

        } else if(bleh=="Save Prefab"){
            if(fileSelector_)
                delete fileSelector_;

            fileSelector_=CreateFileSelector("SAVE PREFAB:");
            SubscribeToEvent(E_FILESELECTED, URHO3D_HANDLER(InGameEditor, HandleNodeSaveFileSelected));

        }


        else
            URHO3D_LOGWARNING("Unhandled DropDownItem: "+bleh);
    }

    void InGameEditor::HandleListViewItemClicked(StringHash eventType, VariantMap& eventData){
        using namespace ItemClicked;

        /// Access the Element that was clicked
        Text* text = (Text*)eventData[P_ITEM].GetPtr();

        int SelectionID = eventData[P_SELECTION].GetInt();

        int btn = eventData[P_BUTTON].GetInt();
        if(btn==MOUSEB_LEFT)
        {
            ListView* lv = (ListView*)HierarchyWindow_->GetChild("ListContent",false);

            float vrange = lv->GetVerticalScrollBar()->GetRange();
            float vpos = lv->GetVerticalScrollBar()->GetValue();

            if(!InspectorIsVisible){
                InspectorIsVisible = true;
                InspectorWindow_->SetVisible(true);
            }

            /// Check if the user clicked on a Scene Node element
            Variant v=text->GetVar("NodeID");
            if(v.GetType()!=VAR_NONE){
                selectedNode_=GetScene()->GetNode(v.GetInt());
                selectedComponent_=nullptr;
                selectedDrawable_=selectedNode_->GetDerivedComponent<Drawable>();
                RebuildInspector();
                RebuildHierarchy( lv, GetScene());
                // restore scroll position
                lv->GetVerticalScrollBar()->SetRange(vrange);
                lv->GetVerticalScrollBar()->SetValue(vpos);
                return;
            }

            /// Check if the user clicked on a Component element
            v=text->GetVar("ComponentID");
            if(v.GetType()!=VAR_NONE){
                selectedComponent_=GetScene()->GetComponent(v.GetInt());
                selectedNode_=selectedComponent_->GetNode();
                selectedDrawable_=selectedNode_->GetDerivedComponent<Drawable>();
                RebuildInspector();
                RebuildHierarchy( lv, GetScene());
                // restore scroll position
                lv->GetVerticalScrollBar()->SetRange(vrange);
                lv->GetVerticalScrollBar()->SetValue(vpos);
                return;
            }
        } else if(btn==MOUSEB_RIGHT){

            Variant v=text->GetVar("NodeID");
            if(v.GetType()!=VAR_NONE){
                CreateContextMenu(GetScene()->GetNode(v.GetInt()));
            }else{
                v=text->GetVar("ComponentID");
                if(v.GetType()!=VAR_NONE)
                {
                    Component* comp = GetScene()->GetComponent(v.GetInt());
                    if(comp)
                        CreateContextMenu(comp);
                }
            }
        }

    }

    /// EventSink: Window CloseButton
    void InGameEditor::HandleClosePressed(StringHash eventType, VariantMap& eventData){
        using namespace Released;
        UIElement* btn = (UIElement*)eventData[P_ELEMENT].GetPtr();
        UIElement* pnt = btn->GetParent()->GetParent();
        const String& pntname = pnt->GetName();
        pnt->SetVisible(false);

        if(pntname=="Inspector")
            InspectorIsVisible=!InspectorIsVisible;
        else if(pntname=="Hierarchy")
            HierarchyIsVisible=!HierarchyIsVisible;

    }

    void InGameEditor::HandleUIButtonClick(StringHash eventType, VariantMap& eventData){
        using namespace Click;

        Button* button = (Button*)eventData[P_ELEMENT].GetPtr();
        if(button->GetName()=="SpatialSystemToggleButton")
        {
            useLocalSpace=!useLocalSpace;
            RebuildInspector();
        }
    }

    /// User has pressed "enter" while editing a LineEdit element
    void InGameEditor::HandleTextEditFinished(StringHash eventType, VariantMap& eventData){
        using namespace TextFinished;
        UIElement* element = (UIElement*)eventData[P_ELEMENT].GetPtr();
        String text = eventData[P_TEXT].GetString();
        float value = eventData[P_VALUE].GetFloat();

        Variant t1 = element->GetVar("VarName");
        if(t1.GetType()!=VAR_NONE){
            Variant t2;
            t2.FromString( (VariantType)element->GetVar("Type").GetInt(),text);
            selectedNode_->SetVar( t1.GetString(), t2 );

        } else {
              t1 = element->GetVar("Attribute");
              if(t1.GetType()!=VAR_NONE){

                Serializable* source=(Serializable*)element->GetVar("Source").GetPtr();

                VariantType type_ = (VariantType)element->GetVar("Type").GetInt();

                if(type_==VAR_VECTOR2){

                    Variant t2;
                    t2.FromString( VAR_FLOAT,text);
                    float fval=t2.GetFloat();

                    Vector2 val = source->GetAttribute(t1.GetString()).GetVector2();

                    const String& field = element->GetName();
                    if(field=="X")
                        val.x_ =fval;
                    else if(field=="Y")
                        val.y_ =fval;

                    source->SetAttribute(t1.GetString(), Variant(val));

                    RebuildInspector();

                    return;

                }

                if(type_==VAR_VECTOR3){

                    Variant t2;
                    t2.FromString( VAR_FLOAT,text);
                    float fval=t2.GetFloat();

                    Vector3 val = source->GetAttribute(t1.GetString()).GetVector3();

                    const String& field = element->GetName();
                    if(field=="X")
                        val.x_ =fval;
                    else if(field=="Y")
                        val.y_ =fval;
                    else if(field=="Z")
                        val.z_ =fval;

                    source->SetAttribute(t1.GetString(), Variant(val));

                    RebuildInspector();

                    return;

                }

                if(type_==VAR_VECTOR4){

                    Variant t2;
                    t2.FromString( VAR_FLOAT,text);
                    float fval=t2.GetFloat();

                    Vector4 val = source->GetAttribute(t1.GetString()).GetVector4();

                    const String& field = element->GetName();
                    if(field=="X")
                        val.x_ =fval;
                    else if(field=="Y")
                        val.y_ =fval;
                    else if(field=="Z")
                        val.z_ =fval;
                    else if(field=="W")
                        val.w_ =fval;
                    source->SetAttribute(t1.GetString(), Variant(val));

                    RebuildInspector();

                    return;

                }

                if(type_==VAR_QUATERNION){
                    Variant t2;
                    t2.FromString( VAR_FLOAT,text);
                    float fval=t2.GetFloat();

                    Quaternion val = source->GetAttribute(t1.GetString()).GetQuaternion();

                    const String& field = element->GetName();
                    if(field=="X")
                        val.x_ =fval;
                    else if(field=="Y")
                        val.y_ =fval;
                    else if(field=="Z")
                        val.z_ =fval;
                    else if(field=="W")
                        val.w_ =fval;

                    source->SetAttribute(t1.GetString(), Variant(val));

                    return;


                }


                /// Convert user plaintext into typed variant
                Variant t2;
                t2.FromString( (VariantType)element->GetVar("Type").GetInt(),text);
                /// Store variant to named attribute
                source->SetAttribute( t1.GetString(), t2 );


                RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetScene());
                RebuildInspector();


              }else{
                 /// We forgot to tag this ui element?
                     int x=0;

              }

        }

        int x=0;
    }

    /// The user has clicked on a "collapsing section" vertical divider
    void InGameEditor::HandleCollapsingSection(StringHash eventType, VariantMap& eventData){
        using namespace Click;
        Button* button = (Button*)eventData[P_ELEMENT].GetPtr();

        if(button->GetName()=="Variables")
            hideVars=!hideVars;
       else if(button->GetName()=="Node Attributes")
            hideNodeAttribs=!hideNodeAttribs;
        else
            hideComponentAttribs=!hideComponentAttribs;

        RebuildInspector();
    }

     /// The user has selected a filename for loading the scene
    void InGameEditor::HandleSceneLoadFileSelected(StringHash eventType, VariantMap& eventData){
        using namespace FileSelected;
        bool ok = eventData[P_OK].GetBool();
        if(ok){

            /// Take ownership of the FileSelector object:
            /// it's not "part of the scene", and we need to ensure
            /// that it is properly removed from the UI system
            SharedPtr<FileSelector> s(fileSelector_);

            /// Attempt to load scene
            String filename = eventData[P_FILENAME].GetString();
            ok=LoadSceneFromXML(filename);

            /// If attempt to load has failed, we'll relinquish ownership
            /// to ensure our fileselector is NOT destroyed when 's' goes out of scope
            if(!ok)
                s.Detach();

        }
        else {
            delete fileSelector_;
        }
    }

    /// The user has selected a filename for saving the scene
    void InGameEditor::HandleSceneSaveFileSelected(StringHash eventType, VariantMap& eventData){
        using namespace FileSelected;
        bool ok = eventData[P_OK].GetBool();
        if(ok){
            String filename = eventData[P_FILENAME].GetString();

            FileSystem* fs = GetSubsystem<FileSystem>();
            if(fs->FileExists(filename))
            {

                MessageBox* box = new MessageBox(context_,"File Exists - Overwrite?", "WARNING!",GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/MessageBox.xml"));
                box->GetWindow()->GetChild("CancelButton",true)->SetVisible(true);
                SetGlobalVar("FileSavePath",filename);
                SubscribeToEvent(box, E_MESSAGEACK,URHO3D_HANDLER(InGameEditor, HandleSceneFileOverwriteAck));
            }else{
                ok=SaveSceneToXML(filename);
                if(ok)
                   delete fileSelector_;

            }
        }
        else{
            delete fileSelector_;
        }

        int x=0;
    }

     /// The user has selected a filename for loading the scene
    void InGameEditor::HandleNodeLoadFileSelected(StringHash eventType, VariantMap& eventData){
        using namespace FileSelected;
        bool ok = eventData[P_OK].GetBool();
        if(ok){


            /// Attempt to load scene
            String filename = eventData[P_FILENAME].GetString();
            ok=LoadNodeFromXML(filename);

            if(ok)
            {
                delete fileSelector_;

                RebuildInspector();
                RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetNode()->GetScene());
            }


        }
        else {
            delete fileSelector_;
        }
    }

    void InGameEditor::HandleNodeSaveFileSelected(StringHash eventType, VariantMap& eventData){
        using namespace FileSelected;
        bool ok = eventData[P_OK].GetBool();
        if(ok){
            String filename = eventData[P_FILENAME].GetString();

            FileSystem* fs = GetSubsystem<FileSystem>();
            if(fs->FileExists(filename))
            {

                MessageBox* box = new MessageBox(context_,"File Exists - Overwrite?", "WARNING!",GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/MessageBox.xml"));
                box->GetWindow()->GetChild("CancelButton",true)->SetVisible(true);
                SetGlobalVar("FileSavePath",filename);
                SubscribeToEvent(box, E_MESSAGEACK,URHO3D_HANDLER(InGameEditor, HandlePrefabFileOverwriteAck));
            }else{
                ok=SaveNodeToXML(filename);
                if(ok)
                   delete fileSelector_;

            }
        }
        else{
            delete fileSelector_;
        }

        int x=0;
    }

    /// The user has responded to MessageBox prompt (whether to overwrite existing file)
    void InGameEditor::HandleSceneFileOverwriteAck(StringHash eventType, VariantMap& eventData){
        using namespace MessageACK;
        if(eventData[P_OK].GetBool()==true){
            bool ok=SaveSceneToXML(GetGlobalVar("FileSavePath").GetString());
            if(ok)
                delete fileSelector_;
        }
    }

    void InGameEditor::HandlePrefabFileOverwriteAck(StringHash eventType, VariantMap& eventData){
        using namespace MessageACK;
        if(eventData[P_OK].GetBool()==true){
            bool ok=SaveNodeToXML(GetGlobalVar("FileSavePath").GetString());
            if(ok)
                delete fileSelector_;
        }
    }

    /// The user has responded to MessageBox prompt (whether to load the "empty" scene)
    void InGameEditor::HandleNewSceneMessageBoxAck(StringHash eventType, VariantMap& eventData){
         using namespace MessageACK;
        if(eventData[P_OK].GetBool()==true){
             String fullpath=GetSubsystem<FileSystem>()->GetProgramDir ()+"EmptyScene.xml" ;
            bool ok=LoadSceneFromXML(fullpath);
        }
    }


    void InGameEditor::HandleCancelContextWindow(StringHash eventType, VariantMap& eventData){
        if(nodeContextWindow_)
        {
            nodeContextWindow_->Remove();
        }
    }

    void InGameEditor::HandleDeleteComponentMessageBoxAck(StringHash eventType, VariantMap& eventData){
        using namespace MessageACK;
        if(eventData[P_OK].GetBool()==true){
            if(deleteTargetComponent_){
                deleteTargetComponent_->Remove();
                RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetNode()->GetScene());
                RebuildInspector();
            }
            nodeContextWindow_->Remove();
        }
    }

    void InGameEditor::HandleDeleteNodeMessageBoxAck(StringHash eventType, VariantMap& eventData){
        using namespace MessageACK;
        if(eventData[P_OK].GetBool()==true){
            if(deleteTargetNode_){
                deleteTargetNode_->Remove();
                RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetNode()->GetScene());
                RebuildInspector();
            }
            nodeContextWindow_->Remove();
        }
    }

    void InGameEditor::HandleCreateComponent(StringHash eventType, VariantMap& eventData){
        using namespace Click;
        Button* b = (Button*)eventData[P_ELEMENT].GetPtr();
        UIElement* e = b->GetParent()->GetParent()->GetParent();
        String name = e->GetVar("ComponentName").GetString();
        CreateMode mode = (CreateMode)e->GetVar("CreateMode").GetInt();

        if(name!=""){

            Node* node = (Node*)e->GetVar("ContextParentNode").GetPtr();

            node->CreateComponent(name,mode);

            compContextWindow_->Remove();
            nodeContextWindow_->Remove();

            RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetNode()->GetScene());
            RebuildInspector();

        }
        int x=0;
    }

    void InGameEditor::HandleComponentLVItemSelected(StringHash eventType, VariantMap& eventData){
        using namespace ItemSelected;

        DropDownList* e = (DropDownList*)eventData[P_ELEMENT].GetPtr();

        Text* ee = (Text*)e->GetSelectedItem();
        String name = ee->GetText();

        String pname = e->GetParent()->GetParent()->GetParent()->GetTypeName();
        e->GetParent()->GetParent()->GetParent()->SetVar("ComponentName",name);
        e->GetParent()->GetParent()->GetParent()->SetVar("ContextParentNode",e->GetVar("ContextParentNode"));

        int x=0;
    }

    /// Create / populate a window for selecting which component type to create
    void InGameEditor::CreateComponentSelector(Node* node, CreateMode mode){
            if(compContextWindow_)
                compContextWindow_->Remove();

            String title="Create Component (";
            if(mode==CreateMode::LOCAL)
                title+="local)";
            else
                title+="replicated)";

            compContextWindow_ = CreateWindow("CreateComponent ",title);
            UIElement* panel = compContextWindow_->GetChild("Panel",true);
            for(auto it=componentMap_.Begin(); it!=componentMap_.End(); it++){

                UIElement* row = AddRow(panel);

                AddText(row, it->first_);

                /// Create a dropdown list for the new main menu item
                DropDownList* list = new DropDownList(context_);
                list->SetName("DropDownList_"+it->first_);
                list->SetStyleAuto();
                list->SetMinHeight(32);
                list->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
                list->SetResizePopup(true); /// Expand popup to width of container
                list->SetVar("ContextParentNode",node);
                list->SetVar("CreateMode",mode);
                row->AddChild(list);

                SubscribeToEvent(list, E_ITEMSELECTED,URHO3D_HANDLER(InGameEditor, HandleComponentLVItemSelected));

                /// Populate the dropdown list
                for (int i = 0; i < it->second_.Size(); i++)
                {
                    Text* t = AddTextItem(list,it->second_[i] );
                    t->SetMinWidth(  t->GetRowWidth(0) + 10 );
                    t->SetHoverColor(Color(0.2f,0.5f,0.2f, 0.9f));

                }





            }

            UIElement* row = AddRow(panel);
            Button* btn = AddButton(row,"CREATE", Color::GREEN);
            SubscribeToEvent(btn,E_CLICK, URHO3D_HANDLER(InGameEditor, HandleCreateComponent));
            btn = AddButton(row,"CANCEL", Color::RED);
            SubscribeToEvent(btn,E_CLICK, URHO3D_HANDLER(InGameEditor, HandleCancelContextWindow));

            Menu* menu = new Menu(context_);
            panel->AddChild(menu);

            menu->SetName("test menu");
            menu->SetStyleAuto();
            menu->SetLayout(LM_VERTICAL, 1, IntRect(2,6,2,6));

            // Create Text Label
 /*           Text* menuText =new Text(context_);
            menuText->SetName("bleh_text");
            menu->AddChild(menuText);
            menuText->SetStyle("EditorMenuText");
            menuText->SetText("meh");
*/
            // set menubutton size
            menu->SetFixedWidth(100);

            // create popup
            Window* popup = new  Window(context_);
            popup->SetName("_popup");
            popup->SetLayout(LM_VERTICAL, 1, IntRect(2, 6, 2, 6));
            popup->SetStyleAuto(panel->GetDefaultStyle());
            menu->SetPopup(popup);
            menu->SetPopupOffset(IntVector2(0, menu->GetHeight()));

            Text* menuText = AddText(menu, "test menu",Color::WHITE, TE_STROKE, "EditorMenuText");
            menuText->SetHoverColor(Color(0.2f,0.5f,0.2f, 0.9f));

            ListView* list = new ListView(context_);
            list->SetDefaultStyle( GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml") );
            list->SetStyleAuto();
            list->SetFixedSize(100,200);
            list->SetVar("OwnerMenuItem",menu);
            popup->AddChild(list);

            menuText = new Text(context_);
            menuText->SetName("1");
            list->AddItem(menuText);
            menuText->SetStyle("EditorMenuText");
            menuText->SetText("grr1");
            menuText->SetHoverColor(Color(0.2f,0.5f,0.2f, 0.9f));


            SubscribeToEvent(list, E_ITEMSELECTED, URHO3D_HANDLER(InGameEditor, HandleMenuSelected));


            menuText = new Text(context_);
            menuText->SetName("2");
            list->AddItem(menuText);
            menuText->SetStyle("EditorMenuText");
            menuText->SetText("grr2");
            menuText->SetHoverColor(Color(0.2f,0.5f,0.2f, 0.9f));

            menuText = new Text(context_);
            menuText->SetName("3");
            list->AddItem(menuText);
            menuText->SetStyle("EditorMenuText");
            menuText->SetText("grr3");
            menuText->SetHoverColor(Color(0.2f,0.5f,0.2f, 0.9f));
    }

    void InGameEditor::HandleContextButtonClick(StringHash eventType, VariantMap& eventData){
        using namespace Click;

        Button* button = (Button*)eventData[P_ELEMENT].GetPtr();

        Node* node = (Node*) button->GetParent()->GetVar("ContextParentNode").GetPtr();
        Component* comp = (Component*) button->GetParent()->GetVar("ContextComponent").GetPtr();


        const String& name = button->GetName();
        if(name=="LocalNode"){
            node->CreateChild("",LOCAL);
            RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetScene());
            nodeContextWindow_->Remove();
        }else if(name=="NetworkNode"){
            node->CreateChild("",REPLICATED);
            RebuildHierarchy( (ListView*)HierarchyWindow_->GetChild("ListContent",false), GetScene());
            nodeContextWindow_->Remove();

        }else if(name=="DeleteComponent"){
                MessageBox* box = new MessageBox(context_,"Are you sure?", "DELETE COMPONENT!",GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/MessageBox.xml"));
                box->GetWindow()->GetChild("CancelButton",true)->SetVisible(true);
                deleteTargetComponent_=comp;
                SubscribeToEvent(box, E_MESSAGEACK,URHO3D_HANDLER(InGameEditor, HandleDeleteComponentMessageBoxAck));

        }else if(name=="DeleteNode"){
                MessageBox* box = new MessageBox(context_,"Are you sure?", "DELETE NODE!",GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/MessageBox.xml"));
                box->GetWindow()->GetChild("CancelButton",true)->SetVisible(true);
                deleteTargetNode_=node;
                SubscribeToEvent(box, E_MESSAGEACK,URHO3D_HANDLER(InGameEditor, HandleDeleteNodeMessageBoxAck));
        } else if(name=="LocalComponent")
            CreateComponentSelector(node, LOCAL);

        else if(name=="NetworkComponent")
            CreateComponentSelector(node, REPLICATED);

        else
            URHO3D_LOGWARNING("Unhandled Context Button: "+name);
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    /// EventSink: user has pressed a keyboard key
    void InGameEditor::HandleKeyDown(StringHash eventType, VariantMap& eventData){
        using namespace KeyDown;
        int key = eventData[P_KEY].GetInt();
        switch(key){

        case KEY_TAB:
            isVisible=!isVisible;
            MainMenu_->SetVisible(isVisible);
            InspectorWindow_->SetVisible(isVisible && InspectorIsVisible);
            HierarchyWindow_->SetVisible(isVisible && HierarchyIsVisible);

            if(InspectorWindow_->IsVisible())
                RebuildInspector();

            /// Flipping mousecursor visibility !
            GetSubsystem<Input>()->SetMouseVisible(isVisible);

            break;

        case KEY_F1:
            if(selectedDrawable_)
                characterNode_=selectedDrawable_->GetNode();
            break;
        }
    }

    void InGameEditor::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData){
        using namespace MouseButtonDown;
        int buttonID = eventData[P_BUTTON].GetInt();
        if(buttonID==MOUSEB_MIDDLE )
        {

            if(!selectedNode_)
                return;

            /// Preserve yaw and pitch values while manipulating character orientation
            oldYaw_=yaw_;
            oldPitch_=pitch_;
            /// Temporarily assume yaw and pitch from character node
            auto q = selectedNode_->GetRotation();
            yaw_   = q.YawAngle();
            pitch_ = q.PitchAngle();
        }

    }

    void InGameEditor::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData){
        using namespace MouseButtonUp;
        int buttonID = eventData[P_BUTTON].GetInt();
        if(buttonID==MOUSEB_MIDDLE )
        {
            if(!selectedNode_)
                return;
            /// Restore yaw and pitch values after manipulating character orientation
            yaw_=oldYaw_;
            pitch_=oldPitch_;
        }

    }

        void InGameEditor::HandleMenuSelected(StringHash eventType, VariantMap& eventData){
            using namespace ItemSelected;

            // LV is inside a popup window - we cant search parents to find owner Menu!
            ListView* e = (ListView*)eventData[P_ELEMENT].GetPtr();

            // But we can tag the ListView with a user variable... and we did!
            Menu* menu = (Menu*)e->GetVar("OwnerMenuItem").GetPtr();

            // Let's close that pesky popup window
            menu->ShowPopup(false);

            // Finally, we can inspect the ListView for the selected item
            UIElement* selection = e->GetSelectedItem();

            String name = selection->GetName();


            int x=0;
        }



   ////////////////////////////////////////////////////////////////////////////////////

    /// Post-Render event handler (DebugDrawing)
    void InGameEditor::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData){

        if(!characterNode_)
            return;

        /// Obtain access to our "Box1" node
        Node* boxnode = GetScene()->GetChild("Box1");

        /// Declare a variable to hold the world position of some Drawable
        Vector3 drawPos;

        /// Define a (buildtime) Macro to draw the local major axes of some Drawable
        #define DrawMajorAxes(node, length, depthtest) \
        if(node!=nullptr){\
        drawPos = node->GetWorldPosition();\
        const Vector3& scale = node->GetWorldScale();\
        debugDraw_->AddLine(drawPos, node->LocalToWorld(Vector3::RIGHT   * length / scale.x_), Color(1, 0, 0, 1), depthtest);\
        debugDraw_->AddLine(drawPos, node->LocalToWorld(Vector3::UP      * length / scale.y_), Color(0, 1, 0, 1), depthtest);\
        debugDraw_->AddLine(drawPos, node->LocalToWorld(Vector3::FORWARD * length / scale.z_), Color(0, 0, 1, 1), depthtest);\
        }

        /// Draw the major axes of our "Box1", and our current character
        DrawMajorAxes(boxnode,        3.0f, true);
        DrawMajorAxes(characterNode_, 3.0f, true);

        /// Draw ORANGE the World AABB of the object currently underneath the mouse cursor
        /// (unless its our current character...)
        if(candidateDrawable_ && candidateDrawable_->GetNode()!=characterNode_){
            debugDraw_->AddBoundingBox( candidateDrawable_->GetWorldBoundingBox(), Color(1,1,0), true);
            candidateDrawable_->DrawDebugGeometry(debugDraw_,false);
            //DrawMajorAxes(candidateDrawable_->GetNode(),3.0f, true);
        }

        /// Draw DARK GREEN the World AABB of the "currently selected object"
        if(selectedDrawable_){
            //debugDraw_->AddBoundingBox( selectedDrawable_->GetWorldBoundingBox(), Color(0,0.4f,0), true);
//            selectedDrawable_->DrawDebugGeometry(debugDraw_,true);

            /// Compute theoretical sphere radius for rotation gizmo
            const BoundingBox& bb = selectedDrawable_->GetWorldBoundingBox();

            Sphere s(bb);
            float r = s.radius_;

            DrawMajorAxes(selectedDrawable_->GetNode(),r, true);


            /// Draw three circles to represent the rotation axes of the selected entity
            Node* n = selectedDrawable_->GetNode();
            debugDraw_->AddCircle(n->GetWorldPosition(), n->GetWorldRight(), r, Color::RED, 32, false);
            debugDraw_->AddCircle(n->GetWorldPosition(), n->GetWorldUp(),    r, Color::GREEN, 32, false);
            debugDraw_->AddCircle(n->GetWorldPosition(), n->GetWorldDirection(), r, Color::BLUE, 32, false);

        }

        // Debug-Draw the Octree
        GetScene()->GetComponent<Octree>()->DrawDebugGeometry(debugDraw_, true);

        /// Create a Query Ray for MousePicking
        /// This is redundant - we already did this in the Frame Update handler!
        /// We should have cached our query ray...
        auto* input=GetSubsystem<Input>();
        IntVector2 pos=input->GetMousePosition();
        auto* graphics = GetSubsystem<Graphics>();
        auto* camera = EditorCameraNode_->GetComponent<Camera>();
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

        /// Draw a surface-oriented crosshair to represent the selection cursor
        debugDraw_->AddCircle(cameraRay.origin_+cameraRay.direction_*0.1f, Normal, 0.005f, color, 32,false);
        debugDraw_->AddCross(cameraRay.origin_+cameraRay.direction_*0.1f, candidateDrawable_?candidateDrawable_->GetNode()->GetRotation():EditorCameraNode_->GetRotation(), 0.005f, color,false);

        /// DebugDraw the navmesh
        auto* navmesh=GetScene()->GetComponent<DynamicNavigationMesh>();
        if(navmesh)
            navmesh->DrawDebugGeometry(debugDraw_, true);

    }

/////////////////////////////////////////////////////////////////////////////////////

    /// Implements "free-look" camera behaviour
    /// Use WASD to move the camera, and mouse to look around.
    void InGameEditor::MoveCamera(float timeStep){

        /// Sanity Check!
        if(!EditorCameraNode_)
            return;

        auto* input = GetSubsystem<Input>();

        /// If system cursor is visible, or app window loses input focus, just get outta here
       // if(input->IsMouseVisible() || !input->HasFocus())
       //     return;

        // Do not move if the UI has a focused element (the console)
        //auto* ui = GetSubsystem<UI>();
        //if (ui->GetFocusElement())
        //    return;

        if(MainMenu_->IsVisible())
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

            EditorCameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));


        /// Watch the WASD Keys - notice we're not using the KeyDown event to do so.
        /// Translation will be performed in "Local Space" - so relative to current camera orientation.
        if (input->GetKeyDown(KEY_W))
            EditorCameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_S))
            EditorCameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_A))
            EditorCameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
        if (input->GetKeyDown(KEY_D))
            EditorCameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    }


