class CameraTool: EditorModule
{
    protected Camera m_oCamera; // active static camera "staticcamera"

    protected ref array<vector> m_cKeyframes = new array<vector>;

    // float velocity = vector magnitutde
    // float acceleration;
    // force = input forward/back
    // resistance

    protected vector velocity;

    protected float yawVelocity;
    protected float pitchVelocity;

    protected float m_CamFOV = 1.0; // default FOV
    protected float m_TargetFOV = 1.0;
    protected float m_TargetRoll;
    protected float m_DistanceToObject;
    protected bool m_FollowTarget = false;
    protected bool m_OrbitalCam = false;
    protected bool m_FreezeCam = false;

    protected bool m_FreezeMouse = false;
    
    static float CAMERA_FOV = 1.0;
    static float CAMERA_TARGETFOV = 1.0;
    static float CAMERA_FOV_SPEED_MODIFIER = 5.0;
    static float CAMERA_SPEED = 5.0;
    static float CAMERA_VELDRAG = 0.9; // 0.9 - 1.0 0.9 == no smoothing
    static float CAMERA_MSENS = 0.8; // acceleration
    static float CAMERA_SMOOTH = 0.8; // drag

    static bool  CAMERA_DOF = false;
    static bool  CAMERA_AFOCUS = true;
    static float CAMERA_BLUR = 0.0; // modified via ui
    static float CAMERA_FLENGTH = 50.0; // modified via ui
    static float CAMERA_FNEAR = 50.0; // modified via ui
    static float CAMERA_FDIST = 0.0;
    static float CAMERA_DOFFSET = 0.0;

    static float CAMERA_SMOOTH_BLUR = 0.0;

    static float EXPOSURE = 0.0;
    static float CHROMABERX = 0.0; // these need to go somewhere else. not where this object is GC'd
    static float CHROMABERY = 0.0;
    static float HUESHIFT = 0.0;

    static float ROTBLUR = 0.0;
    static float MINDEPTH = 2.5;
    static float MAXDEPTH = 4.5;

    static float RADBLURX = 0.0;
    static float RADBLURY = 0.0;
    static float RADBLUROFFX = 0.0;
    static float RADBLUROFFY = 0.0;

    static float VIGNETTE = 0.0;
    static float VARGB[4] = { 0, 0, 0, 0 };

    static float CARGB[4] = { 0, 0, 0, 1 }; // color overlay

    static int VIEWDISTANCE = 1600; // move later

    protected vector m_CamOffset;
    
    protected Object m_Target;
    protected vector m_TargetPos; // Static position
    
    protected float m_CurrentSmoothBlur;

    void CameraTool()
    {
        GetRPCManager().AddRPC( "COT_Camera", "EnterCamera", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "COT_Camera", "LeaveCamera", this, SingeplayerExecutionType.Server );

        GetPermissionsManager().RegisterPermission( "CameraTools.EnterCamera" );
        GetPermissionsManager().RegisterPermission( "CameraTools.LeaveCamera" );

    }

    void ~CameraTool()
    {
    }

    override string GetLayoutRoot()
    {
        return "COT/gui/layouts/Camera/CameraSettings.layout";
    }
    
    override void Init() 
    {
        Print("CameraTool::Init");
        super.Init();
    }
    
    override void onUpdate( float timeslice )
    {
        float speed = 0.2;
        m_CurrentSmoothBlur = Math.Lerp( m_CurrentSmoothBlur, CAMERA_SMOOTH_BLUR, speed );
        PPEffects.SetBlur( m_CurrentSmoothBlur );

        UpdateCamera( timeslice );
    }
    
