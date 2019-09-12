
using namespace Urho3D;

class AgentController:public LogicComponent
{
    URHO3D_OBJECT(AgentController, LogicComponent);
public:
    static void RegisterObject(Context* context){ context->RegisterFactory<AgentController>(); }
    AgentController(Context* context):LogicComponent(context) { }

    virtual void DelayedStart(){

        Agent_ = GetComponent<CrowdAgent>();

        /// This event fires whenever our node is translated
        /// We can use it to determine if our agent has "arrived at destination"
        SubscribeToEvent(Agent_,E_CROWD_AGENT_REPOSITION, URHO3D_HANDLER(AgentController, HandleAgentMoved));

        // Subscribe HandleCrowdAgentFailure() function for resolving invalidation issues with agents, during which we
        // use a larger extents for finding a point on the navmesh to fix the agent's position
        SubscribeToEvent(Agent_,E_CROWD_AGENT_FAILURE, URHO3D_HANDLER(AgentController, HandleAgentFailure));

        /// This event fires when the state of the agent has changed
        SubscribeToEvent(Agent_, E_CROWD_AGENT_NODE_STATE_CHANGED, URHO3D_HANDLER(AgentController, HandleAgentStateChanged));

    }

    virtual void Update(float dT){

        int x=0;
    }


private:

    void HandleAgentMoved(StringHash eventType, VariantMap& eventData){

        using namespace CrowdAgentReposition;
        CrowdAgent* agent = (CrowdAgent*)eventData[P_CROWD_AGENT].GetPtr();
        bool hasArrived = eventData[P_ARRIVED].GetBool();
        if(hasArrived){
            //agent->ResetTarget();
            agent->SetTargetVelocity(Vector3::ZERO);
        }
        int x=0;
    }

    void HandleAgentFailure(StringHash eventType, VariantMap& eventData){
        using namespace CrowdAgentFailure;

        auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());
        auto agentState = (CrowdAgentState)eventData[P_CROWD_AGENT_STATE].GetInt();

        // If the agent's state is invalid, likely from spawning on the side of a box, find a point in a larger area
        if (agentState == CA_STATE_INVALID)
        {
            // Get a point on the navmesh using more generous extents
            Vector3 newPos = node->GetScene()->GetComponent<DynamicNavigationMesh>()->FindNearestPoint(node->GetWorldPosition(), Vector3(5.0f, 5.0f, 5.0f));
            // Set the new node position, CrowdAgent component will automatically reset the state of the agent
            node->SetWorldPosition(newPos);
        }else{

            int x=0;
        }
    }

    void HandleAgentStateChanged(StringHash eventType, VariantMap& eventData){

        using namespace CrowdAgentStateChanged;
        CrowdAgentState       astate = (CrowdAgentState)eventData[P_CROWD_AGENT_STATE].GetInt();
        CrowdAgentTargetState tstate = (CrowdAgentTargetState)eventData[P_CROWD_TARGET_STATE].GetInt();

        String astring;
        switch(astate){
            case CA_STATE_INVALID:
                astring="Invalid"; break;
            case CA_STATE_WALKING:
                astring="Walking"; break;
            case CA_STATE_OFFMESH:
                astring="OffMesh";
        }

        String tstring;
        switch(tstate){
            case CA_TARGET_NONE:
                tstring="None"; break;
            case CA_TARGET_FAILED:
                tstring="Failed"; break;
            case CA_TARGET_VALID:
                tstring="Valid"; break;
            case CA_TARGET_REQUESTING:
                tstring="Requesting"; break;
            case CA_TARGET_WAITINGFORQUEUE:
                tstring="WaitingForQueue"; break;
            case CA_TARGET_WAITINGFORPATH:
                tstring="WaitingForPath"; break;
            case CA_TARGET_VELOCITY:
                tstring="Velocity";
        }

        URHO3D_LOGINFO("AgentState: "+ astring+", TargetState: "+tstring);
        int x=0;
    }

    /// We expect a CrowdAgent Component to be attached to the same scene node as 'this' component
    WeakPtr<CrowdAgent> Agent_;

};
