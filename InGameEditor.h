
using namespace Urho3D;

class InGameEditor:public LogicComponent
{
    URHO3D_OBJECT(InGameEditor, LogicComponent);

    /// Camera Behaviour
    enum CameraBehaviour{
        FreeLook,
        Chase
    };
    CameraBehaviour cameraBehaviour_ = FreeLook;

public:

    /// Factory Function
    static void RegisterObject(Context* context);

    /// Constructor
    InGameEditor(Context* context);

    /// Set up UI and event handling
    virtual void DelayedStart();

    /// When the menu is hidden, we can move the camera
    virtual void Update(float dT);

private:
    float yaw_, pitch_;                             // Camera Orientation

    bool useLocalSpace=false;                       // Not Implemented

    bool isVisible=false;                           // Visibility: Main Menu
    bool HierarchyIsVisible=false;                  // Visibility: Scene Hierarchy
    bool InspectorIsVisible=false;                  // Visibility: Inspector

    WeakPtr<DebugRenderer> debugDraw_;              // Support for debug-drawing

    WeakPtr<UIElement>  uiRoot_;                    // Root element of UI subsystem
    WeakPtr<UIElement>  MainMenu_;                  // Dropdown Menu at top of the screen
    WeakPtr<Window>     HierarchyWindow_;           // Scene Hierarchy Editor
    WeakPtr<Window>     InspectorWindow_;           // Node and Component Editor
    WeakPtr<FileSelector> fileSelector_;            // Specialized UI Window for selecting files

    WeakPtr<Node>       EditorCameraNode_;          // Scene's camera node (shared)
    WeakPtr<Node>       selectedNode_;              // Currently selected Node (in Hierarchy window)
    WeakPtr<Component>  selectedComponent_;         // Currently selected Component (in Hierarchy window)
    WeakPtr<Drawable>   selectedDrawable_;          // Drawable currently "selected"
    WeakPtr<Drawable>   candidateDrawable_;         // Drawable under the mousecursor
    Vector3             candidateNormal_;           // SurfaceNormal under the mousecursor

    WeakPtr<Node> characterNode_;                   // Root node for our "player character"

    IntVector2 InspectorPos_;                       // Screen position of these UI Windows
    IntVector2 HierarchyPos_;

    HashMap<String, Vector<String>> componentMap_;


    /// State of Inspector "collapsing sections"
    bool hideVars=true, hideNodeAttribs=true, hideComponentAttribs=true;

    /// UI utility methods
    void AddVerticalDivider( UIElement*    container, const String& text);
    LineEdit* AddLineEdit(   UIElement*    container, const String& Name, const String& text);
    LineEdit* InsertLineEdit(DropDownList* container, const String& Name, const String& text);
    Text* AddText(           UIElement*    container, const String& text, const Color& color=Color::WHITE, TextEffect effect=TE_NONE, String style="");
    Text* AddTextItem(       DropDownList* container, const String& text, const Color& color=Color::WHITE, TextEffect effect=TE_NONE, String style="");
    Button* AddButton(       UIElement*    container, const String& text, const Color& TextColor=Color::WHITE);
    UIElement* AddRow(       UIElement*    container);
    DropDownList* AddDropDownList(UIElement* container);
    ListView* CreateTreeView_FixedSize(UIElement* container, const String& Name, int width, int height );
    DropDownList* CreateMainMenuItem(String label, Vector<String> items, String handler);
    Window* CreateWindow(const String& Name, const String& Title);
    FileSelector* CreateFileSelector(const String& title);
    void CreateComponentSelector(Node* node, CreateMode mode=LOCAL);