    override void RegisterKeyMouseBindings() 
    {
        Print("CameraTool::RegisterKeyMouseBindings");
        KeyMouseBinding toggleCamera  = new KeyMouseBinding( GetModuleType(), "ToggleCamera" , "[Insert]"    , "Toggle camera." );
        KeyMouseBinding freezeCamera  = new KeyMouseBinding( GetModuleType(), "FreezeCamera" , "[BackSlash]" , "Freezes camera." );
        KeyMouseBinding followTarget  = new KeyMouseBinding( GetModuleType(), "FollowTarget" , "[LBracket]"  , "Follows target." );
        KeyMouseBinding toggleOrbit   = new KeyMouseBinding( GetModuleType(), "ToggleOrbital", "[RBracket]"  , "Toggle orbital mode", true );
        KeyMouseBinding targetCamera  = new KeyMouseBinding( GetModuleType(), "TargetCamera" , "[Return]"     , "Targets objects or positions", true );
        KeyMouseBinding zoomCamera    = new KeyMouseBinding( GetModuleType(), "ZoomCamera"   , "(RMB)+(Drag)", "Zooms camera"     , true);
        KeyMouseBinding speedCamera   = new KeyMouseBinding( GetModuleType(), "CameraSpeed"  , "(MouseWheel)", "Change camera speed", true);

        toggleCamera.AddKeyBind( KeyCode.KC_INSERT    , KeyMouseBinding.KB_EVENT_RELEASE );
        freezeCamera.AddKeyBind( KeyCode.KC_BACKSLASH , KeyMouseBinding.KB_EVENT_RELEASE );
        followTarget.AddKeyBind( KeyCode.KC_LBRACKET  , KeyMouseBinding.KB_EVENT_RELEASE );
        toggleOrbit .AddKeyBind( KeyCode.KC_RBRACKET  , KeyMouseBinding.KB_EVENT_RELEASE );
        
        targetCamera.AddMouseBind( MouseState.MIDDLE , KeyMouseBinding.MB_EVENT_CLICK );
        
        zoomCamera    .AddMouseBind( MouseState.RIGHT  , KeyMouseBinding.MB_EVENT_DRAG  );
        zoomCamera  .AddKeyBind( KeyCode.KC_LCONTROL     , KeyMouseBinding.KB_EVENT_HOLD  );
            
        speedCamera .AddMouseBind( MouseState.WHEEL, 0);    
        //zoomCamera    .AddMouseBind( MouseState.WHEEL, 0 );
        
        RegisterKeyMouseBinding( toggleCamera );
        RegisterKeyMouseBinding( freezeCamera );
        RegisterKeyMouseBinding( followTarget );
        RegisterKeyMouseBinding( toggleOrbit  );
        RegisterKeyMouseBinding( targetCamera );
        RegisterKeyMouseBinding( zoomCamera   );
        RegisterKeyMouseBinding( speedCamera  );
    }

    Camera GetCamera()
    {
        return m_oCamera;
    }

    void EnterCamera( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        Print("CameraTool::EnterCamera");

        if ( !GetPermissionsManager().HasPermission( "CameraTools.EnterCamera", sender ) )
            return;

        bool cont = false;

        if( type == CallType.Server )
        {
            GetGame().SelectSpectator( sender, "StaticCamera", target.GetPosition() + Vector( 0, 1.8, 0 ) );

            GetGame().SelectPlayer( sender, NULL );

            if ( GetGame().IsMultiplayer() )
            {
                GetRPCManager().SendRPC( "COT_Camera", "EnterCamera", new Param, true, sender );
            } else
            {
                cont = true;
            }
        }

        if( type == CallType.Client || cont )
        {
            m_oCamera = Camera.GetCurrentCamera();
        }
    }

    void LeaveCamera( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        Print("CameraTool::LeaveCamera");

        if ( !GetPermissionsManager().HasPermission( "CameraTools.LeaveCamera", sender ) )
            return;

        bool cont = false;

        if( type == CallType.Server )
        {
            Param1< vector > data;
            if ( !ctx.Read( data ) ) return;

            GetGame().SelectPlayer( sender, target );

            if ( target ) 
            {
                target.SetPosition( data.param1 );
            }

            if ( GetGame().IsMultiplayer() )
            {
                GetRPCManager().SendRPC( "COT_Camera", "LeaveCamera", new Param, true, sender );
            } else
            {
                cont = true;
            }
        }

        if( type == CallType.Client || cont )
        {
            m_oCamera.SetActive( false );

            m_oCamera = NULL;
            
            m_CamFOV = 1.0;
            m_TargetFOV = 1.0;
            m_TargetRoll = 0;
            
            m_FollowTarget = false;
            m_OrbitalCam = false;
            
            m_Target = NULL;
            m_TargetPos = vector.Zero;
            
            PPEffects.ResetDOFOverride();
        }
    }

    void EnableCamera( bool staticCam = false )
    {
        Print("CameraTool::EnableCamera");

        if ( m_oCamera )
        {
            return;
        }
        
        m_DistanceToObject = 0.0;

        GetRPCManager().SendRPC( "COT_Camera", "EnterCamera", new Param, true, NULL, GetPlayer() );
    }

    void DisableCamera()
    {
        Print("CameraTool::DisableCamera");

        if ( m_oCamera )
        {
            SetFreezeMouse(false);

            vector position = "0 0 0";

            if( CTRL() || SHIFT() ) // Extra
            {
                position = m_oCamera.GetPosition();
                position[1] = GetGame().SurfaceY( position[0], position[2] );
            }
            else
            {
                position = GetCursorPos();
            }

            GetRPCManager().SendRPC( "COT_Camera", "LeaveCamera", new Param1<vector>(position), true, NULL, GetPlayer() );
        }
    }
    
