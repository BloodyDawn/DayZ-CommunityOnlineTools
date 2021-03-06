class ObjectModule: EditorModule
{
	void ObjectModule()
	{   
		GetRPCManager().AddRPC( "COT_Object", "SpawnObjectPosition", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "COT_Object", "SpawnObjectInventory", this, SingeplayerExecutionType.Server );

		GetRPCManager().AddRPC( "COT_Object", "DeleteObject", this, SingeplayerExecutionType.Server );

		GetPermissionsManager().RegisterPermission( "Object.Spawn.Position" );
		GetPermissionsManager().RegisterPermission( "Object.Spawn.Inventory" );
		GetPermissionsManager().RegisterPermission( "Object.View" );

		GetPermissionsManager().RegisterPermission( "Object.Delete" );
	}

	override bool HasAccess()
	{
		return GetPermissionsManager().HasPermission( "Object.View" );
	}

	override string GetLayoutRoot()
	{
		return "JM/COT/GUI/layouts/Object/ObjectMenu.layout";
	} 
	
	void SpawnObjectPosition( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( !GetPermissionsManager().HasPermission( "Object.Spawn.Position", sender ) )
			return;

		Param3< string, vector, string > data;
		if ( !ctx.Read( data ) ) return;
		
		if( type == CallType.Server )
		{
			bool ai = false;

			if ( GetGame().IsKindOf( data.param1, "DZ_LightAI" ) ) 
			{
				ai = true;
			}

			EntityAI entity = EntityAI.Cast( GetGame().CreateObject( data.param1, data.param2, false, ai ) );

			if ( entity == NULL ) return;

			entity.SetHealth( entity.GetMaxHealth() );

			int quantity = 0;
			
			if ( entity.IsInherited( ItemBase ) )
			{
				ItemBase oItem = ItemBase.Cast( entity );
				SetupSpawnedItem( oItem, oItem.GetMaxHealth(), 1 );


				string text = data.param3;

				text.ToUpper();

				if (text == "MAX")
				{
					quantity = oItem.GetQuantityMax();
				} else
				{
					quantity = text.ToInt();
				}

				oItem.SetQuantity(quantity);
			}

			entity.PlaceOnSurface();

			COTLog( sender, "Spawned object " + entity.GetDisplayName() + " (" + data.param1 + ") at " + data.param2.ToString() + " with amount " + quantity );
		}
	}
	
	protected void SpawnItemOnPlayer( ref PlayerIdentity sender, PlayerBase player, string item, string quantText )
	{
		EntityAI entity = player.GetInventory().CreateInInventory( item );

		entity.SetHealth( entity.GetMaxHealth() );

		int quantity = 0;
				
		if ( entity.IsInherited( ItemBase ) )
		{
			ItemBase oItem = ItemBase.Cast( entity );
			SetupSpawnedItem( oItem, oItem.GetMaxHealth(), 1 );

			quantText.ToUpper();

			if ( quantText == "MAX")
			{
				quantity = oItem.GetQuantityMax();
			} else
			{
				quantity = quantText.ToInt();
			}

			oItem.SetQuantity(quantity);
		}
		
		COTLog( sender, "Spawned object " + entity.GetDisplayName() + " (" + item + ") on " + player.authenticatedPlayer.GetSteam64ID() + " with amount " + quantity );
	}

	void SpawnObjectInventory( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( !GetPermissionsManager().HasPermission( "Object.Spawn.Inventory", sender ) )
			return;

		ref Param3< string, string, ref array< string > > data;
		if ( !ctx.Read( data ) ) return;
		
		if( type == CallType.Server )
		{
			if ( !GetGame().IsMultiplayer() )
			{
				SpawnItemOnPlayer( sender, GetGame().GetPlayer(), data.param1, data.param2 );
			} else
			{
				array< ref AuthPlayer > players = DeserializePlayersID( data.param3 );
	
				for ( int i = 0; i < players.Count(); i++ )
				{
					SpawnItemOnPlayer( sender, players[i].PlayerObject, data.param1, data.param2 );
				}
			}
		}
	}

	void DeleteObject( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( !GetPermissionsManager().HasPermission( "Object.Delete", sender ) )
			return;
		
		if( type == CallType.Server )
		{
			if ( target == NULL ) return;

			string obtype;
			GetGame().ObjectGetType( target, obtype );

			COTLog( sender, "Deleted object " + target.GetDisplayName() + " (" + obtype + ") at " + target.GetPosition() );
			GetGame().ObjectDelete( target );
		}
	}
}