    /// Methods for adding a suitable input UI element for a Typed Attribute
    void AddAttribute_Vector2(        Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_Vector3(        Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_Vector4(        Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_Quaternion(     Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_Float(          Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_Bool(           Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_Int(            Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_String(         Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_StringVector(   Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_ResourceRef(    Serializable* source, UIElement* container, const AttributeInfo* info);
    void AddAttribute_ResourceRefList(Serializable* source, UIElement* container, const AttributeInfo* info);

    SharedPtr<Window> nodeContextWindow_;
    SharedPtr<Window> compContextWindow_;

    void CreateContextMenu(Node* parent);
    void CreateContextMenu(Component* comp);

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// Attribute Inspector Editor Window
    void RebuildInspector();
    void RebuildInspector_NodeVars(UIElement* panel);
    void RebuildInspector_NodeAttribs(UIElement* panel);
    void RebuildInspector_ComponentAttribs(UIElement* panel);

    /////////////////////////////////////////////////////////////////////////////////////////////
    /// Scene Hierarchy Editor Window
    void CreateHierarchyWindow();
    void RebuildHierarchy(ListView* meh, Node* node, UIElement* parent=nullptr);
    void RebuildHierarchyRecursive(ListView* meh, Node* node, UIElement* parent=nullptr);

    /////////////////////////////////////////////////////////////////////////////////////////////
    /// LOADING AND SAVING!
    bool LoadSceneFromXML(String filepath);
    bool SaveSceneToXML(String filepath);
    bool LoadNodeFromXML(String filepath);
    bool SaveNodeToXML(String filepath);
    /////////////////////////////////////////////////////////////////////////////////////////////

    /// PickRay selection of Drawables
    bool Raycast(const Ray& ray, float maxDistance, Vector3& hitPos, Vector3& hitNormal, Drawable*& hitDrawable);

    /// Implements "free-look" camera behaviour
    /// Use WASD to move the camera, and mouse to look around.
    void MoveCamera(float timeStep);

    /////////////////////////////////////////////////////////////////////////////////

    /// UI Event Handling:
    void HandleWindowDragMove(        StringHash eventType, VariantMap& eventData);
    void HandleUIDropDownItemSelected(StringHash eventType, VariantMap& eventData);
    void HandleListViewItemClicked(   StringHash eventType, VariantMap& eventData);
    void HandleClosePressed(          StringHash eventType, VariantMap& eventData);
    void HandleUIButtonClick(         StringHash eventType, VariantMap& eventData);
    void HandleTextEditFinished(      StringHash eventType, VariantMap& eventData);
    void HandleCollapsingSection(     StringHash eventType, VariantMap& eventData);
    void HandleSceneLoadFileSelected( StringHash eventType, VariantMap& eventData);
    void HandleSceneSaveFileSelected( StringHash eventType, VariantMap& eventData);
    void HandleNodeLoadFileSelected(  StringHash eventType, VariantMap& eventData);
    void HandleNodeSaveFileSelected(  StringHash eventType, VariantMap& eventData);
    void HandleContextButtonClick(    StringHash eventType, VariantMap& eventData);
    void HandleMenuSelected(          StringHash eventType, VariantMap& eventData);
    void HandleComponentLVItemSelected(StringHash eventType, VariantMap& eventData);

    /// MessageBox Acknowledgement
    void HandleSceneFileOverwriteAck( StringHash eventType, VariantMap& eventData);
    void HandlePrefabFileOverwriteAck(StringHash eventType, VariantMap& eventData);
    void HandleNewSceneMessageBoxAck( StringHash eventType, VariantMap& eventData);
    void HandleCancelContextWindow(   StringHash eventType, VariantMap& eventData);
    void HandleCreateComponent(  StringHash eventType, VariantMap& eventData);
    void HandleDeleteComponentMessageBoxAck(StringHash eventType, VariantMap& eventData);
    void HandleDeleteNodeMessageBoxAck(StringHash eventType, VariantMap& eventData);


    /// Input Event Handling:
    void HandleKeyDown(               StringHash eventType, VariantMap& eventData);
    void HandleMouseButtonDown(       StringHash eventType, VariantMap& eventData);
    void HandleMouseButtonUp(         StringHash eventType, VariantMap& eventData);

    /// Post-Render event handler (DebugDrawing)
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    float oldYaw_, oldPitch_;

    Component*  deleteTargetComponent_;
    Node*       deleteTargetNode_;
};