    void ToggleCamera() 
    {
        Print("CameraTool::ToggleCamera");
        if ( m_oCamera )
        {
            DisableCamera();
        }
        else
        {
            EnableCamera();
        }
    }
    
    void TargetCamera() 
    {
        if ( m_oCamera ) 
        {
            if ( GetGame().GetUIManager().IsCursorVisible() ) 
            {
                return;
            }

            if ( m_Target || m_TargetPos != vector.Zero ) 
            {
                // Stop targetting
                m_Target = NULL;
                m_TargetPos = vector.Zero;
            
                SetFreezeMouse(false);

                return;
            }
            
            vector dir = GetGame().GetPointerDirection();
            vector from = GetGame().GetCurrentCameraPosition();
            vector to = from + ( dir * 10000 );
            
            set< Object > objects = GetObjectsAt(from, to);
            
            if ( objects.Count() > 0) 
            {
                Object object = objects.Get(0);
                if ( object.IsInherited( EntityAI ) && !object.IsBuilding() )
                {
                    m_Target = object;
                    return;
                } 
            }
            m_TargetPos = GetCursorPos();
        }
    }

    void SetTarget(Object oObject)
    {
        if (oObject)
        {
            m_Target = oObject;
            m_TargetPos = oObject.GetPosition();
        } else
        {
            m_Target = NULL;
        }
    }
    
