
using namespace Urho3D;

class GameSceneController:public LogicComponent
{
    URHO3D_OBJECT(GameSceneController, LogicComponent);
public:
    static void RegisterObject(Context* context){
            context->RegisterFactory<GameSceneController>();
    }

    GameSceneController(Context* context):LogicComponent(context){}

    /// Restore weak pointers when scene is "ready"
    virtual void DelayedStart(){
        gameScene_ = GetNode()->GetScene();

        Variant v=gameScene_->GetVar("Character Node");
        if(v.GetType()!=VAR_NONE){
            unsigned ID = v.GetUInt();
            Node* node = gameScene_->GetNode( ID );
            characterNode_  = node;

        }
        else
            characterNode_  = gameScene_->GetChild("Character");


        Quaternion rot = characterNode_->GetWorldRotation();
        pitch_ = rot.PitchAngle();
        yaw_   = rot.YawAngle();

        int x=0;
    }

    virtual void Update(float dT){
        MoveCharacter(dT);
    }

private:
    /// Implements "character chase" camera behaviour
    void MoveCharacter(float timeStep){
        auto* input = GetSubsystem<Input>();

        const float CAMERA_DISTANCE = 10.0f;

        const float MOVE_SPEED = 18.0f;

        /// Mouse sensitivity (degrees per pixel, scaled to match display resolution)
        const float MOUSE_SENSITIVITY = 0.1f * (768.0f / GetSubsystem<Graphics>()->GetHeight());

//        Vector3 currentpos = cameraNode_->GetWorldPosition();
        Vector3 newpos;// = currentpos;
        Vector3 targetPos=characterNode_->GetWorldPosition();

        /// Convert mouse movement into change in camera pitch and yaw

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

            const BoundingBox& box = characterNode_->GetDerivedComponent<Drawable>(true)->GetBoundingBox();
            float effectiveDistance = (box.max_ - box.min_).Length() * 10.0f;//+ CAMERA_DISTANCE;
            newpos = targetPos + dir * effectiveDistance;

            if(input->GetMouseButtonDown(MOUSEB_MIDDLE)){
                // Apply rotation to character
                Quaternion q(0.0f, yaw_, 0.0f);
                characterNode_->SetWorldRotation( q );
            }else if(input->GetMouseButtonDown(MOUSEB_RIGHT))
            {

                // Get the current facing direction (character local Z axis, in worldspace)
                Vector3 currentDir =  characterNode_->GetWorldDirection();
                // Get the desired new direction (camera local Z axis, in worldspace)
                Vector3 newDir = Quaternion(0.0f, yaw_, 0.0f) * Vector3::FORWARD;
                newDir = newDir.Normalized();

                // Compute the angle between current and new directions
                // Set the rate of rotation: 5 degrees per second
                float angle = SignedAngle(currentDir, newDir, Vector3::UP);
                angle *= 5 * timeStep;

                // Apply rotation to character
                characterNode_->Rotate( Quaternion(angle, Vector3::UP), TS_WORLD);
            }


        /// Watch the WASD keys

        if (input->GetKeyDown(KEY_W) && input->GetKeyDown(KEY_SHIFT)){
                characterNode_->Translate( Vector3::FORWARD * MOVE_SPEED * timeStep);
        }

        if (input->GetKeyDown(KEY_S) && input->GetKeyDown(KEY_SHIFT)){
                characterNode_->Translate( Vector3::BACK * MOVE_SPEED * timeStep);
        }

        if (input->GetKeyDown(KEY_A) && input->GetKeyDown(KEY_SHIFT)){
                characterNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
        }

        if (input->GetKeyDown(KEY_D) && input->GetKeyDown(KEY_SHIFT)){
                characterNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
        }
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


    WeakPtr<Scene> gameScene_;
    WeakPtr<Node> characterNode_;

    float pitch_, yaw_;

    float oldPitch_, oldYaw_;

    bool isCameraLerping;

};
