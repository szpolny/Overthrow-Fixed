class OVT_ShopContext : OVT_UIContext
{	
	protected OVT_ShopComponent m_Shop;
	protected int m_iPageNum = 0;
	protected RplId m_SelectedResource;
		
	override void PostInit()
	{
		if(SCR_Global.IsEditMode()) return;
		m_Economy.m_OnPlayerMoneyChanged.Insert(OnPlayerMoneyChanged);
	}
	
	protected void OnPlayerMoneyChanged(string playerId, int amount)
	{
		if(playerId == m_sPlayerID && m_bIsActive)
		{
			TextWidget money = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerMoney"));		
			money.SetText("$" + amount);
		}
	}
	
	override void OnShow()
	{	
		m_iPageNum = 0;	
		
		Widget buyButton = m_wRoot.FindAnyWidget("BuyButton");
		ButtonActionComponent action = ButtonActionComponent.Cast(buyButton.FindHandler(ButtonActionComponent));
		
		action.GetOnAction().Insert(Buy);
		
		Widget sellButton = m_wRoot.FindAnyWidget("SellButton");
		if(m_Shop.m_ShopType == OVT_ShopType.SHOP_GUNDEALER || m_Shop.m_ShopType == OVT_ShopType.SHOP_VEHICLE)
		{
			sellButton.SetVisible(false);
		}else{
			ButtonActionComponent sellAction = ButtonActionComponent.Cast(sellButton.FindHandler(ButtonActionComponent));
		
			sellAction.GetOnAction().Insert(Sell);
		}
		
		Refresh();		
	}
	
	void Refresh()
	{
		if(!m_Shop) return;
		
		TextWidget money = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerMoney"));
		
		money.SetText("$" + m_Economy.GetPlayerMoney(m_sPlayerID));
		
		TextWidget pages = TextWidget.Cast(m_wRoot.FindAnyWidget("Pages"));
		
		Widget grid = m_wRoot.FindAnyWidget("BrowserGrid");
		
		int numPages = Math.Ceil(m_Shop.m_aInventory.Count() / 15);
		if(m_iPageNum >= numPages) m_iPageNum = 0;
		string pageNumText = (m_iPageNum + 1).ToString();
		
		pages.SetText(pageNumText + "/" + numPages);
		
		int wi = 0;
		
		//We only read m_Shop.m_aInventory because m_Shop.m_aInventoryItems is not replicated
		for(int i = m_iPageNum * 15; i < (m_iPageNum + 1) * 15 && i < m_Shop.m_aInventory.Count(); i++)
		{			
			RplId id = m_Shop.m_aInventory.GetKey(i);
						
			if(wi == 0 && !m_SelectedResource){
				SelectItem(id);
			}
			
			Widget w = grid.FindWidget("ShopMenu_Card" + wi);
			w.SetOpacity(1);
			OVT_ShopMenuCardComponent card = OVT_ShopMenuCardComponent.Cast(w.FindHandler(OVT_ShopMenuCardComponent));
			
			int cost = m_Economy.GetPrice(id, m_Shop.GetOwner().GetOrigin());
			int qty = m_Shop.GetStock(id);
			
			card.Init(id, cost, qty, this);
			
			wi++;
		}
		
		for(; wi < 15; wi++)
		{
			Widget w = grid.FindWidget("ShopMenu_Card" + wi);			
			w.SetOpacity(0);
		}
		
	}
	
	void SelectItem(RplId id)
	{
		m_SelectedResource = id;
		TextWidget typeName = TextWidget.Cast(m_wRoot.FindAnyWidget("SelectedTypeName"));
		TextWidget details = TextWidget.Cast(m_wRoot.FindAnyWidget("SelectedDetails"));
		TextWidget desc = TextWidget.Cast(m_wRoot.FindAnyWidget("SelectedDescription"));
		
		int cost = m_Economy.GetPrice(id, m_Shop.GetOwner().GetOrigin());
		int qty = m_Shop.GetStock(id);
				
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(id));
		IEntity spawnedItem = rpl.GetEntity();
		
		SCR_EditableVehicleComponent veh = SCR_EditableVehicleComponent.Cast(spawnedItem.FindComponent(SCR_EditableVehicleComponent));
		if(veh){
			SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(veh.GetInfo());
			if(info)
			{
				typeName.SetText(info.GetName());
				details.SetText("$" + cost + "\n" + qty + " #OVT-Shop_InStock");
				desc.SetText(info.GetDescription());
			}
		}else{		
			InventoryItemComponent inv = InventoryItemComponent.Cast(spawnedItem.FindComponent(InventoryItemComponent));
			if(inv){
				SCR_ItemAttributeCollection attr = SCR_ItemAttributeCollection.Cast(inv.GetAttributes());
				if(attr)
				{
					UIInfo info = attr.GetUIInfo();
					typeName.SetText(info.GetName());
					details.SetText("$" + cost + "\n" + qty + " #OVT-Shop_InStock");
					desc.SetText(info.GetDescription());
				}
			}
		}
	}
	
	void SetShop(OVT_ShopComponent shop)
	{
		m_Shop = shop;
	}
	
	override void RegisterInputs()
	{
		super.RegisterInputs();
		if(!m_InputManager) return;
				
		m_InputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, CloseLayout);
	}
	
	override void UnregisterInputs()
	{
		super.UnregisterInputs();
		if(!m_InputManager) return;
				
		m_InputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, CloseLayout);
	}		
	
	void Buy(Widget src, float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if(m_Shop.GetStock(m_SelectedResource) < 1) return;
		
		int playerId = OVT_Global.GetPlayers().GetPlayerIDFromPersistentID(m_sPlayerID);
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if(!player) return;
		
		int cost = m_Economy.GetPrice(m_SelectedResource, m_Shop.GetOwner().GetOrigin());
		
		if(!m_Economy.PlayerHasMoney(m_sPlayerID, cost)) return;
				
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent( SCR_InventoryStorageManagerComponent ));
		if(!inventory) return;
		
		if(m_Shop.m_ShopType == OVT_ShopType.SHOP_VEHICLE)
		{
			if(OVT_Global.GetVehicles().SpawnVehicleBehind(m_SelectedResource, player, m_sPlayerID))
			{
				m_Economy.TakePlayerMoney(m_iPlayerID, cost);
				m_Shop.TakeFromInventory(m_SelectedResource, 1);
				Refresh();
				SelectItem(m_SelectedResource);
			}
			return;
		}
		
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(m_SelectedResource));
		IEntity spawnedItem = rpl.GetEntity();
		
		IEntity item = GetGame().SpawnEntityPrefab(Resource.Load(spawnedItem.GetPrefabData().GetPrefabName()));
		
		if(inventory.TryInsertItem(item))
		{
			m_Economy.TakePlayerMoney(m_iPlayerID, cost);
			m_Shop.TakeFromInventory(m_SelectedResource, 1);
		}
	}
	
	void Sell(Widget src, float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		int playerId = OVT_Global.GetPlayers().GetPlayerIDFromPersistentID(m_sPlayerID);
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if(!player) return;
		
		int cost = m_Economy.GetPrice(m_SelectedResource, m_Shop.GetOwner().GetOrigin());
		
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent( SCR_InventoryStorageManagerComponent ));
		if(!inventory) return;
		
		array<IEntity> items = new array<IEntity>;
		inventory.GetItems(items);
		
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(m_SelectedResource));
		IEntity spawnedItem = rpl.GetEntity();
		
		foreach(IEntity ent : items)
		{
			if(ent.GetPrefabData().GetPrefabName() == spawnedItem.GetPrefabData().GetPrefabName())
			{
				if(inventory.TryDeleteItem(ent))
				{
					m_Economy.AddPlayerMoney(m_iPlayerID, cost);
					m_Shop.AddToInventory(m_SelectedResource, 1);
					break;
				}
			}
		}
	}
	
	void ~OVT_ShopContext()
	{
		if(!m_Economy) return;
		m_Economy.m_OnPlayerMoneyChanged.Remove(OnPlayerMoneyChanged);
	}
}