    void UpdateCamera( float timeslice) 
    {
        if ( m_oCamera ) 
        {

            if ( m_CamFOV != m_TargetFOV ) 
            {
                m_CamFOV = Math.Lerp( m_CamFOV, m_TargetFOV, timeslice*CAMERA_FOV_SPEED_MODIFIER );
                m_oCamera.SetFOV( m_CamFOV );
            }

            vector oldOrient = m_oCamera.GetOrientation();
            if ( oldOrient[2] != m_TargetRoll ) 
            {
                //oldOrient[2] = Math.Lerp( oldOrient[2], m_TargetRoll, timeslice*CAMERA_FOV_SPEED_MODIFIER );
                //m_oCamera.SetOrientation( oldOrient );
            }

            // Camera movement
            Input input = GetGame().GetInput();

            if ( !m_FreezeCam ) 
            {
                float forward = KeyState(KeyCode.KC_W) - KeyState(KeyCode.KC_S); // -1, 0, 1
                float strafe = KeyState(KeyCode.KC_D) - KeyState(KeyCode.KC_A);
                float altitude = KeyState(KeyCode.KC_Q) - KeyState(KeyCode.KC_Z); // change to hardcode keys? these actions can be rebinded via vanilla keybind menu

                if( KeyState(KeyCode.KC_LSHIFT) ) 
                {
                    forward *= 10.0;
                    strafe *= 10.0;
                    altitude *= 10.0;
                }

                float cam_speed = CAMERA_SPEED;

                vector up = vector.Up;
                vector direction = m_oCamera.GetDirection();
                vector directionAside = vector.Up * direction;

                vector oldPos = m_oCamera.GetPosition();

                up = up * altitude * cam_speed;
                direction = direction * forward * cam_speed;
                directionAside = directionAside * strafe * cam_speed;

                velocity = velocity * CAMERA_VELDRAG;
                velocity = velocity + (direction + directionAside + up) * timeslice;

                vector newPos = oldPos + velocity;

                float surfaceY = GetGame().SurfaceY( newPos[0], newPos[2] ) + 0.25;
                if ( newPos[1] < surfaceY ) 
                {
                    newPos[1] = surfaceY;
                }

                m_oCamera.SetPosition(newPos);
            }

            if ( !m_FreezeMouse ) 
            {
                float yawDiff = input.GetAction(UAAimLeft) - input.GetAction(UAAimRight);
                float pitchDiff = input.GetAction(UAAimDown) - input.GetAction(UAAimUp);

                yawVelocity = yawVelocity + yawDiff * CAMERA_MSENS;
                pitchVelocity = pitchVelocity + pitchDiff * CAMERA_MSENS; // 0.8

                vector newOrient = oldOrient;

                //yawVelocity = Math.Clamp ( yawVelocity, -1.5, 1.5);
                //pitchVelocity = Math.Clamp ( pitchVelocity, -1.5, 1.5);

                newOrient[0] = oldOrient[0] - Math.RAD2DEG * yawVelocity * timeslice;
                newOrient[1] = oldOrient[1] - Math.RAD2DEG * pitchVelocity * timeslice;

                yawVelocity *= CAMERA_SMOOTH; // drag 0.9
                pitchVelocity *= CAMERA_SMOOTH;

                if( newOrient[1] < -89 )
                    newOrient[1] = -89;
                if( newOrient[1] > 89 )
                    newOrient[1] = 89;

                m_oCamera.SetOrientation( newOrient );
            }


            // Camera targetting
            float dist = 0.0;
            vector from = GetGame().GetCurrentCameraPosition();
    
            if ( m_Target ) 
            {
                vector targetPos;
                
                if ( m_Target.IsInherited( SurvivorBase ) ) 
                {
                    targetPos = GetTargetCenter();
                }
                else 
                {
                    vector pos = m_Target.GetPosition();
                    pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
                    
                    vector clippingInfo;
                    vector objectBBOX;
                    
                    m_Target.GetCollisionBox(objectBBOX);
                    
                    pos[1] = (pos[1] - objectBBOX[1] + clippingInfo[1] - objectBBOX[1]) + 1.5;
                    
                    targetPos = pos;
                }
                
                if ( m_OrbitalCam ) 
                {
                    m_oCamera.LookAt( targetPos );
                }
                
                dist = vector.Distance( from, targetPos );
                
                if ( m_FollowTarget ) 
                {
                    if ( m_DistanceToObject == 0.0 )
                    {
                        m_DistanceToObject = vector.Distance(GetTargetCenter(), m_oCamera.GetPosition());
                        m_CamOffset = vector.Direction( GetTargetCenter() , m_oCamera.GetPosition() );
                        m_CamOffset.Normalize();
                    }
                    
                    if ( m_OrbitalCam ) 
                    {
                        direction = vector.Direction( GetTargetCenter() , m_oCamera.GetPosition() );
                        direction.Normalize();
                        newPos = GetTargetCenter() + ( direction * m_DistanceToObject );
                    } 
                    else 
                    {
                        newPos = GetTargetCenter() + ( m_CamOffset * m_DistanceToObject );
                    }
                    
                    m_oCamera.SetPosition( newPos );
                    dist = m_DistanceToObject;
                }
            }
            else if ( m_TargetPos != vector.Zero ) 
            {
                // m_oCamera.LookAt( m_TargetPos ); // auto orbital
                vector lookDir = m_TargetPos - m_oCamera.GetPosition();
                float roll = m_oCamera.GetOrientation()[2];
                m_oCamera.SetDirection( lookDir );

                vector newRoll = m_oCamera.GetOrientation();
                newRoll[2] = roll;
                m_oCamera.SetOrientation(newRoll);
                dist = vector.Distance( from, m_TargetPos );
            }
            
            if ( CAMERA_DOF ) // DOF enabled
            {
                if ( CAMERA_AFOCUS && !m_Target ) //auto focus
                {
                    vector to = from + (GetGame().GetCurrentCameraDirection() * 9999);
                    vector contact_pos;
                    
                    DayZPhysics.RaycastRV( from, to, contact_pos, NULL, NULL, NULL , NULL, NULL, false, false, ObjIntersectIFire);
                    dist = vector.Distance( from, contact_pos );
                }
                if ( dist > 0 ) CAMERA_FDIST = dist;
                
                PPEffects.OverrideDOF(true, CAMERA_FDIST, CAMERA_FLENGTH, CAMERA_FNEAR, CAMERA_BLUR, CAMERA_DOFFSET);
            }
        }
    }
    
    void CameraSpeed() 
    {
        if ( m_oCamera ) 
        {
            if ( GetGame().GetUIManager().IsCursorVisible() ) 
            {
                return;
            }
            int i = GetMouseState( MouseState.WHEEL );

            if ( ALT() ) 
            {
                vector ori = m_oCamera.GetOrientation();
                ori[2] = ori[2] - i*5;
                m_oCamera.SetOrientation( ori );
                //m_TargetRoll = ori[2] - i*5; // redo this
                //Message(m_TargetRoll.ToString());
            }
            else 
            {
                float value = 1.2;
                if ( i < 0 ) 
                {
                    value = 0.8;
                }

                CAMERA_SPEED *= value;
                if ( CAMERA_SPEED < 0.001 ) 
                {
                    CAMERA_SPEED = 0.001;
                }
            }
        }
    }

