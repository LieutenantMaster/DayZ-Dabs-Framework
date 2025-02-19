/*

Controller Class Example:

class TestController: Controller
{
	// "TextBox1" goes into Binding_Name
	string TextBox1 = "TextBox1Text";
	
	// "TextBox2" goes into Binding_Name
	int TextBox2;
	
	// "WindowButton1" goes into Binding_Name
	bool WindowButton1; 
	
	
	// If WindowButton1 is bound to a button,
	// call this function and it will
	// set the state of the button on your UI
	void SetWindowButton1(bool state)
	{
		WindowButton1 = state;
		NotifyPropertyChanged("WindowButton1");
	}
	
	// Gets called whenever a property was changed in the controller
	override void PropertyChanged(string propertyName)
	{
		switch (propertyName)
		{
			case "WindowButton1": {
				Print("WindowButton1 " + WindowButton1);
				break;
			}
		}
	}
}
*/

// Abstract Class
class ViewController: ScriptedViewBase
{
	// All View Bindings
	[NonSerialized()]
	protected ref ViewBindingHashMap m_ViewBindingHashMap = new ViewBindingHashMap();

	// View Bindings indexed by their Binding_Name
	[NonSerialized()]
	protected ref DataBindingHashMap m_DataBindingHashMap = new DataBindingHashMap();
	
	// Hashmap of all properties in the Controller
	[NonSerialized()]
	protected ref PropertyTypeHashMap m_PropertyTypeHashMap = new PropertyTypeHashMap(Type());

	[NonSerialized()]
	protected ref ScriptedViewBaseHandler m_ScriptedViewBaseHandler = new ScriptedViewBaseHandler(this);

	override void OnWidgetScriptInit(Widget w)
	{
		super.OnWidgetScriptInit(w);
				
		// ViewController controls the hierarchy events
		m_LayoutRoot.SetHandler(m_ScriptedViewBaseHandler);
		
		//m_PropertyTypeHashMap.RemoveType(Controller); crashing?

		// Load all child Widgets and obtain their DataBinding class
		int binding_count = LoadDataBindings(m_LayoutRoot);
		Log("%1: %2 DataBindings found!", m_LayoutRoot.GetName(), binding_count.ToString());
	}

	/*
	* Example:
	*	NotifyPropertyChanged("slider_value");
	*
	* Call this when you update a Controller property (variable)
	* Do NOT call this when using arrays / collections. Use ObservableCollection!	
	*
	* Calling this with No Parameters will update ALL properties in the Controller. 
	* This is NOT recommended because it is VERY resource intensive
	*
	*/

	void NotifyPropertyChanged(string property_name = "", bool notify_controller = true)
	{
		// Did you know that when the compiler checks for ambiguous types, it uses string.Contains()
		// instead of string.Match()? PropertyChanged and NotifyPropertyChanged need to be distinct or
		// the whole damn thing breaks. Thanks 
		Trace("NotifyPropertyChanged %1", property_name);

		if (property_name == string.Empty) {
			Log("Updating all properties in View, this is NOT recommended as it is performance intensive");
			foreach (ViewBindingSet view_array: m_DataBindingHashMap) {
				foreach (ViewBinding view_binding: view_array) {
					Trace("NotifyPropertyChanged %1", view_binding.Binding_Name);
					view_binding.UpdateView(this);
					if (notify_controller) {
						PropertyChanged(view_binding.Binding_Name);
					}
				}
			}

			return;
		}

		if (m_DataBindingHashMap[property_name]) {
			foreach (ViewBinding view: m_DataBindingHashMap[property_name]) {
				if (view) {
					view.UpdateView(this);
				}
			}
		}

		if (notify_controller) {
			PropertyChanged(property_name);
		}
	}
	
	/*
	* Example:
	*	NotifyPropertyChanged({"slider_value", "slider_number"});
	*
	*  Calls multiple Property Changes
	*/
	
	void NotifyPropertiesChanged(notnull array<string> property_names, bool notify_controller = true)
	{
		foreach (string property_name: property_names) {
			NotifyPropertyChanged(property_name, notify_controller);
		}
	}

