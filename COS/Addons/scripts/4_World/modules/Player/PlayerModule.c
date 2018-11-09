class PlayerModule: EditorModule
{
    ref array< Man > m_Players;

    void PlayerModule()
    {
        m_Players = new ref array< Man >;

        GetRPCManager().AddRPC( "COS_Player", "ReloadList", this, SingeplayerExecutionType.Server );
        
        GetRPCManager().AddRPC( "COS_Player", "SetPermissions", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "COS_Player", "LoadPermissions", this, SingeplayerExecutionType.Server );
    }

    override string GetLayoutRoot()
    {
        return "COS/gui/layouts/player/PlayerMenu.layout";
    }

    void SetPermissions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        if ( !GetPermissionsManager().HasPermission( sender, "Perms.Set" ) )
            return;
   
        if ( type == CallType.Server )
        {
            PlayerIdentity player = Man.Cast( target ).GetIdentity();

            Param1< ref array< string > > data;
            if ( !ctx.Read( data ) || !player ) return;

            ref array< string > perms;
            perms.Copy( data.param1 );

            ref AuthPlayer au = GetPermissionsManager().GetPlayer( player );

            if ( au )
            {

                au.ClearPermissions();

                for ( int i = 0; i < perms.Count(); i++ )
                {
                    au.AddPermission( perms[i] );
                }
            }
        }
    }

    void LoadPermissions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        if ( !GetPermissionsManager().HasPermission( sender, "Perms.Load" ) )
            return;
   
        ref array< string > perms = new ref array< string >;

        if ( type == CallType.Server )
        {
            PlayerIdentity player = Man.Cast( target ).GetIdentity();

            ref AuthPlayer au = GetPermissionsManager().GetPlayer( player );

            if ( au )
            {
                perms = au.Serialize();
                GetRPCManager().SendRPC( "COS_Player", "LoadPermissions", new Param1< ref array< string > >( perms ), true, NULL, SELECTED_PLAYER );
            }
        }

        if ( type == CallType.Client )
        {
            Param1< ref array< string > > data;
            if ( !ctx.Read( data ) ) return;

            perms.Copy( data.param1 );

            ref PlayerMenu menu = PlayerMenu.Cast( m_MenuPopup );
            
            if ( menu )
            {
                menu.LoadPermissions( perms );
            }
        }
    }

    void ReloadList( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        if ( !GetPermissionsManager().HasPermission( sender, "Player.ReloadList" ) )
            return;
        
        if ( type == CallType.Server )
        {
            m_Players.Clear();

            ref array<Man> players = new ref array<Man>;

            GetGame().GetPlayers( players );

            for ( int i = 0; i < players.Count(); i++ )
            {
                m_Players.Insert( players[i] );
            }

            GetRPCManager().SendRPC( "COS_Player", "ReloadList", new Param1< ref array< Man > >( m_Players ), true, sender );
        }

        if ( type == CallType.Client )
        {
            Param1< ref array< Man > > data;
            if ( !ctx.Read( data ) ) return;

            m_Players.Clear();

            m_Players.Copy( data.param1 );

            PlayerMenu menu = PlayerMenu.Cast( m_MenuPopup );

            Print( m_Players );
            
            if ( menu )
            {
                menu.UpdateList( m_Players );
            }
        }
    }
}