    void ZoomCamera() 
    {
        if ( m_oCamera ) 
        {
            if ( GetGame().GetUIManager().IsCursorVisible() ) 
            {
                return;
            }

            int i = GetMouseState(MouseState.Y);
        
            if ( i != 0 ) 
            {
                SetFreezeMouse(true);
                m_TargetFOV+=i*0.000006; // zoom speed
                
                if ( m_TargetFOV < 0.01 ) 
                {
                    m_TargetFOV = 0.01;
                }
                //m_oCamera.SetFOV(m_CamFOV);
            }
        }
        
        /*
        if ( m_oCamera ) 
        {
            if ( GetGame().GetUIManager().IsCursorVisible() ) 
            {
                return;
            }

            int i = GetMouseState( MouseState.WHEEL );

            ObjectEditor objEditor = GetModuleManager().GetModule( ObjectEditor );

            if ( objEditor.m_SelectedObject ) 
            {    
                return;
            }

            if ( CTRL() ) 
            {
                vector ori = m_oCamera.GetOrientation();
                m_TargetRoll = ori[2] - Math.RAD2DEG * i*0.09;
            }
            else 
            {
                m_TargetFOV-=i*0.09; // invert
                if ( m_TargetFOV < 0.01 ) 
                {
                    m_TargetFOV = 0.01;
                }
            }
            //m_oCamera.SetFOV(m_CamFOV);    
        }
        */
    }
    
    void FreezeCamera() 
    {
        if ( m_oCamera ) 
        {
            SetFreezeCam(!m_FreezeCam);
        }        
    }
    
    void FollowTarget()
    {
        if ( m_oCamera ) 
        {
            if ( m_Target || m_TargetPos != vector.Zero ) 
            {
                m_FollowTarget = !m_FollowTarget;
                SetFreezeMouse( m_FollowTarget );
                SetFreezeCam( m_FollowTarget );
                
                if ( !m_FollowTarget ) 
                {
                    m_DistanceToObject = 0.0;
                    m_CamOffset = vector.Zero;
                }
            }
        }        
    }
    
    void ToggleOrbital() 
    {
        if ( m_oCamera )
        {
            if ( m_Target || m_TargetPos != vector.Zero ) 
            {
                m_OrbitalCam = !m_OrbitalCam;
                SetFreezeMouse(m_OrbitalCam);
                if (! m_OrbitalCam ) 
                {
                    SetFreezeCam(false);
                }
            }
        }        
    }
    
    override void onKeyRelease( int key ) // Fix
    {
        switch( key ) 
        {            
            case KeyCode.KC_W:
            case KeyCode.KC_A:
            case KeyCode.KC_S:
            case KeyCode.KC_D:
            case KeyCode.KC_Q:
            case KeyCode.KC_Z:
            {
                if ( m_oCamera )
                {
                    if ( m_Target ) 
                    {
                        m_CamOffset = vector.Direction( GetTargetCenter() , m_oCamera.GetPosition() );
                        m_CamOffset.Normalize();
                        break;
                    }
                }
            }
        }
    }
    
    override void onMouseButtonRelease( int button ) 
    {
        if ( m_oCamera ) 
        {
            if ( button == MouseState.RIGHT ) 
            {
                if ( !m_OrbitalCam ) 
                {
                    SetFreezeMouse( false );
                }
            }
        }
    }
    
    override void onMouseButtonPress( int button ) 
    {
    }
    
    bool IsUsingCamera() 
    {
        return m_oCamera != NULL;
    }
    
    vector GetTargetCenter() 
    {
        vector targetPosition;
        
        if ( m_Target.IsInherited( SurvivorBase )) 
        {
            targetPosition = m_Target.GetPosition();
            targetPosition[1] = targetPosition[1] + 1.5;
        }
        else 
        {
            targetPosition = m_Target.GetPosition();
            targetPosition[1] = GetGame().SurfaceY(targetPosition[0], targetPosition[2]);
            
            vector clippingInfo;
            vector objectBBOX;
            
            m_Target.GetCollisionBox(objectBBOX);
            
            targetPosition[1] = (targetPosition[1] - objectBBOX[1] + clippingInfo[1] - objectBBOX[1]) + 1.5;
        }
                
        return targetPosition;
    }

    Object GetTargetObject() 
    {
        return m_Target;
    }

    vector GetTargetPos() 
    {
        return m_TargetPos;
    }
    
    void SetFreezeCam( bool freeze ) 
    {
        m_FreezeCam = freeze;
    }
    
    void SetFreezeMouse( bool freeze ) 
    {
        m_FreezeMouse = freeze;
    }
}