	// Do NOT call this. ObservableCollection does this for you
	void NotifyCollectionChanged(CollectionChangedEventArgs args)
	{
		Trace("NotifyCollectionChanged %1", args.Source.ToString());

		string collection_name = GetVariableName(args.Source);
		if (collection_name == string.Empty) {
			Error("NotifyCollectionChanged could not find variable %1 in %2", args.Source.ToString(), string.ToString(this));
			return;
		}
		
		if (m_DataBindingHashMap[collection_name]) {
			foreach (ViewBinding view: m_DataBindingHashMap[collection_name]) {
				if (view) {
					view.UpdateViewFromCollection(args);
				}
			}
		}

		CollectionChanged(collection_name, args);
	}

	// Gets called every time a property is changed.
	// Override this when you want to have an event AFTER property is changed
	void PropertyChanged(string property_name)
	{
	}

	// Gets called every time an observable collection is changed.
	// Override this when you want to have an event AFTER collection is changed
	void CollectionChanged(string collection_name, CollectionChangedEventArgs args)
	{
	}

	protected int LoadDataBindings(Widget w)
	{
		ScriptedViewBase view_base;
		w.GetScript(view_base);

		// If we find a ViewBinding
		if (view_base && view_base.IsInherited(ViewBinding)) {
			ViewBinding view_binding = ViewBinding.Cast(view_base);
			view_binding.SetParent(this);
			m_ViewBindingHashMap.Insert(w, view_binding);
			m_DataBindingHashMap.InsertView(view_binding);

			view_binding.SetProperties(GetControllerProperty(view_binding.Binding_Name), GetControllerProperty(view_binding.Selected_Item));

			// todo find a way to define these on ScriptView aswell
			// Load RelayCommand
			if (view_binding.Relay_Command != string.Empty) {
				RelayCommand relay_command = LoadRelayCommand(view_binding);
				// Success! One of the two options were found
				if (relay_command) {
					Log("%2: RelayCommand %1 succesfully acquired. Assigning...", view_binding.Relay_Command, view_binding.GetLayoutRoot().GetName());
					relay_command.SetController(this);
					view_binding.SetRelayCommand(relay_command);
				} 
				
				else { // Must be a function on the controller
					Log("%2: RelayCommand %1 not found - Assuming its a function on the ViewController / ScriptView!", view_binding.Relay_Command, view_binding.GetLayoutRoot().GetName());
				}
			}

			// Load property for the first time
			if (view_binding.Binding_Name != string.Empty) {
				NotifyPropertyChanged(view_binding.Binding_Name, false);
			}
		}

		// really wish i had XOR here
		bool b1 = (w.GetChildren() != null);
		bool b2 = (view_base && view_base.IsInherited(ViewController) && view_base != this);

		// scuffed XOR
		// Makes it stop loading when it finds another controller
		// needs to be looked at
		if (b1 && (b1 || b2) && !(b1 && b2)) {
			LoadDataBindings(w.GetChildren());
		} 
		
		// Sets parent of the child controller
		else if (b2) { 
			ViewController child_controller = ViewController.Cast(view_base);
			if (child_controller) {
				child_controller.SetParent(this);
			}
		}

		// w != m_LayoutRoot is so we dont bleed into siblings
		if (w.GetSibling() != null && w != m_LayoutRoot) {
			LoadDataBindings(w.GetSibling());
		}

		return m_DataBindingHashMap.Count();
	}

	protected typename GetControllerProperty(string property_name)
	{
		if (m_PropertyTypeHashMap[property_name]) {
			return m_PropertyTypeHashMap[property_name];
		}

		// Searches properties for Sub properties
		Class context = this;
		return GetControllerProperty(context, property_name);
	}

	protected typename GetControllerProperty(out Class context, string property_name)
	{
		PropertyInfo property_info = PropertyInfo.GetSubScope(context, property_name);
		if (property_info) {
			return property_info.Type;
		}

		return EMPTY_TYPENAME;
	}

	protected string GetVariableName(Class target_variable)
	{
		typename type = Type();
		for (int i = 0; i < type.GetVariableCount(); i++) {
			typename variableType = type.GetVariableType(i);
			string variableName = type.GetVariableName(i);

			// optimization
			if (!variableType.IsInherited(target_variable.Type())) {
				continue;
			}

			Class result;
			EnScript.GetClassVar(this, variableName, 0, result);
			if (result == target_variable) {
				return variableName;
			}
		}

		return string.Empty;
	}

	private	RelayCommand LoadRelayCommand(ViewBinding view_binding)
	{
		string relay_command_name = view_binding.Relay_Command;
		RelayCommand relay_command;

		// Attempt to load instance of Variable from Controller
		Class context = this;
		PropertyInfo relay_command_property = PropertyInfo.GetSubScope(context, relay_command_name);
		if (relay_command_property) {
			typename relay_command_type = relay_command_property.Type;
			relay_command_name = relay_command_property.Name;

			// If we find the variable on the Controller
			if (relay_command_type && relay_command_type.IsInherited(RelayCommand)) {
				Log("RelayCommand Property %1 found on Controller!", relay_command_name);
				EnScript.GetClassVar(context, relay_command_name, 0, relay_command);

				// If that property isnt initialized, but exists
				if (!relay_command) {
					Log("RelayCommand Property %1 was not initialized! Initializing...", relay_command_name);
					Class.CastTo(relay_command, relay_command_type.Spawn());
					EnScript.SetClassVar(context, relay_command_name, 0, relay_command);
					return relay_command;
				}
			}
		} 
		
		// If we DONT find the variable on the controller, attempt to create an instance of it
		else { 
			Log("RelayCommand Property %1 not found on Controller", relay_command_name);
			relay_command_type = relay_command_name.ToType();

			if (relay_command_type && relay_command_type.IsInherited(RelayCommand)) {
				Log("RelayCommand type found %1", relay_command_name);
				Class.CastTo(relay_command, relay_command_type.Spawn());
				return relay_command;
			}
		}

		return relay_command;
	}

	// Update Controller on action from ViewBinding
	override bool OnClick(Widget w, int x, int y, int button)
	{
		Trace("OnClick");
		ViewBinding view_binding = m_ViewBindingHashMap.Get(w);
		if (view_binding) {
			view_binding.UpdateController(this);
			
			switch (w.Type()) {
				case ButtonWidget: { // only thing that isnt called in OnChange for some reason
					if (view_binding.InvokeCommand(this, new ButtonCommandArgs(ButtonWidget.Cast(w), button))) {	
						return true;
					}
	
					break;
				}
			}
		}
		
		return super.OnClick(w, x, y, button);
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		Trace("OnChange");	
		
		ViewBinding view_binding = m_ViewBindingHashMap.Get(w);
		if (view_binding) {
			view_binding.UpdateController(this);
			
			switch (w.Type()) {
				case CheckBoxWidget: {
					if (view_binding.InvokeCommand(this, new CheckBoxCommandArgs(CheckBoxWidget.Cast(w)))) {						
						return true;
					}
	
					break;
				}
			}
		}

		return super.OnChange(w, x, y, finished);
	}
	
	
	override bool OnKeyPress(Widget w, int x, int y, int key)
	{
		Trace("OnKeyPress");
		ViewBinding view_binding = m_ViewBindingHashMap.Get(w);
		if (view_binding) {
			view_binding.UpdateController(this);
		}

		return super.OnKeyPress(w, x, y, key);
	}
	

	// Two way binding interfaces
	// Specifically for SpacerBaseWidget
	/*
	override bool OnDropReceived(Widget w, int x, int y, Widget reciever)
	{
		ViewBinding reciever_view;
		reciever.GetScript(reciever_view);
		//ScriptedViewBase.FindScriptedRoot(reciever_view);
		if (reciever_view) {
			
			ScriptedViewBase scripted_view;
			w.GetScript(scripted_view);
			ScriptedViewBase.FindScriptedRoot(scripted_view);
			if (scripted_view) {
				reciever_view.HandleDropReceived(scripted_view, this);
			}
		}
		
		return super.OnDropReceived(w, x, y, reciever);
	}
	*/	
	DataBindingHashMap GetDataBindings()
	{
		return m_DataBindingHashMap;
	}
	
	typename GetPropertyType(string property_name)
	{
		return m_PropertyTypeHashMap[property_name];
	}
	
	ViewBindingHashMap GetViewBindings()
	{
		return m_ViewBindingHashMap;
	}

	ViewBinding GetViewBinding(Widget source)
	{
		return m_ViewBindingHashMap[source];
	}
	
	ScriptedViewBaseHandler GetHandler()
	{
		return m_ScriptedViewBaseHandler;
	}
}